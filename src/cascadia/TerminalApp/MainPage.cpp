﻿#include "pch.h"
#include "MainPage.h"
#include <sstream>


using namespace winrt;
using namespace Windows::UI::Xaml;

using namespace winrt::Microsoft::Graphics::Canvas;
using namespace winrt::Microsoft::Graphics::Canvas::Text;
using namespace winrt::Microsoft::Graphics::Canvas::UI::Xaml;
// using namespace Microsoft::Graphics::Canvas::UI::Xaml;

using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;

using namespace Windows::UI;

namespace winrt::TerminalApp::implementation
{
    MainPage::MainPage() :
        _renderTarget{},
        _connection{TerminalConnection::ConptyConnection(L"cmd.exe", 32, 80)},
        _engine{ &_dispatch },
        _canvasView{ nullptr, L"Consolas", 12.0f }
    {
        InitializeComponent();
        _canvasView = TerminalCanvasView( canvas00(), L"Consolas", 12.0f );
        // Do this to avoid having to std::bind(canvasControl_Draw, this, placeholders::_1)
        // Which I don't even know if it would work
        canvas00().Draw([&](const auto& s, const auto& args) { terminalView_Draw(s, args); });
        canvas00().CreateResources([&](const auto& /*s*/, const auto& /*args*/)
        {
            _canvasView.Initialize();
            // The Canvas view must be initialized first so we can get the size from it.
            _InitializeTerminal();
        });


    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        auto cursorX = _buffer->GetCursor().GetPosition().X;

        const auto burrito = L"🌯";
        OutputCellIterator burriter{ burrito };

        _buffer->Write({ L"F" });
        _buffer->IncrementCursor();
        _buffer->Write(burriter);
        _buffer->IncrementCursor();
        _buffer->IncrementCursor();

        _canvasView.Invalidate();
    }

    void MainPage::canvasControl_Draw(const CanvasControl& sender, const CanvasDrawEventArgs & args)
    {
        CanvasTextFormat format;
        format.HorizontalAlignment(CanvasHorizontalAlignment::Center);
        format.VerticalAlignment(CanvasVerticalAlignment::Center);
        format.FontSize(72.0f);
        format.FontFamily(L"Segoe UI Semibold");

        float2 size = sender.Size();
        float2 center{ size.x / 2.0f, size.y / 2.0f };
        Rect bounds{ 0.0f, 0.0f, size.x, size.y };

        CanvasDrawingSession session = args.DrawingSession();

        session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());
        session.DrawText(L"Win2D with\nC++/WinRT!", bounds, Colors::Orange(), format);
    }

    void MainPage::_InitializeTerminal()
    {
        // DO NOT USE canvase00().Width(), those are nan?
        float windowWidth = canvas00().Size().Width;
        float windowHeight = canvas00().Size().Height;
        COORD viewportSizeInChars = _canvasView.PixelsToChars(windowWidth, windowHeight);

        // COORD bufferSize { 80, 9001 };
        COORD bufferSize { viewportSizeInChars.X, viewportSizeInChars.Y + 9001 };
        UINT cursorSize = 12;
        TextAttribute attr{};
        _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, _renderTarget);

        // Display our calculated buffer, viewport size
        std::wstringstream bufferSizeSS;
        bufferSizeSS << L"{" << bufferSize.X << L", " << bufferSize.Y << L"}";
        BufferSizeText().Text(bufferSizeSS.str());

        std::wstringstream viewportSizeSS;
        viewportSizeSS << L"{" << viewportSizeInChars.X << L", " << viewportSizeInChars.Y << L"}";
        ViewportSizeText().Text(viewportSizeSS.str());

        auto fn = [&](const hstring str) {
            std::wstring_view _v(str.c_str());
            _buffer->Write(_v);
            for (int x = 0; x < _v.size(); x++) _buffer->IncrementCursor();
        };
        _connectionOutputEventToken = _connection.TerminalOutput(fn);
        _connection.Start();
    }

    void MainPage::terminalView_Draw(const CanvasControl& sender, const CanvasDrawEventArgs & args)
    {
        float2 size = sender.Size();
        float2 center{ size.x / 2.0f, size.y / 2.0f };

        CanvasDrawingSession session = args.DrawingSession();

        session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());

        if (_buffer == nullptr) return;
        auto textIter = _buffer->GetTextLineDataAt({ 0, 0 });
        std::wstring wstr = L"";
        while (textIter)
        {
            auto view = *textIter;
            wstr += view;
            textIter += view.size();
        }

        _canvasView.PrepDrawingSession(session);
        _canvasView.PaintRun({ wstr }, { 0, 0 }, Colors::White(), Colors::Black());
        _canvasView.FinishDrawingSession();
    }
}

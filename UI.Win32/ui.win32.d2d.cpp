module;

#include <Windows.h>
#include <d2d1.h>

module ui.win32.d2d;

import <stdexcept>;
import <variant>;
import domain.diagram;

using Microsoft::WRL::ComPtr;


namespace ui::win32::d2d
{
    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram)
    {
        D2DContext ctx{};

        D2D1_FACTORY_OPTIONS options{};
        auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory),
            &options,
            reinterpret_cast<void**>(ctx.factory.GetAddressOf()));
        if (FAILED(hr)) throw std::runtime_error("Failed to create D2D factory");

        RECT rc{};
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = ctx.factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            ctx.renderTarget.GetAddressOf());

        if (FAILED(hr)) throw std::runtime_error("Failed to create HwndRenderTarget");

        ctx.node = FakeNode{.x = 100.f, .y = 100.f, .width = 200.f, .height = 120.f};
        
		
		ctx.diagram = diagram;

        return ctx;
    }

    void resize(D2DContext& ctx, UINT width, UINT height)
    {
        if (ctx.renderTarget)
        {
            ctx.renderTarget->Resize(D2D1::SizeU(width, height));
        }
    }

    void render(D2DContext& ctx)
    {
        if (!ctx.renderTarget || !ctx.diagram)
            return;

        ctx.renderTarget->BeginDraw();
        ctx.renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        ctx.renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
            brush.GetAddressOf()
        );

        for (auto& shape : ctx.diagram->shapes())
        {
            std::visit([&](auto&& s)
                {
                    using T = std::decay_t<decltype(s)>;

                    if constexpr (std::is_same_v<T, domain::Rectangle>)
                    {
                        D2D1_RECT_F rect = D2D1::RectF(
                            s.x, s.y,
                            s.x + s.width,
                            s.y + s.height
                        );

                        ctx.renderTarget->FillRectangle(rect, brush.Get());
                        ctx.renderTarget->DrawRectangle(rect, brush.Get(), 2.0f);
                    }
                    else if constexpr (std::is_same_v<T, domain::Ellipse>)
                    {
                        D2D1_ELLIPSE e = D2D1::Ellipse(
                            D2D1::Point2F(s.x + s.width / 2.f, s.y + s.height / 2.f),
                            s.width / 2.f,
                            s.height / 2.f
                        );

                        ctx.renderTarget->FillEllipse(e, brush.Get());
                        ctx.renderTarget->DrawEllipse(e, brush.Get(), 2.0f);
                    }

                }, shape);
        }

        ctx.renderTarget->EndDraw();
    }

}

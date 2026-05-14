module;

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

module ui.win32.d2d;

import <stdexcept>;
import <variant>;
import <string>;
import domain.diagram;
import ui.win32.stencil;

using Microsoft::WRL::ComPtr;

namespace ui::win32::d2d
{
    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------
    static D2D1_RECT_F canvas_from_size(UINT w, UINT h)
    {
        return D2D1::RectF(STENCIL_WIDTH,
                           0.f,
                           static_cast<float>(w),
                           static_cast<float>(h) - STATUS_BAR_HEIGHT);
    }

    // ----------------------------------------------------------------
    // create_d2d_context
    // ----------------------------------------------------------------
    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram)
    {
        D2DContext ctx{};

        // D2D factory
        D2D1_FACTORY_OPTIONS options{};
        auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory), &options,
            reinterpret_cast<void**>(ctx.factory.GetAddressOf()));
        if (FAILED(hr)) throw std::runtime_error("Failed to create D2D factory");

        // Render target
        RECT rc{};
        GetClientRect(hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = ctx.factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            ctx.renderTarget.GetAddressOf());
        if (FAILED(hr)) throw std::runtime_error("Failed to create HwndRenderTarget");

        // Dashed stroke style for lasso
        float dashes[] = { 4.f, 3.f };
        D2D1_STROKE_STYLE_PROPERTIES ssp = D2D1::StrokeStyleProperties();
        ssp.dashStyle = D2D1_DASH_STYLE_CUSTOM;
        ctx.factory->CreateStrokeStyle(ssp, dashes, 2,
            ctx.lasso_stroke_style.GetAddressOf());

        // Dashed stroke style for shape preview (6px dash / 4px gap)
        float prev_dashes[] = { 6.f, 4.f };
        D2D1_STROKE_STYLE_PROPERTIES prev_ssp = D2D1::StrokeStyleProperties();
        prev_ssp.dashStyle = D2D1_DASH_STYLE_CUSTOM;
        ctx.factory->CreateStrokeStyle(prev_ssp, prev_dashes, 2,
            ctx.preview_stroke_style.GetAddressOf());

        // DirectWrite factory
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(ctx.dwrite_factory.GetAddressOf()));
        if (FAILED(hr)) throw std::runtime_error("Failed to create DWrite factory");

        // Text format for the status bar (12 px, left-aligned)
        ctx.dwrite_factory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.f, L"en-us",
            ctx.status_text_format.GetAddressOf());

        if (ctx.status_text_format)
        {
            ctx.status_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            ctx.status_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Stencil tile label (11px, centred)
        ctx.dwrite_factory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.f, L"en-us",
            ctx.stencil_label_format.GetAddressOf());

        if (ctx.stencil_label_format)
        {
            ctx.stencil_label_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            ctx.stencil_label_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        }

        ctx.canvas_rect = canvas_from_size(size.width, size.height);
        ctx.diagram     = diagram;

        return ctx;
    }

    // ----------------------------------------------------------------
    // resize
    // ----------------------------------------------------------------
    void resize(D2DContext& ctx, UINT width, UINT height)
    {
        if (ctx.renderTarget)
        {
            ctx.renderTarget->Resize(D2D1::SizeU(width, height));
            ctx.canvas_rect = canvas_from_size(width, height);
        }
    }

    // ----------------------------------------------------------------
    // render
    // ----------------------------------------------------------------
    void render(D2DContext& ctx)
    {
        if (!ctx.renderTarget || !ctx.diagram)
            return;

        const D2D1_SIZE_F rt_size = ctx.renderTarget->GetSize();

        ctx.renderTarget->BeginDraw();
        ctx.renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        // ── Brushes ──────────────────────────────────────────────────
        ComPtr<ID2D1SolidColorBrush> shape_brush;
        ctx.renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::CornflowerBlue), shape_brush.GetAddressOf());

        ComPtr<ID2D1SolidColorBrush> sel_fill_brush;
        ctx.renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 0.55f, 0.f, 0.18f), sel_fill_brush.GetAddressOf());

        ComPtr<ID2D1SolidColorBrush> sel_stroke_brush;
        ctx.renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 0.55f, 0.f, 1.f), sel_stroke_brush.GetAddressOf());

        ComPtr<ID2D1SolidColorBrush> lasso_brush;
        ctx.renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0.f, 0.47f, 1.f, 1.f), lasso_brush.GetAddressOf());

        // ── Clip to canvas (everything above the status bar) ─────────
        ctx.renderTarget->PushAxisAlignedClip(ctx.canvas_rect,
            D2D1_ANTIALIAS_MODE_ALIASED);

        // ── 1. Draw all shapes ───────────────────────────────────────
        for (auto& shape : ctx.diagram->shapes())
        {
            std::visit([&](auto&& s)
            {
                using T = std::decay_t<decltype(s)>;

                if constexpr (std::is_same_v<T, domain::Rectangle>)
                {
                    D2D1_RECT_F r = D2D1::RectF(s.x, s.y, s.x + s.width, s.y + s.height);
                    ctx.renderTarget->FillRectangle(r, shape_brush.Get());
                    ctx.renderTarget->DrawRectangle(r, shape_brush.Get(), 2.f);
                }
                else if constexpr (std::is_same_v<T, domain::Ellipse>)
                {
                    D2D1_ELLIPSE e = D2D1::Ellipse(
                        D2D1::Point2F(s.x + s.width / 2.f, s.y + s.height / 2.f),
                        s.width / 2.f, s.height / 2.f);
                    ctx.renderTarget->FillEllipse(e, shape_brush.Get());
                    ctx.renderTarget->DrawEllipse(e, shape_brush.Get(), 2.f);
                }
            }, shape);
        }

        // ── 2. Selection highlights ──────────────────────────────────
        if (ctx.selected_shapes)
        {
            for (auto* sp : *ctx.selected_shapes)
            {
                std::visit([&](auto&& s)
                {
                    using T = std::decay_t<decltype(s)>;

                    if constexpr (std::is_same_v<T, domain::Rectangle>)
                    {
                        D2D1_RECT_F r = D2D1::RectF(s.x, s.y, s.x + s.width, s.y + s.height);
                        ctx.renderTarget->FillRectangle(r, sel_fill_brush.Get());
                        ctx.renderTarget->DrawRectangle(r, sel_stroke_brush.Get(), 2.f);
                    }
                    else if constexpr (std::is_same_v<T, domain::Ellipse>)
                    {
                        D2D1_ELLIPSE e = D2D1::Ellipse(
                            D2D1::Point2F(s.x + s.width / 2.f, s.y + s.height / 2.f),
                            s.width / 2.f, s.height / 2.f);
                        ctx.renderTarget->FillEllipse(e, sel_fill_brush.Get());
                        ctx.renderTarget->DrawEllipse(e, sel_stroke_brush.Get(), 2.f);
                    }
                }, *sp);
            }
        }

        // ── 3. Lasso overlay ─────────────────────────────────────────
        if (ctx.lasso_active && ctx.lasso_stroke_style)
        {
            float x0 = ctx.lasso_x0 < ctx.lasso_x1 ? ctx.lasso_x0 : ctx.lasso_x1;
            float y0 = ctx.lasso_y0 < ctx.lasso_y1 ? ctx.lasso_y0 : ctx.lasso_y1;
            float x1 = ctx.lasso_x0 > ctx.lasso_x1 ? ctx.lasso_x0 : ctx.lasso_x1;
            float y1 = ctx.lasso_y0 > ctx.lasso_y1 ? ctx.lasso_y0 : ctx.lasso_y1;
            D2D1_RECT_F lr = D2D1::RectF(x0, y0, x1, y1);

            ComPtr<ID2D1SolidColorBrush> lasso_fill;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.f, 0.47f, 1.f, 0.07f), lasso_fill.GetAddressOf());
            ctx.renderTarget->FillRectangle(lr, lasso_fill.Get());
            ctx.renderTarget->DrawRectangle(lr, lasso_brush.Get(), 1.f,
                ctx.lasso_stroke_style.Get());
        }

        ctx.renderTarget->PopAxisAlignedClip();

        // ── 4. Shape preview overlay (add tools) ─────────────────────
        if (ctx.preview_active && ctx.preview_stroke_style)
        {
            ComPtr<ID2D1SolidColorBrush> prev_fill;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.2f, 0.6f, 1.f, 0.15f), prev_fill.GetAddressOf());
            ComPtr<ID2D1SolidColorBrush> prev_stroke;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.2f, 0.6f, 1.f, 1.f), prev_stroke.GetAddressOf());

            const float px0 = ctx.preview_x0, py0 = ctx.preview_y0;
            const float px1 = ctx.preview_x1, py1 = ctx.preview_y1;

            if (ctx.preview_kind == StencilItemKind::Rectangle)
            {
                D2D1_RECT_F pr = D2D1::RectF(px0, py0, px1, py1);
                ctx.renderTarget->FillRectangle(pr, prev_fill.Get());
                ctx.renderTarget->DrawRectangle(pr, prev_stroke.Get(), 1.5f,
                    ctx.preview_stroke_style.Get());
            }
            else if (ctx.preview_kind == StencilItemKind::Ellipse)
            {
                D2D1_ELLIPSE pe = D2D1::Ellipse(
                    D2D1::Point2F((px0 + px1) * 0.5f, (py0 + py1) * 0.5f),
                    (px1 - px0) * 0.5f, (py1 - py0) * 0.5f);
                ctx.renderTarget->FillEllipse(pe, prev_fill.Get());
                ctx.renderTarget->DrawEllipse(pe, prev_stroke.Get(), 1.5f,
                    ctx.preview_stroke_style.Get());
            }
        }

        // ── 5. Stencil panel ─────────────────────────────────────────
        if (ctx.stencil)
        {
            const float panel_h = rt_size.height - STATUS_BAR_HEIGHT;
            D2D1_RECT_F panel_rect = D2D1::RectF(0.f, 0.f, STENCIL_WIDTH, panel_h);

            // Background
            ComPtr<ID2D1SolidColorBrush> panel_bg;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.13f, 0.13f, 0.13f, 1.f), panel_bg.GetAddressOf());
            ctx.renderTarget->FillRectangle(panel_rect, panel_bg.Get());

            // Right separator
            ComPtr<ID2D1SolidColorBrush> sep;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.30f, 0.30f, 0.30f, 1.f), sep.GetAddressOf());
            ctx.renderTarget->DrawLine(
                D2D1::Point2F(STENCIL_WIDTH, 0.f),
                D2D1::Point2F(STENCIL_WIDTH, panel_h),
                sep.Get(), 1.f);

            // "Shapes" header
            if (ctx.stencil_label_format)
            {
                ComPtr<ID2D1SolidColorBrush> header_brush;
                ctx.renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(0.55f, 0.55f, 0.55f, 1.f), header_brush.GetAddressOf());
                D2D1_RECT_F hdr = D2D1::RectF(0.f, 4.f, STENCIL_WIDTH, 18.f);
                ctx.renderTarget->DrawText(L"SHAPES", 6,
                    ctx.stencil_label_format.Get(), hdr, header_brush.Get());
            }

            // Tiles
            for (auto& item : ctx.stencil->items())
            {
                const bool selected = (ctx.stencil->active() == item.kind);

                // Tile background
                ComPtr<ID2D1SolidColorBrush> tile_bg;
                ctx.renderTarget->CreateSolidColorBrush(
                    selected ? D2D1::ColorF(0.20f, 0.45f, 0.80f, 1.f)
                             : D2D1::ColorF(0.20f, 0.20f, 0.20f, 1.f),
                    tile_bg.GetAddressOf());

                D2D1_RECT_F tile = D2D1::RectF(item.x0, item.y0, item.x1, item.y1);
                ctx.renderTarget->FillRectangle(tile, tile_bg.Get());

                // Tile border
                ComPtr<ID2D1SolidColorBrush> tile_border;
                ctx.renderTarget->CreateSolidColorBrush(
                    selected ? D2D1::ColorF(0.40f, 0.65f, 1.0f, 1.f)
                             : D2D1::ColorF(0.35f, 0.35f, 0.35f, 1.f),
                    tile_border.GetAddressOf());
                ctx.renderTarget->DrawRectangle(tile, tile_border.Get(), 1.f);

                // Mini shape preview inside tile
                const float ico_margin = 12.f;
                const float ico_x0 = item.x0 + ico_margin;
                const float ico_y0 = item.y0 + ico_margin;
                const float ico_x1 = item.x1 - ico_margin;
                const float ico_y1 = item.y1 - 20.f; // leave room for label

                ComPtr<ID2D1SolidColorBrush> ico_brush;
                ctx.renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(0.55f, 0.75f, 1.0f, 1.f), ico_brush.GetAddressOf());

                if (item.kind == StencilItemKind::Rectangle)
                {
                    D2D1_RECT_F ico = D2D1::RectF(ico_x0, ico_y0, ico_x1, ico_y1);
                    ctx.renderTarget->DrawRectangle(ico, ico_brush.Get(), 1.5f);
                }
                else if (item.kind == StencilItemKind::Ellipse)
                {
                    D2D1_ELLIPSE ico = D2D1::Ellipse(
                        D2D1::Point2F((ico_x0 + ico_x1) * 0.5f, (ico_y0 + ico_y1) * 0.5f),
                        (ico_x1 - ico_x0) * 0.5f, (ico_y1 - ico_y0) * 0.5f);
                    ctx.renderTarget->DrawEllipse(ico, ico_brush.Get(), 1.5f);
                }

                // Label
                if (ctx.stencil_label_format)
                {
                    ComPtr<ID2D1SolidColorBrush> lbl_brush;
                    ctx.renderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(0.85f, 0.85f, 0.85f, 1.f), lbl_brush.GetAddressOf());
                    D2D1_RECT_F lbl_rect = D2D1::RectF(item.x0, item.y0, item.x1, item.y1);
                    const auto* lbl = item.label;
                    ctx.renderTarget->DrawText(lbl,
                        static_cast<UINT32>(wcslen(lbl)),
                        ctx.stencil_label_format.Get(), lbl_rect, lbl_brush.Get());
                }
            }
        }

        // ── 6. Status bar ─────────────────────────────────────────────
        {
            const float bar_y = rt_size.height - STATUS_BAR_HEIGHT;
            D2D1_RECT_F bar_rect = D2D1::RectF(0.f, bar_y, rt_size.width, rt_size.height);

            // Background
            ComPtr<ID2D1SolidColorBrush> bar_bg;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.17f, 0.17f, 0.17f, 1.f), bar_bg.GetAddressOf());
            ctx.renderTarget->FillRectangle(bar_rect, bar_bg.Get());

            // Top separator line
            ComPtr<ID2D1SolidColorBrush> sep_brush;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.35f, 0.35f, 0.35f, 1.f), sep_brush.GetAddressOf());
            ctx.renderTarget->DrawLine(
                D2D1::Point2F(0.f, bar_y), D2D1::Point2F(rt_size.width, bar_y),
                sep_brush.Get(), 1.f);

            if (ctx.status_text_format)
            {
                ComPtr<ID2D1SolidColorBrush> text_brush;
                ctx.renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(0.85f, 0.85f, 0.85f, 1.f), text_brush.GetAddressOf());

                // Build status string:
                // "Tool: Select   |   Stencil: Rectangle   |   Shapes: 4   |   Selected: 1"
                std::wstring stencil_part = ctx.status_stencil.empty()
                    ? L"—" : ctx.status_stencil;

                std::wstring text =
                    L"  Tool: "    + ctx.status_tool +
                    L"   \u2502   Stencil: " + stencil_part +
                    L"   \u2502   Shapes: "  + std::to_wstring(ctx.status_shape_count) +
                    L"   \u2502   Selected: " + std::to_wstring(ctx.status_selected_count);

                // Left-side status fields
                D2D1_RECT_F text_rect = D2D1::RectF(0.f, bar_y, rt_size.width * 0.75f, rt_size.height);
                ctx.renderTarget->DrawText(
                    text.c_str(), static_cast<UINT32>(text.size()),
                    ctx.status_text_format.Get(), text_rect, text_brush.Get());

                // Right-side: cursor mode hint
                const std::wstring hint = L"S \u2192 Select   \u2502   Del \u2192 Delete  ";
                D2D1_RECT_F hint_rect = D2D1::RectF(rt_size.width * 0.75f, bar_y,
                                                     rt_size.width, rt_size.height);
                ComPtr<ID2D1SolidColorBrush> hint_brush;
                ctx.renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.f), hint_brush.GetAddressOf());

                // Right-align the hint text
                if (ctx.status_text_format)
                {
                    ctx.status_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
                    ctx.renderTarget->DrawText(
                        hint.c_str(), static_cast<UINT32>(hint.size()),
                        ctx.status_text_format.Get(), hint_rect, hint_brush.Get());
                    ctx.status_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                }
            }
        }

        ctx.renderTarget->EndDraw();
	}
}

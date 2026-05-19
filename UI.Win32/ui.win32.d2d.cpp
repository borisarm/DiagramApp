module;

#define NOMINMAX
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <cmath>
#include "../Domain/shape_interfaces.h"

module ui.win32.d2d;

import <stdexcept>;
import <string>;
import domain.diagram;

using Microsoft::WRL::ComPtr;

namespace ui::win32::d2d
{
    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------
    static D2D1_RECT_F canvas_from_size(UINT w, UINT h)
    {
        return D2D1::RectF(0.f,
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

        // Label text format — centred inside shapes, slightly larger
        ctx.dwrite_factory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            13.f, L"en-us",
            ctx.label_text_format.GetAddressOf());

        if (ctx.label_text_format)
        {
            ctx.label_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            ctx.label_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            ctx.label_text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
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

    // Helper: unpack 0xAARRGGBB into a D2D1_COLOR_F
    static D2D1_COLOR_F unpack_color(UINT32 c)
    {
        return D2D1::ColorF(
            ((c >> 16) & 0xFF) / 255.f,
            ((c >>  8) & 0xFF) / 255.f,
            ( c        & 0xFF) / 255.f,
            ((c >> 24) & 0xFF) / 255.f);
    }

    // Helper: get fill / stroke colors from IShapeProperties, with fallback.
    static void shape_colors(IShape* s,
                             D2D1_COLOR_F& fill,
                             D2D1_COLOR_F& stroke)
    {
        constexpr UINT32 k_default = 0xFF6495ED;   // cornflower blue
        fill   = unpack_color(k_default);
        stroke = unpack_color(k_default);

        IShapeProperties* props = nullptr;
        if (SUCCEEDED(s->QueryInterface(__uuidof(IShapeProperties),
                                        reinterpret_cast<void**>(&props))) && props)
        {
            UINT32 fc = k_default, sc = k_default;
            props->GetColor(L"fill_color",   &fc);
            props->GetColor(L"stroke_color", &sc);
            fill   = unpack_color(fc);
            stroke = unpack_color(sc);
            props->Release();
        }
    }

    void render(D2DContext& ctx)
    {
        if (!ctx.renderTarget || !ctx.diagram)
            return;

        const D2D1_SIZE_F rt_size = ctx.renderTarget->GetSize();

        ctx.renderTarget->BeginDraw();
        ctx.renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        // ── Brushes ──────────────────────────────────────────────────
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

        // ── Apply view transform (zoom + pan) ────────────────────────
        ctx.renderTarget->SetTransform(ctx.view.matrix());

        // ── 1. Draw all shapes ───────────────────────────────────────
        for (auto& sp : ctx.diagram->shapes())
        {
            IShape* s = sp.Get();
            ShapeGeometry g{};
            s->GetGeometry(&g);
            float x = g.x, y = g.y, w = g.width, h = g.height;

            D2D1_COLOR_F fill_col, stroke_col;
            shape_colors(s, fill_col, stroke_col);

            ComPtr<ID2D1SolidColorBrush> fill_br, stroke_br;
            ctx.renderTarget->CreateSolidColorBrush(fill_col,   fill_br.GetAddressOf());
            ctx.renderTarget->CreateSolidColorBrush(stroke_col, stroke_br.GetAddressOf());

            if (g.kind == ShapeGeometryKind::Rect)
            {
                D2D1_RECT_F r = D2D1::RectF(x, y, x + w, y + h);
                ctx.renderTarget->FillRectangle(r, fill_br.Get());
                ctx.renderTarget->DrawRectangle(r, stroke_br.Get(), 2.f);
            }
            else if (g.kind == ShapeGeometryKind::Ellipse)
            {
                D2D1_ELLIPSE e = D2D1::Ellipse(
                    D2D1::Point2F(x + w * 0.5f, y + h * 0.5f),
                    w * 0.5f, h * 0.5f);
                ctx.renderTarget->FillEllipse(e, fill_br.Get());
                ctx.renderTarget->DrawEllipse(e, stroke_br.Get(), 2.f);
            }

            // Label text
            IShapeProperties* props = nullptr;
            if (SUCCEEDED(s->QueryInterface(__uuidof(IShapeProperties),
                                            reinterpret_cast<void**>(&props))) && props)
            {
                BSTR label = nullptr;
                props->GetString(L"label", &label);
                if (label && label[0] && ctx.label_text_format)
                {
                    ComPtr<ID2D1SolidColorBrush> text_br;
                    ctx.renderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(D2D1::ColorF::Black), text_br.GetAddressOf());
                    D2D1_RECT_F tr = D2D1::RectF(x + 4.f, y + 4.f, x + w - 4.f, y + h - 4.f);
                    ctx.renderTarget->DrawText(
                        label, static_cast<UINT32>(wcslen(label)),
                        ctx.label_text_format.Get(), tr, text_br.Get());
                    SysFreeString(label);
                }
                else if (label) SysFreeString(label);
                props->Release();
            }
        }

        // ── 2. Selection highlights ──────────────────────────────────
        if (ctx.selected_shapes)
        {
            for (IShape* sp : *ctx.selected_shapes)
            {
                ShapeGeometry g{};
                sp->GetGeometry(&g);
                float x = g.x, y = g.y, w = g.width, h = g.height;

                if (g.kind == ShapeGeometryKind::Rect)
                {
                    D2D1_RECT_F r = D2D1::RectF(x, y, x + w, y + h);
                    ctx.renderTarget->FillRectangle(r, sel_fill_brush.Get());
                    ctx.renderTarget->DrawRectangle(r, sel_stroke_brush.Get(), 2.f);
                }
                else if (g.kind == ShapeGeometryKind::Ellipse)
                {
                    D2D1_ELLIPSE e = D2D1::Ellipse(
                        D2D1::Point2F(x + w * 0.5f, y + h * 0.5f),
                        w * 0.5f, h * 0.5f);
                    ctx.renderTarget->FillEllipse(e, sel_fill_brush.Get());
                    ctx.renderTarget->DrawEllipse(e, sel_stroke_brush.Get(), 2.f);
                }
            }
        }

        // ── 3. Lasso overlay (world space, drawn with view transform) ──
        ctx.renderTarget->SetTransform(ctx.view.matrix());
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

        // ── 3b. Committed connectors (world space) ───────────────────
        ctx.renderTarget->SetTransform(ctx.view.matrix());
        {
            ComPtr<ID2D1SolidColorBrush> conn_brush;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.f), conn_brush.GetAddressOf());

            const auto& shapes     = ctx.diagram->shapes();
            const auto& connectors = ctx.diagram->connectors();

            auto shape_centre = [&](std::size_t idx, float& cx, float& cy)
            {
                float bx, by, bw, bh;
                shapes[idx]->GetBounds(&bx, &by, &bw, &bh);
                cx = bx + bw * 0.5f;
                cy = by + bh * 0.5f;
            };

            for (const auto& c : connectors)
            {
                if (c.source_index >= shapes.size() || c.target_index >= shapes.size())
                    continue;

                float x0, y0, x1, y1;
                shape_centre(c.source_index, x0, y0);
                shape_centre(c.target_index, x1, y1);

                ctx.renderTarget->DrawLine(
                    D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1),
                    conn_brush.Get(), 1.5f);

                // Arrowhead at target end
                float dx = x1 - x0, dy = y1 - y0;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.01f)
                {
                    float ux = dx / len, uy = dy / len;
                    float px = -uy, py = ux;           // perpendicular
                    const float hs = 8.f;              // arrowhead size
                    float ax = x1 - ux * hs, ay = y1 - uy * hs;
                    ctx.renderTarget->DrawLine(
                        D2D1::Point2F(x1, y1),
                        D2D1::Point2F(ax + px * hs * 0.4f, ay + py * hs * 0.4f),
                        conn_brush.Get(), 1.5f);
                    ctx.renderTarget->DrawLine(
                        D2D1::Point2F(x1, y1),
                        D2D1::Point2F(ax - px * hs * 0.4f, ay - py * hs * 0.4f),
                        conn_brush.Get(), 1.5f);
                }
            }
        }

        // ── 3c. Connector preview (world space) ──────────────────────
        if (ctx.connector_preview_active)
        {
            ComPtr<ID2D1SolidColorBrush> cprev_brush;
            ctx.renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(1.f, 0.4f, 0.f, 0.85f), cprev_brush.GetAddressOf());

            ctx.renderTarget->DrawLine(
                D2D1::Point2F(ctx.connector_preview_x0, ctx.connector_preview_y0),
                D2D1::Point2F(ctx.connector_preview_x1, ctx.connector_preview_y1),
                cprev_brush.Get(), 1.5f,
                ctx.preview_stroke_style.Get());
        }

        ctx.renderTarget->PopAxisAlignedClip();
        ctx.renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

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

            if (ctx.preview_kind == ShapeGeometryKind::Rect)
            {
                D2D1_RECT_F pr = D2D1::RectF(px0, py0, px1, py1);
                ctx.renderTarget->FillRectangle(pr, prev_fill.Get());
                ctx.renderTarget->DrawRectangle(pr, prev_stroke.Get(), 1.5f,
                    ctx.preview_stroke_style.Get());
            }
            else if (ctx.preview_kind == ShapeGeometryKind::Ellipse)
            {
                D2D1_ELLIPSE pe = D2D1::Ellipse(
                    D2D1::Point2F((px0 + px1) * 0.5f, (py0 + py1) * 0.5f),
                    (px1 - px0) * 0.5f, (py1 - py0) * 0.5f);
                ctx.renderTarget->FillEllipse(pe, prev_fill.Get());
                ctx.renderTarget->DrawEllipse(pe, prev_stroke.Get(), 1.5f,
                    ctx.preview_stroke_style.Get());
            }
        }

        // ── 5. Status bar ─────────────────────────────────────────────
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
                    L"   \u2502   Selected: " + std::to_wstring(ctx.status_selected_count) +
                    L"   \u2502   Zoom: " + std::to_wstring(static_cast<int>(ctx.view.scale * 100.f)) + L"%";

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

module;

#include "../Domain/shape_interfaces.h"

export module ui.win32.d2d;

import <d2d1.h>;
import <dwrite.h>;
import <wrl/client.h>;
import <vector>;
import <string>;
import domain.diagram;

export namespace ui::win32::d2d
{
    constexpr float STATUS_BAR_HEIGHT = 26.f;

    // ----------------------------------------------------------------
    // View transform: screen = world * scale + offset
    // ----------------------------------------------------------------
    export struct ViewTransform
    {
        float scale    = 1.f;
        float offset_x = 0.f;
        float offset_y = 0.f;

        static constexpr float min_scale =  0.05f;
        static constexpr float max_scale = 20.f;

        void world_to_screen(float wx, float wy, float& sx, float& sy) const noexcept
        {
            sx = wx * scale + offset_x;
            sy = wy * scale + offset_y;
        }

        void screen_to_world(float sx, float sy, float& wx, float& wy) const noexcept
        {
            wx = (sx - offset_x) / scale;
            wy = (sy - offset_y) / scale;
        }

        // Zoom by factor, keeping the screen point (px, py) fixed
        void zoom_at(float factor, float px, float py) noexcept
        {
            const float new_scale = scale * factor;
            if (new_scale < min_scale || new_scale > max_scale) return;
            offset_x = px - (px - offset_x) * factor;
            offset_y = py - (py - offset_y) * factor;
            scale    = new_scale;
        }

        D2D1_MATRIX_3X2_F matrix() const noexcept
        {
            return D2D1::Matrix3x2F::Scale(scale, scale) *
                   D2D1::Matrix3x2F::Translation(offset_x, offset_y);
        }
    };

    struct D2DContext
    {
        Microsoft::WRL::ComPtr<ID2D1Factory>          factory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle>      lasso_stroke_style;
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle>      preview_stroke_style;

        // DirectWrite
        Microsoft::WRL::ComPtr<IDWriteFactory>        dwrite_factory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>     status_text_format;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>     label_text_format;

        const domain::Diagram*           diagram         = nullptr;
        const std::vector<IShape*>*        selected_shapes = nullptr;

        // View transform (zoom + pan)
        ViewTransform view{};

        // Lasso overlay (screen space)
        bool  lasso_active = false;
        float lasso_x0 = 0, lasso_y0 = 0;
        float lasso_x1 = 0, lasso_y1 = 0;

        // Canvas region (right of stencil, above status bar)
        D2D1_RECT_F canvas_rect{};

        // Status bar fields
        std::wstring status_tool;
        std::wstring status_stencil;
        int          status_shape_count    = 0;
        int          status_selected_count = 0;

        // Add-tool preview overlay
        bool              preview_active = false;
        ShapeGeometryKind preview_kind   = ShapeGeometryKind::Rect;
        float preview_x0 = 0, preview_y0 = 0;
        float preview_x1 = 0, preview_y1 = 0;

        // Connector preview overlay (AddConnectorTool in SourceLocked state)
        bool  connector_preview_active = false;
        float connector_preview_x0 = 0, connector_preview_y0 = 0;
        float connector_preview_x1 = 0, connector_preview_y1 = 0;
    };

    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram);
    void resize(D2DContext& ctx, UINT width, UINT height);
    void render(D2DContext& ctx);
}

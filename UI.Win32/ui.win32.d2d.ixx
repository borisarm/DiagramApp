module;



export module ui.win32.d2d;

import <d2d1.h>;
import <dwrite.h>;
import <wrl/client.h>;
import <vector>;
import <string>;
import domain.diagram;
import domain.shape;

export namespace ui::win32::d2d
{
    constexpr float STATUS_BAR_HEIGHT = 26.f;

    struct D2DContext
    {
        Microsoft::WRL::ComPtr<ID2D1Factory>          factory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle>      lasso_stroke_style;

        // DirectWrite
        Microsoft::WRL::ComPtr<IDWriteFactory>        dwrite_factory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>     status_text_format;

        const domain::Diagram*                   diagram         = nullptr;
        const std::vector<const domain::Shape*>* selected_shapes = nullptr;

        // Lasso overlay (set each frame by window.cpp)
        bool  lasso_active = false;
        float lasso_x0 = 0, lasso_y0 = 0;
        float lasso_x1 = 0, lasso_y1 = 0;

        // Canvas region (full window minus the status bar)
        D2D1_RECT_F canvas_rect{};

        // Status bar fields (set each frame by window.cpp)
        std::wstring status_tool;        // active tool name
        std::wstring status_stencil;     // currently selected stencil shape
        int          status_shape_count = 0;
        int          status_selected_count = 0;
    };

    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram);
    void resize(D2DContext& ctx, UINT width, UINT height);
    void render(D2DContext& ctx);
}

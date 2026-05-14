module;



export module ui.win32.d2d;

import <d2d1.h>;
import <wrl/client.h>;
import <vector>;
import domain.diagram;
import domain.shape;

export namespace ui::win32::d2d
{
    struct D2DContext
    {
        Microsoft::WRL::ComPtr<ID2D1Factory>          factory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;
        Microsoft::WRL::ComPtr<ID2D1StrokeStyle>      lasso_stroke_style;

        const domain::Diagram*                  diagram          = nullptr;
        const std::vector<const domain::Shape*>* selected_shapes = nullptr;

        // Lasso overlay (set each frame by window.cpp)
        bool  lasso_active = false;
        float lasso_x0 = 0, lasso_y0 = 0;
        float lasso_x1 = 0, lasso_y1 = 0;
    };

    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram);
    void resize(D2DContext& ctx, UINT width, UINT height);
    void render(D2DContext& ctx);
}

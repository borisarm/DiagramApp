module;



export module ui.win32.d2d;

import <d2d1.h>;
import <wrl/client.h>;
import domain.diagram;

export namespace ui::win32::d2d
{
    struct FakeNode
    {
        float x;
        float y;
        float width;
        float height;
    };


    struct D2DContext
    {
        Microsoft::WRL::ComPtr<ID2D1Factory> factory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;

        FakeNode node;
        const domain::Diagram* diagram = nullptr;
    };

    D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram);
    void resize(D2DContext& ctx, UINT width, UINT height);
    void render(D2DContext& ctx);
    
}

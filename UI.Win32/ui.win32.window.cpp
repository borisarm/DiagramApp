module;

#include <Windows.h>

module ui.win32.window;

import domain.diagram;
import <stdexcept>;
import <memory>;
import ui.win32.d2d;
import ui.win32.toolmanager;
import ui.win32.selecttool;
import ui.win32.addrectangletool;
import ui.win32.addellipsetool;
import ui.win32.stencil;
import ui.win32.tool;

using ui::win32::d2d::D2DContext;

namespace
{
    constexpr wchar_t CLASS_NAME[] = L"DiagramAppWindowClass";

    D2DContext g_d2d{};
    HWND g_hwnd{};
    ui::win32::ToolManager           g_tool_manager{};
    std::unique_ptr<ui::win32::SelectTool>        g_select_tool;
    std::unique_ptr<ui::win32::AddRectangleTool>  g_add_rect_tool;
    std::unique_ptr<ui::win32::AddEllipseTool>    g_add_ellipse_tool;
    ui::win32::Stencil                            g_stencil;

    // ----------------------------------------------------------------
    // Activates the add-tool that matches the chosen stencil tile and
    // deactivates the previously active tool.
    // ----------------------------------------------------------------
    void activate_tool_for_stencil(ui::win32::StencilItemKind kind)
    {
        g_tool_manager.active_tool()->on_deactivate();
        g_stencil.set_active(kind);

        if (kind == ui::win32::StencilItemKind::Rectangle)
        {
            g_add_rect_tool->reset_done();
            g_tool_manager.set_active_tool(g_add_rect_tool.get());
            g_add_rect_tool->on_activate();
        }
        else if (kind == ui::win32::StencilItemKind::Ellipse)
        {
            g_add_ellipse_tool->reset_done();
            g_tool_manager.set_active_tool(g_add_ellipse_tool.get());
            g_add_ellipse_tool->on_activate();
        }
    }

    // ----------------------------------------------------------------
    // Pushes the current selection, lasso, preview, and status fields
    // into D2DContext so render() sees a fully up-to-date snapshot.
    // ----------------------------------------------------------------
    void sync_render_state()
    {
        // Selection + lasso
        g_d2d.selected_shapes = &g_select_tool->selected();
        g_d2d.lasso_active    = g_select_tool->is_lasso_active();
        g_d2d.lasso_x0        = g_select_tool->get_lasso_x0();
        g_d2d.lasso_y0        = g_select_tool->get_lasso_y0();
        g_d2d.lasso_x1        = g_select_tool->get_lasso_x1();
        g_d2d.lasso_y1        = g_select_tool->get_lasso_y1();

        // Add-tool preview
        if (g_add_rect_tool && g_tool_manager.active_tool() == g_add_rect_tool.get()
            && g_add_rect_tool->has_preview())
        {
            g_d2d.preview_active = true;
            g_d2d.preview_kind   = ui::win32::StencilItemKind::Rectangle;
            g_d2d.preview_x0     = g_add_rect_tool->preview_x0();
            g_d2d.preview_y0     = g_add_rect_tool->preview_y0();
            g_d2d.preview_x1     = g_add_rect_tool->preview_x1();
            g_d2d.preview_y1     = g_add_rect_tool->preview_y1();
        }
        else if (g_add_ellipse_tool && g_tool_manager.active_tool() == g_add_ellipse_tool.get()
                 && g_add_ellipse_tool->has_preview())
        {
            g_d2d.preview_active = true;
            g_d2d.preview_kind   = ui::win32::StencilItemKind::Ellipse;
            g_d2d.preview_x0     = g_add_ellipse_tool->preview_x0();
            g_d2d.preview_y0     = g_add_ellipse_tool->preview_y0();
            g_d2d.preview_x1     = g_add_ellipse_tool->preview_x1();
            g_d2d.preview_y1     = g_add_ellipse_tool->preview_y1();
        }
        else
        {
            g_d2d.preview_active = false;
        }

        // Status bar
        const auto* active    = g_tool_manager.active_tool();
        g_d2d.status_tool     = active ? active->display_name() : L"—";

        switch (g_stencil.active())
        {
        case ui::win32::StencilItemKind::Rectangle: g_d2d.status_stencil = L"Rectangle"; break;
        case ui::win32::StencilItemKind::Ellipse:   g_d2d.status_stencil = L"Ellipse";   break;
        default:                                    g_d2d.status_stencil = L"";           break;
        }

        g_d2d.status_shape_count    = static_cast<int>(ui::win32::g_diagram.shapes().size());
        g_d2d.status_selected_count = static_cast<int>(g_select_tool->selected().size());
    }

    // ----------------------------------------------------------------
    // WM_* handlers
    // ----------------------------------------------------------------
    void on_create(HWND hwnd)
    {
        g_hwnd    = hwnd;
        g_d2d     = ui::win32::d2d::create_d2d_context(hwnd, &ui::win32::g_diagram);
        g_d2d.stencil = &g_stencil;

        RECT rc{};
        GetClientRect(hwnd, &rc);
        g_stencil.layout(static_cast<float>(rc.bottom - rc.top)
                         - ui::win32::d2d::STATUS_BAR_HEIGHT);
    }

    void on_size(UINT width, UINT height)
    {
        ui::win32::d2d::resize(g_d2d, width, height);
        g_stencil.layout(static_cast<float>(height)
                         - ui::win32::d2d::STATUS_BAR_HEIGHT);
    }

    void on_paint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        sync_render_state();
        ui::win32::d2d::render(g_d2d);
        EndPaint(hwnd, &ps);
    }

    void on_lbutton_down(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        const float fx = static_cast<float>(LOWORD(lParam));
        const float fy = static_cast<float>(HIWORD(lParam));

        if (fy >= g_d2d.canvas_rect.bottom) return; // status bar

        if (fx < ui::win32::STENCIL_WIDTH)
        {
            auto kind = g_stencil.hit_test(fx, fy);
            if (kind != ui::win32::StencilItemKind::None)
                activate_tool_for_stencil(kind);
            return;
        }

        ui::win32::PointerEvent e{};
        e.x     = fx;
        e.y     = fy;
        e.left  = true;
        e.shift = (wParam & MK_SHIFT)   != 0;
        e.ctrl  = (wParam & MK_CONTROL) != 0;
        SetCapture(hwnd);
        g_tool_manager.dispatch_pointer_down(e);
    }

    void on_mouse_move(WPARAM wParam, LPARAM lParam)
    {
        const float fx = static_cast<float>(LOWORD(lParam));
        const float fy = static_cast<float>(HIWORD(lParam));

        if (fy >= g_d2d.canvas_rect.bottom) return;
        if (fx < ui::win32::STENCIL_WIDTH)  return;

        ui::win32::PointerEvent e{};
        e.x     = fx;
        e.y     = fy;
        e.left  = (wParam & MK_LBUTTON)  != 0;
        e.right = (wParam & MK_RBUTTON)  != 0;
        e.shift = (wParam & MK_SHIFT)    != 0;
        e.ctrl  = (wParam & MK_CONTROL)  != 0;
        g_tool_manager.dispatch_pointer_move(e);
    }

    void on_lbutton_up(WPARAM wParam, LPARAM lParam)
    {
        const float fx = static_cast<float>(LOWORD(lParam));
        const float fy = static_cast<float>(HIWORD(lParam));

        ReleaseCapture();

        if (fy >= g_d2d.canvas_rect.bottom) return;
        if (fx < ui::win32::STENCIL_WIDTH)  return;

        ui::win32::PointerEvent e{};
        e.x     = fx;
        e.y     = fy;
        e.shift = (wParam & MK_SHIFT)   != 0;
        e.ctrl  = (wParam & MK_CONTROL) != 0;
        g_tool_manager.dispatch_pointer_up(e);

        // If an add tool just finished, revert to SelectTool
        auto* cur = g_tool_manager.active_tool();
        const bool done =
            (cur == g_add_rect_tool.get()    && g_add_rect_tool->is_done()) ||
            (cur == g_add_ellipse_tool.get() && g_add_ellipse_tool->is_done());

        if (done)
        {
            cur->on_deactivate();
            g_stencil.set_active(ui::win32::StencilItemKind::None);
            g_tool_manager.set_active_tool(g_select_tool.get());
            g_select_tool->on_activate();
        }
    }

    void on_key_down(WPARAM wParam)
    {
        ui::win32::KeyEvent e{};
        e.key   = static_cast<int>(wParam);
        e.shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
        e.ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        e.alt   = (GetKeyState(VK_MENU)    & 0x8000) != 0;
        g_tool_manager.dispatch_key_down(e);
    }

    void on_key_up(WPARAM wParam)
    {
        ui::win32::KeyEvent e{};
        e.key   = static_cast<int>(wParam);
        e.shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
        e.ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        e.alt   = (GetKeyState(VK_MENU)    & 0x8000) != 0;
        g_tool_manager.dispatch_key_up(e);
    }

    // ----------------------------------------------------------------
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:   on_create(hwnd);                        return 0;
        case WM_SIZE:     on_size(LOWORD(lParam), HIWORD(lParam)); return 0;
        case WM_PAINT:    on_paint(hwnd);                         return 0;
        case WM_DESTROY:  PostQuitMessage(0);                     return 0;

        case WM_LBUTTONDOWN: on_lbutton_down(hwnd, wParam, lParam); return 0;
        case WM_MOUSEMOVE:   on_mouse_move(wParam, lParam);         return 0;
        case WM_LBUTTONUP:   on_lbutton_up(wParam, lParam);         return 0;

        case WM_KEYDOWN: on_key_down(wParam); return 0;
        case WM_KEYUP:   on_key_up(wParam);   return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
    
namespace ui::win32
{

	domain::Diagram g_diagram; // definition

    void init_diagram()
    {
        using namespace domain;

        g_diagram.add_shape(domain::Rectangle{ 100, 100, 200, 120 });
        g_diagram.add_shape(domain::Ellipse{ 400, 200, 150, 150 });
        g_diagram.add_shape(domain::Rectangle{ 300, 50,  100, 80 });
        g_diagram.add_shape(domain::Ellipse{ 150, 300, 120, 90 });
    }

    
    int run()
    {
        init_diagram();

        g_select_tool      = std::make_unique<ui::win32::SelectTool>(g_diagram);
        g_add_rect_tool    = std::make_unique<ui::win32::AddRectangleTool>(g_diagram);
        g_add_ellipse_tool = std::make_unique<ui::win32::AddEllipseTool>(g_diagram);
        g_tool_manager.set_active_tool(g_select_tool.get());
        g_select_tool->on_activate();

        HINSTANCE hInstance = GetModuleHandle(nullptr);

        WNDCLASS wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClass(&wc))
            throw std::runtime_error("Failed to register window class");

        HWND hwnd = CreateWindowEx(
            0,
            CLASS_NAME,
            L"Diagram App",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1280, 720,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        if (!hwnd)
            throw std::runtime_error("Failed to create window");

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg{};
        while (true)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return static_cast<int>(msg.wParam);

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Idle: redraw
            InvalidateRect(g_hwnd, nullptr, FALSE);
            Sleep(16); // ~60 FPS
        }

        return static_cast<int>(msg.wParam);
    }


    
}

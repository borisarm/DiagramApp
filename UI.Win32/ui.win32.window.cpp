module;

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"
#include "../Domain/domain.shape_impls.h"

module ui.win32.window;

import domain.diagram;
import domain.shape_registry;
import <stdexcept>;
import <memory>;
import ui.win32.d2d;
import ui.win32.toolmanager;
import ui.win32.selecttool;
import ui.win32.addshapetool;
import ui.win32.addconnectortool;
import ui.win32.stencil;
import ui.win32.properties;
import ui.win32.tool;

using Microsoft::WRL::ComPtr;
using ui::win32::d2d::D2DContext;

namespace
{
	constexpr wchar_t CLASS_NAME[] = L"DiagramAppWindowClass";

	D2DContext                                  g_d2d{};
	HWND                                        g_hwnd{};
	domain::ShapeRegistry                       g_registry{};
	ui::win32::ToolManager                      g_tool_manager{};
	std::unique_ptr<ui::win32::SelectTool>      g_select_tool;
	std::unique_ptr<ui::win32::AddShapeTool>    g_add_shape_tool;
	std::unique_ptr<ui::win32::AddConnectorTool> g_add_connector_tool;
	ui::win32::StencilWindow                    g_stencil_window;
	ui::win32::PropertiesWindow                 g_properties_window;

	// Pan state (middle-mouse or Space+drag)
	bool  g_panning    = false;
	bool  g_space_down = false;
	float g_pan_last_x = 0.f;
	float g_pan_last_y = 0.f;

	// Convert a raw screen point (from WM_* lParam) to world space.
	ui::win32::PointerEvent screen_to_world_event(float sx, float sy, WPARAM wParam)
	{
		ui::win32::PointerEvent e{};
		g_d2d.view.screen_to_world(sx, sy, e.x, e.y);
		e.left  = (wParam & MK_LBUTTON)  != 0;
		e.right = (wParam & MK_RBUTTON)  != 0;
		e.shift = (wParam & MK_SHIFT)    != 0;
		e.ctrl  = (wParam & MK_CONTROL)  != 0;
		return e;
	}

	// ----------------------------------------------------------------
	// Activate the tool that corresponds to a stencil tile index.
	// ----------------------------------------------------------------
	void activate_tool_for_tile(std::size_t tile_idx)
	{
		g_tool_manager.active_tool()->on_deactivate();
		g_stencil_window.set_active(tile_idx);

		const auto& tiles = g_stencil_window.stencil().tiles();
		if (tile_idx >= tiles.size()) return;

		const auto& tile = tiles[tile_idx];

		if (tile.shape_class == nullptr)
		{
			// Connector sentinel tile
			g_add_connector_tool->reset_done();
			g_tool_manager.set_active_tool(g_add_connector_tool.get());
			g_add_connector_tool->on_activate();
		}
		else
		{
			g_add_shape_tool->set_shape_class(tile.shape_class.Get());
			g_add_shape_tool->reset_done();
			g_tool_manager.set_active_tool(g_add_shape_tool.get());
			g_add_shape_tool->on_activate();
		}
	}

	// ----------------------------------------------------------------
	// Push current tool/selection/preview state into D2DContext.
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

		// Add-shape preview
		if (g_tool_manager.active_tool() == g_add_shape_tool.get()
			&& g_add_shape_tool->has_preview())
		{
			g_d2d.preview_active = true;
			g_d2d.preview_kind   = g_add_shape_tool->preview_geometry_kind();
			g_d2d.preview_x0     = g_add_shape_tool->preview_x0();
			g_d2d.preview_y0     = g_add_shape_tool->preview_y0();
			g_d2d.preview_x1     = g_add_shape_tool->preview_x1();
			g_d2d.preview_y1     = g_add_shape_tool->preview_y1();
		}
		else
		{
			g_d2d.preview_active = false;
		}

		// Connector preview
		if (g_tool_manager.active_tool() == g_add_connector_tool.get()
			&& g_add_connector_tool->has_preview())
		{
			g_d2d.connector_preview_active = true;
			g_d2d.connector_preview_x0     = g_add_connector_tool->preview_x0();
			g_d2d.connector_preview_y0     = g_add_connector_tool->preview_y0();
			g_d2d.connector_preview_x1     = g_add_connector_tool->preview_x1();
			g_d2d.connector_preview_y1     = g_add_connector_tool->preview_y1();
		}
		else
		{
			g_d2d.connector_preview_active = false;
		}

		// Status bar
		const auto* active = g_tool_manager.active_tool();
		g_d2d.status_tool  = active ? active->display_name() : L"—";

		const auto& stencil = g_stencil_window.stencil();
		const std::size_t ai = stencil.active_index();
		if (ai != ui::win32::Stencil::npos && ai < stencil.tiles().size())
			g_d2d.status_stencil = stencil.tiles()[ai].name;
		else
			g_d2d.status_stencil = L"";

		g_d2d.status_shape_count    = static_cast<int>(ui::win32::g_diagram.shapes().size());
		g_d2d.status_selected_count = static_cast<int>(g_select_tool->selected().size());

		// Populate properties panel with the first selected shape (or clear it)
		{
			const auto& sel = g_select_tool->selected();
			IShape* shape = sel.empty() ? nullptr : sel.front();
			static IShape* s_last_populated = nullptr;
			if (shape != s_last_populated)
			{
				s_last_populated = shape;
				g_properties_window.populate(shape);
			}
		}
	}

	// ----------------------------------------------------------------
	// WM_* handlers
	// ----------------------------------------------------------------
	void on_create(HWND hwnd)
	{
		g_hwnd = hwnd;
		g_d2d  = ui::win32::d2d::create_d2d_context(hwnd, &ui::win32::g_diagram);
	}
	void on_size(UINT width, UINT height)
	{
		ui::win32::d2d::resize(g_d2d, width, height);
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

		if (fy >= g_d2d.canvas_rect.bottom) return;

		// Space + left drag = pan
		if (g_space_down)
		{
			g_panning    = true;
			g_pan_last_x = fx;
			g_pan_last_y = fy;
			SetCapture(hwnd);
			return;
		}

		SetCapture(hwnd);
		auto e = screen_to_world_event(fx, fy, wParam);
		e.left = true;
		g_tool_manager.dispatch_pointer_down(e);

		// Connector tool commits on the second pointer_down
		auto* cur = g_tool_manager.active_tool();
		if (cur == g_add_connector_tool.get() && g_add_connector_tool->is_done())
		{
			cur->on_deactivate();
			g_stencil_window.clear_active();
			g_tool_manager.set_active_tool(g_select_tool.get());
			g_select_tool->on_activate();
		}
	}

	void on_mouse_move(WPARAM wParam, LPARAM lParam)
	{
		const float fx = static_cast<float>(LOWORD(lParam));
		const float fy = static_cast<float>(HIWORD(lParam));

		if (g_panning)
		{
			g_d2d.view.offset_x += fx - g_pan_last_x;
			g_d2d.view.offset_y += fy - g_pan_last_y;
			g_pan_last_x = fx;
			g_pan_last_y = fy;
			InvalidateRect(g_hwnd, nullptr, FALSE);
			return;
		}

		if (fy >= g_d2d.canvas_rect.bottom) return;

		const auto e = screen_to_world_event(fx, fy, wParam);
		g_tool_manager.dispatch_pointer_move(e);
	}

	void on_lbutton_up(WPARAM wParam, LPARAM lParam)
	{
		const float fx = static_cast<float>(LOWORD(lParam));
		const float fy = static_cast<float>(HIWORD(lParam));

		if (g_panning)
		{
			g_panning = false;
			ReleaseCapture();
			return;
		}

		ReleaseCapture();

		if (fy >= g_d2d.canvas_rect.bottom) return;

		auto e = screen_to_world_event(fx, fy, wParam);
		g_tool_manager.dispatch_pointer_up(e);

		// If an add-shape or connector tool just finished, revert to SelectTool
		auto* cur = g_tool_manager.active_tool();
		const bool done =
			(cur == g_add_shape_tool.get()      && g_add_shape_tool->is_done()) ||
			(cur == g_add_connector_tool.get()  && g_add_connector_tool->is_done());

		if (done)
		{
			cur->on_deactivate();
			g_stencil_window.clear_active();
			g_tool_manager.set_active_tool(g_select_tool.get());
			g_select_tool->on_activate();
		}
	}

	void on_key_down(WPARAM wParam)
	{
		if (wParam == VK_SPACE) { g_space_down = true; return; }

		// Ctrl+0 – reset view
		if (wParam == '0' && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			g_d2d.view = ui::win32::d2d::ViewTransform{};
			InvalidateRect(g_hwnd, nullptr, FALSE);
			return;
		}

		// Ctrl+= / Ctrl+- or Numpad+/- – zoom in/out centred on window
		if (wParam == VK_OEM_PLUS || wParam == VK_ADD)
		{
			RECT rc{}; GetClientRect(g_hwnd, &rc);
			g_d2d.view.zoom_at(1.15f, (rc.right - rc.left) * 0.5f, (rc.bottom - rc.top) * 0.5f);
			InvalidateRect(g_hwnd, nullptr, FALSE);
			return;
		}
		if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT)
		{
			RECT rc{}; GetClientRect(g_hwnd, &rc);
			g_d2d.view.zoom_at(1.f / 1.15f, (rc.right - rc.left) * 0.5f, (rc.bottom - rc.top) * 0.5f);
			InvalidateRect(g_hwnd, nullptr, FALSE);
			return;
		}

		ui::win32::KeyEvent e{};
		e.key   = static_cast<int>(wParam);
		e.shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
		e.ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		e.alt   = (GetKeyState(VK_MENU)    & 0x8000) != 0;
		g_tool_manager.dispatch_key_down(e);
	}

	void on_key_up(WPARAM wParam)
	{
		if (wParam == VK_SPACE) { g_space_down = false; g_panning = false; return; }

		ui::win32::KeyEvent e{};
		e.key   = static_cast<int>(wParam);
		e.shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
		e.ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		e.alt   = (GetKeyState(VK_MENU)    & 0x8000) != 0;
		g_tool_manager.dispatch_key_up(e);
	}

	// ----------------------------------------------------------------
	void on_mouse_wheel(HWND hwnd, WPARAM wParam, LPARAM lParam)
	{
		// Convert cursor from screen to client coordinates
		POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(hwnd, &pt);
		const float fx = static_cast<float>(pt.x);
		const float fy = static_cast<float>(pt.y);

		const int   delta   = GET_WHEEL_DELTA_WPARAM(wParam);
		const float factor  = (delta > 0) ? 1.15f : (1.f / 1.15f);
		g_d2d.view.zoom_at(factor, fx, fy);
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	void on_mbutton_down(HWND hwnd, WPARAM /*wParam*/, LPARAM lParam)
	{
		g_panning    = true;
		g_pan_last_x = static_cast<float>(LOWORD(lParam));
		g_pan_last_y = static_cast<float>(HIWORD(lParam));
		SetCapture(hwnd);
	}

	void on_mbutton_up(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		g_panning = false;
		ReleaseCapture();
	}

	// ----------------------------------------------------------------
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CREATE:   on_create(hwnd);                         return 0;
		case WM_SIZE:     on_size(LOWORD(lParam), HIWORD(lParam)); return 0;
		case WM_PAINT:    on_paint(hwnd);                          return 0;
		case WM_DESTROY:  PostQuitMessage(0);                      return 0;

		case WM_LBUTTONDOWN: on_lbutton_down(hwnd, wParam, lParam); return 0;
		case WM_MOUSEMOVE:   on_mouse_move(wParam, lParam);          return 0;
		case WM_LBUTTONUP:   on_lbutton_up(wParam, lParam);          return 0;

		case WM_MBUTTONDOWN: on_mbutton_down(hwnd, wParam, lParam);  return 0;
		case WM_MBUTTONUP:   on_mbutton_up(wParam, lParam);          return 0;
		case WM_MOUSEWHEEL:  on_mouse_wheel(hwnd, wParam, lParam);   return 0;

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
		// Seed the diagram with a couple of built-in shapes via the registry.
		// The registry is already populated by run() before this is called.
		auto rect_class = g_registry.find_class(L"com.diagramapp.rectangle");
		auto ell_class  = g_registry.find_class(L"com.diagramapp.ellipse");

		auto make = [&](ComPtr<IShapeClass>& cls, float x, float y, float w, float h)
		{
			if (!cls) return;
			ComPtr<IShape> s;
			cls->CreateShape(x, y, w, h, s.GetAddressOf());
			if (s) g_diagram.add_shape(std::move(s));
		};

		make(rect_class, 100.f, 100.f, 200.f, 120.f);
		make(ell_class,  400.f, 200.f, 150.f, 150.f);
		make(rect_class, 300.f,  50.f, 100.f,  80.f);
		make(ell_class,  150.f, 300.f, 120.f,  90.f);
	}

	int run()
	{
		// ── 1. Register built-in shape plugins ───────────────────────
		{
			IShapePlugin* p = nullptr;
			builtin_rectangle_plugin(&p);
			if (p) { g_registry.register_plugin(p); p->Release(); }
		}
		{
			IShapePlugin* p = nullptr;
			builtin_ellipse_plugin(&p);
			if (p) { g_registry.register_plugin(p); p->Release(); }
		}

		// ── 2. Create tools ──────────────────────────────────────────
		g_select_tool        = std::make_unique<ui::win32::SelectTool>(g_diagram);
		g_add_shape_tool     = std::make_unique<ui::win32::AddShapeTool>(g_diagram, nullptr);
		g_add_connector_tool = std::make_unique<ui::win32::AddConnectorTool>(g_diagram);
		g_tool_manager.set_active_tool(g_select_tool.get());
		g_select_tool->on_activate();

		// ── 3. Seed diagram ──────────────────────────────────────────
		init_diagram();

		// ── 4. Create main window ────────────────────────────────────
		HINSTANCE hInstance = GetModuleHandle(nullptr);

		WNDCLASS wc{};
		wc.lpfnWndProc   = WndProc;
		wc.hInstance     = hInstance;
		wc.lpszClassName = CLASS_NAME;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

		if (!RegisterClass(&wc))
			throw std::runtime_error("Failed to register window class");

		HWND hwnd = CreateWindowEx(
			0, CLASS_NAME, L"Diagram App", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
			nullptr, nullptr, hInstance, nullptr);

		if (!hwnd)
			throw std::runtime_error("Failed to create window");

		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);

		// ── 5. Create stencil palette (after ShowWindow) ─────────────
		{
			RECT wr{};
			GetWindowRect(hwnd, &wr);
			g_stencil_window.on_tile_click = activate_tool_for_tile;
			g_stencil_window.create(hwnd, wr.left - 130, wr.top);
			g_stencil_window.populate(g_registry.classes());
		}

		// ── 6. Create properties panel (after ShowWindow) ────────────
		{
			RECT wr{};
			GetWindowRect(hwnd, &wr);
			g_properties_window.create(hwnd, wr.right + 10, wr.top);
			g_properties_window.on_commit = []()
			{
				// User edited a property → commit a diagram snapshot for undo
				ui::win32::g_diagram.commit();
				InvalidateRect(g_hwnd, nullptr, FALSE);
			};
		}

		// ── 6. Message pump ──────────────────────────────────────────
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

			InvalidateRect(g_hwnd, nullptr, FALSE);
			Sleep(16);
		}

		return static_cast<int>(msg.wParam);
	}
}

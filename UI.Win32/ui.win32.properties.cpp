module;

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include "../Domain/shape_interfaces.h"

module ui.win32.properties;

import <string>;
import <vector>;
import <cstdio>;
import <cwchar>;

namespace ui::win32
{
	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------
	static float parse_float(HWND edit)
	{
		wchar_t buf[64] = {};
		GetWindowText(edit, buf, 64);
		return static_cast<float>(_wtof(buf));
	}

	static int parse_int(HWND edit)
	{
		wchar_t buf[16] = {};
		GetWindowText(edit, buf, 16);
		return static_cast<int>(_wtoi(buf));
	}

	static UINT32 pack_color(int r, int g, int b)
	{
		auto clamp = [](int v) -> UINT32 { return static_cast<UINT32>(v < 0 ? 0 : v > 255 ? 255 : v); };
		return 0xFF000000u | (clamp(r) << 16) | (clamp(g) << 8) | clamp(b);
	}

	static HFONT make_label_font()
	{
		return CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
						  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
						  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
	}

	// -----------------------------------------------------------------------
	// WndProc trampoline
	// -----------------------------------------------------------------------
	LRESULT CALLBACK PropertiesWindow::WndProc(HWND hwnd, UINT msg,
											   WPARAM wParam, LPARAM lParam)
	{
		PropertiesWindow* self = nullptr;
		if (msg == WM_NCCREATE)
		{
			auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			self = reinterpret_cast<PropertiesWindow*>(cs->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		else
		{
			self = reinterpret_cast<PropertiesWindow*>(
				GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (self) return self->handle_message(hwnd, msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	// -----------------------------------------------------------------------
	// create / destroy
	// -----------------------------------------------------------------------
	void PropertiesWindow::create(HWND owner, int x, int y)
	{
		HINSTANCE hInst = GetModuleHandle(nullptr);

		WNDCLASS wc{};
		wc.lpfnWndProc   = WndProc;
		wc.hInstance     = hInst;
		wc.lpszClassName = CLASS_NAME;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			WS_EX_TOOLWINDOW, CLASS_NAME, L"Properties",
			WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
			x, y, 260, 200, owner, nullptr, hInst, this);

		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);
	}

	void PropertiesWindow::destroy()
	{
		if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
	}

	// -----------------------------------------------------------------------
	// populate — read property descriptors + current values from shape
	// -----------------------------------------------------------------------
	void PropertiesWindow::populate(IShape* shape)
	{
		m_shape = shape;
		m_rows.clear();
		destroy_controls();

		if (!shape)
		{
			SetWindowText(m_hwnd, L"Properties");
			InvalidateRect(m_hwnd, nullptr, TRUE);
			on_resize();
			return;
		}

		// QI for IShapeProperties
		IShapeProperties* props = nullptr;
		shape->QueryInterface(__uuidof(IShapeProperties),
							  reinterpret_cast<void**>(&props));

		if (!props)
		{
			SetWindowText(m_hwnd, L"Properties (none)");
			on_resize();
			return;
		}

		UINT32 count = 0;
		props->GetPropertyCount(&count);

		for (UINT32 i = 0; i < count; ++i)
		{
			ShapePropertyDescriptor desc{};
			if (FAILED(props->GetDescriptor(i, &desc))) continue;

			PropertyRow row;
			row.key       = desc.key;
			row.label     = desc.label;
			row.type      = desc.type;
			row.category  = desc.category;
			row.read_only = desc.read_only != FALSE;

			if (desc.type == ShapePropertyType::Float)
			{
				float v = 0.f;
				props->GetFloat(desc.key, &v);
				wchar_t buf[32];
				swprintf_s(buf, L"%.2f", v);
				row.text = buf;
			}
			else if (desc.type == ShapePropertyType::Color)
			{
				UINT32 c = 0;
				props->GetColor(desc.key, &c);
				row.color_value = c;
			}
			else if (desc.type == ShapePropertyType::String)
			{
				BSTR s = nullptr;
				props->GetString(desc.key, &s);
				if (s) { row.text = s; SysFreeString(s); }
			}

			m_rows.push_back(std::move(row));
		}

		props->Release();

		// Update title with shape class name
		IShapeClass* cls = nullptr;
		shape->GetShapeClass(&cls);
		if (cls)
		{
			BSTR name = nullptr;
			cls->GetName(&name);
			if (name)
			{
				std::wstring title = std::wstring(L"Properties — ") + name;
				SetWindowText(m_hwnd, title.c_str());
				SysFreeString(name);
			}
			cls->Release();
		}

		rebuild_controls();
		on_resize();
	}

	void PropertiesWindow::clear()
	{
		populate(nullptr);
	}

	// -----------------------------------------------------------------------
	// rebuild_controls — create child EDIT controls for each row
	// -----------------------------------------------------------------------
	void PropertiesWindow::destroy_controls()
	{
		for (auto& row : m_rows)
		{
			if (row.edit)   { DestroyWindow(row.edit);   row.edit = nullptr; }
			if (row.edit_r) { DestroyWindow(row.edit_r); row.edit_r = nullptr; }
			if (row.edit_g) { DestroyWindow(row.edit_g); row.edit_g = nullptr; }
			if (row.edit_b) { DestroyWindow(row.edit_b); row.edit_b = nullptr; }
		}
	}

	void PropertiesWindow::rebuild_controls()
	{
		if (!m_hwnd) return;
		HINSTANCE hInst = GetModuleHandle(nullptr);
		HFONT font = make_label_font();

		RECT client{};
		GetClientRect(m_hwnd, &client);
		int w = client.right - client.left;

		int y = MARGIN;
		for (auto& row : m_rows)
		{
			int edit_x = LABEL_W + MARGIN;
			int edit_w = w - edit_x - MARGIN;

			if (row.type == ShapePropertyType::Color)
			{
				// Three narrow EDIT boxes: R G B
				int box_w = (edit_w - SWATCH_W - 2 * MARGIN) / 3;
				UINT32 c = row.color_value;
				int r = (c >> 16) & 0xFF;
				int g = (c >>  8) & 0xFF;
				int b =  c        & 0xFF;

				auto make_rgb = [&](int val, int ox) -> HWND {
					wchar_t buf[8];
					swprintf_s(buf, L"%d", val);
					DWORD style = WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_BORDER;
					if (row.read_only) style |= ES_READONLY;
					HWND h = CreateWindowEx(0, L"EDIT", buf, style,
						edit_x + ox, y, box_w, ROW_H - 2,
						m_hwnd, nullptr, hInst, nullptr);
					SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
					return h;
				};
				row.edit_r = make_rgb(r, 0);
				row.edit_g = make_rgb(g, box_w + MARGIN);
				row.edit_b = make_rgb(b, (box_w + MARGIN) * 2);
			}
			else
			{
				DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
				if (row.read_only) style |= ES_READONLY;
				row.edit = CreateWindowEx(0, L"EDIT", row.text.c_str(), style,
					edit_x, y, edit_w, ROW_H - 2,
					m_hwnd, nullptr, hInst, nullptr);
				SendMessage(row.edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
			}
			y += ROW_H;
		}
		DeleteObject(font);
	}

	// -----------------------------------------------------------------------
	// on_resize — reposition controls to match current client width
	// -----------------------------------------------------------------------
	void PropertiesWindow::on_resize()
	{
		if (!m_hwnd) return;

		RECT client{};
		GetClientRect(m_hwnd, &client);
		int w = client.right;

		// Required client height
		int needed_h = MARGIN + static_cast<int>(m_rows.size()) * ROW_H + MARGIN;
		if (needed_h < 40) needed_h = 40;

		// Resize window so it fits all rows (keep current width)
		RECT wr{};
		GetWindowRect(m_hwnd, &wr);

		RECT adjusted{ 0, 0, w, needed_h };   // w = current client width
		AdjustWindowRectEx(&adjusted, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
						   FALSE, WS_EX_TOOLWINDOW);
		SetWindowPos(m_hwnd, nullptr, 0, 0,
					 wr.right - wr.left,                       // keep current window width
					 adjusted.bottom - adjusted.top,
					 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		// Re-fetch client after resize
		GetClientRect(m_hwnd, &client);
		w = client.right;

		int y = MARGIN;
		for (auto& row : m_rows)
		{
			int edit_x = LABEL_W + MARGIN;
			int edit_w = w - edit_x - MARGIN;

			if (row.type == ShapePropertyType::Color)
			{
				int box_w = (edit_w - SWATCH_W - 2 * MARGIN) / 3;
				if (row.edit_r) SetWindowPos(row.edit_r, nullptr, edit_x, y, box_w, ROW_H - 2, SWP_NOZORDER);
				if (row.edit_g) SetWindowPos(row.edit_g, nullptr, edit_x + box_w + MARGIN, y, box_w, ROW_H - 2, SWP_NOZORDER);
				if (row.edit_b) SetWindowPos(row.edit_b, nullptr, edit_x + (box_w + MARGIN) * 2, y, box_w, ROW_H - 2, SWP_NOZORDER);
			}
			else
			{
				if (row.edit) SetWindowPos(row.edit, nullptr, edit_x, y, edit_w, ROW_H - 2, SWP_NOZORDER);
			}
			y += ROW_H;
		}
		InvalidateRect(m_hwnd, nullptr, TRUE);
	}

	// -----------------------------------------------------------------------
	// flush_to_shape — read EDIT controls → IShapeProperties → on_commit
	// -----------------------------------------------------------------------
	void PropertiesWindow::flush_row(PropertyRow& row)
	{
		if (!m_shape || row.read_only) return;

		IShapeProperties* props = nullptr;
		m_shape->QueryInterface(__uuidof(IShapeProperties),
								reinterpret_cast<void**>(&props));
		if (!props) return;

		if (row.type == ShapePropertyType::Float && row.edit)
		{
			float v = parse_float(row.edit);
			props->SetFloat(row.key.c_str(), v);
		}
		else if (row.type == ShapePropertyType::Color &&
				 row.edit_r && row.edit_g && row.edit_b)
		{
			UINT32 c = pack_color(parse_int(row.edit_r),
								  parse_int(row.edit_g),
								  parse_int(row.edit_b));
			row.color_value = c;
			props->SetColor(row.key.c_str(), c);
			InvalidateRect(m_hwnd, nullptr, FALSE);   // redraw swatch
		}
		else if (row.type == ShapePropertyType::String && row.edit)
		{
			wchar_t buf[512] = {};
			GetWindowText(row.edit, buf, 512);
			props->SetString(row.key.c_str(), buf);
		}

		props->Release();
	}

	void PropertiesWindow::flush_to_shape()
	{
		for (auto& row : m_rows)
			flush_row(row);
		if (on_commit) on_commit();
	}

	// -----------------------------------------------------------------------
	// paint — draw row labels + color swatches
	// -----------------------------------------------------------------------
	void PropertiesWindow::paint(HWND hwnd)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT client{};
		GetClientRect(hwnd, &client);

		HFONT font = make_label_font();
		HFONT old_font = reinterpret_cast<HFONT>(SelectObject(hdc, font));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

		int y = MARGIN;
		for (auto& row : m_rows)
		{
			// Label
			RECT lr{ MARGIN, y, LABEL_W, y + ROW_H - 2 };
			DrawText(hdc, row.label.c_str(), -1, &lr,
					 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

			// Color swatch (right of the RGB boxes)
			if (row.type == ShapePropertyType::Color)
			{
				int edit_x = LABEL_W + MARGIN;
				RECT cr{ client.right - MARGIN - SWATCH_W, y + 2,
						 client.right - MARGIN,             y + ROW_H - 4 };
				UINT32 c = row.color_value;
				COLORREF cr_col = RGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
				HBRUSH br = CreateSolidBrush(cr_col);
				FillRect(hdc, &cr, br);
				DeleteObject(br);
				FrameRect(hdc, &cr, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
			}

			// Separator line
			HPEN pen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
			HPEN old_pen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
			MoveToEx(hdc, 0, y + ROW_H - 1, nullptr);
			LineTo(hdc, client.right, y + ROW_H - 1);
			SelectObject(hdc, old_pen);
			DeleteObject(pen);

			y += ROW_H;
		}

		SelectObject(hdc, old_font);
		DeleteObject(font);
		EndPaint(hwnd, &ps);
	}

	// -----------------------------------------------------------------------
	// handle_message
	// -----------------------------------------------------------------------
	LRESULT PropertiesWindow::handle_message(HWND hwnd, UINT msg,
											 WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_SIZE:
			on_resize();
			return 0;

		case WM_PAINT:
			paint(hwnd);
			return 0;

		case WM_COMMAND:
		{
			// EN_KILLFOCUS on any child EDIT → flush that row
			if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				HWND edit_hwnd = reinterpret_cast<HWND>(lParam);
				for (auto& row : m_rows)
				{
					if (row.edit   == edit_hwnd ||
						row.edit_r == edit_hwnd ||
						row.edit_g == edit_hwnd ||
						row.edit_b == edit_hwnd)
					{
						flush_row(row);
						if (on_commit) on_commit();
						break;
					}
				}
			}
			// EN_CHANGE for color rows → update swatch live
			if (HIWORD(wParam) == EN_CHANGE)
			{
				HWND edit_hwnd = reinterpret_cast<HWND>(lParam);
				for (auto& row : m_rows)
				{
					if (row.type == ShapePropertyType::Color &&
						(row.edit_r == edit_hwnd ||
						 row.edit_g == edit_hwnd ||
						 row.edit_b == edit_hwnd))
					{
						UINT32 c = pack_color(
							row.edit_r ? parse_int(row.edit_r) : 0,
							row.edit_g ? parse_int(row.edit_g) : 0,
							row.edit_b ? parse_int(row.edit_b) : 0);
						row.color_value = c;
						InvalidateRect(hwnd, nullptr, FALSE);
						break;
					}
				}
			}
			return 0;
		}

		case WM_KEYDOWN:
			if (wParam == VK_RETURN)
			{
				flush_to_shape();
				return 0;
			}
			break;

		case WM_CLOSE:
			ShowWindow(hwnd, SW_HIDE);
			return 0;

		case WM_DESTROY:
			destroy_controls();
			m_hwnd = nullptr;
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

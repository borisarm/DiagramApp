module;

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

module ui.win32.stencil;

import <stdexcept>;

using Microsoft::WRL::ComPtr;

namespace ui::win32
{
	// ================================================================
	// Stencil (model)
	// ================================================================
	Stencil::Stencil()
	{
		m_items =
		{
			{ StencilItemKind::Rectangle, L"Rectangle" },
			{ StencilItemKind::Ellipse,   L"Ellipse"   },
		};
	}

	void Stencil::layout(float panel_height)
	{
		float y = STENCIL_PADDING + 20;
		for (auto& item : m_items)
		{
			item.x0 = STENCIL_PADDING;
			item.y0 = y;
			item.x1 = STENCIL_WIDTH - STENCIL_PADDING;
			item.y1 = y + STENCIL_TILE_H - STENCIL_PADDING;
			y += STENCIL_TILE_H;

			if (y > panel_height) break;
		}
	}

	StencilItemKind Stencil::hit_test(float px, float py) const
	{
		for (auto& item : m_items)
		{
			if (px >= item.x0 && px <= item.x1 &&
				py >= item.y0 && py <= item.y1)
				return item.kind;
		}
		return StencilItemKind::None;
	}

	// ================================================================
	// StencilWindow
	// ================================================================

	// Static WndProc — routes to the instance via GWLP_USERDATA
	LRESULT CALLBACK StencilWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		StencilWindow* self = nullptr;

		if (msg == WM_NCCREATE)
		{
			auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			self = reinterpret_cast<StencilWindow*>(cs->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		else
		{
			self = reinterpret_cast<StencilWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (self)
			return self->handle_message(hwnd, msg, wParam, lParam);

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void StencilWindow::create(HWND owner, int x, int y)
	{
		HINSTANCE hInst = GetModuleHandle(nullptr);

		// Register once
		WNDCLASS wc{};
		wc.lpfnWndProc   = WndProc;
		wc.hInstance     = hInst;
		wc.lpszClassName = CLASS_NAME;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		RegisterClass(&wc); // ignored if already registered

		const int w = static_cast<int>(STENCIL_WIDTH + 2 * STENCIL_PADDING);
		const int h = static_cast<int>(20 + static_cast<float>(m_stencil.items().size())
											* STENCIL_TILE_H + STENCIL_PADDING);

		m_hwnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			CLASS_NAME,
			L"Shapes",
			WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
			x, y, w, h,
			owner,
			nullptr,
			hInst,
			this);  // passed as lpCreateParams → caught in WM_NCCREATE

		if (!m_hwnd)
			throw std::runtime_error("Failed to create stencil palette window");

		init_d2d();

		RECT client{};
		GetClientRect(m_hwnd, &client);
		m_stencil.layout(static_cast<float>(client.bottom - client.top));

		ShowWindow(m_hwnd, SW_SHOW);
		UpdateWindow(m_hwnd);
	}

	void StencilWindow::destroy()
	{
		if (m_hwnd)
		{
			DestroyWindow(m_hwnd);
			m_hwnd = nullptr;
		}
	}

	void StencilWindow::set_active(StencilItemKind kind)
	{
		m_stencil.set_active(kind);
		if (m_hwnd)
			InvalidateRect(m_hwnd, nullptr, FALSE);
	}

	void StencilWindow::init_d2d()
	{
		D2D1_FACTORY_OPTIONS opts{};
		auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory), &opts,
			reinterpret_cast<void**>(m_factory.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Stencil: D2D factory failed");

		RECT rc{};
		GetClientRect(m_hwnd, &rc);
		D2D1_SIZE_U sz = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		hr = m_factory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, sz),
			m_rt.GetAddressOf());
		if (FAILED(hr)) throw std::runtime_error("Stencil: render target failed");

		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(m_dwrite.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Stencil: DWrite factory failed");

		m_dwrite->CreateTextFormat(
			L"Segoe UI", nullptr,
			DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			11.f, L"en-us",
			m_label_fmt.GetAddressOf());

		if (m_label_fmt)
		{
			m_label_fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			m_label_fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
		}
	}

	void StencilWindow::paint()
	{
		if (!m_rt) return;

		const D2D1_SIZE_F sz = m_rt->GetSize();

		m_rt->BeginDraw();
		m_rt->Clear(D2D1::ColorF(0.13f, 0.13f, 0.13f));

		// "SHAPES" header
		if (m_label_fmt)
		{
			ComPtr<ID2D1SolidColorBrush> hdr_brush;
			m_rt->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.55f, 0.55f), hdr_brush.GetAddressOf());
			D2D1_RECT_F hdr = D2D1::RectF(0.f, 4.f, sz.width, 18.f);
			m_rt->DrawText(L"SHAPES", 6, m_label_fmt.Get(), hdr, hdr_brush.Get());
		}

		// Tiles
		for (auto& item : m_stencil.items())
		{
			const bool selected = (m_stencil.active() == item.kind);

			ComPtr<ID2D1SolidColorBrush> tile_bg;
			m_rt->CreateSolidColorBrush(
				selected ? D2D1::ColorF(0.20f, 0.45f, 0.80f)
						 : D2D1::ColorF(0.20f, 0.20f, 0.20f),
				tile_bg.GetAddressOf());

			D2D1_RECT_F tile = D2D1::RectF(item.x0, item.y0, item.x1, item.y1);
			m_rt->FillRectangle(tile, tile_bg.Get());

			ComPtr<ID2D1SolidColorBrush> tile_border;
			m_rt->CreateSolidColorBrush(
				selected ? D2D1::ColorF(0.40f, 0.65f, 1.0f)
						 : D2D1::ColorF(0.35f, 0.35f, 0.35f),
				tile_border.GetAddressOf());
			m_rt->DrawRectangle(tile, tile_border.Get(), 1.f);

			// Mini shape icon
			const float ico_margin = 12.f;
			const float ico_x0 = item.x0 + ico_margin;
			const float ico_y0 = item.y0 + ico_margin;
			const float ico_x1 = item.x1 - ico_margin;
			const float ico_y1 = item.y1 - 20.f;

			ComPtr<ID2D1SolidColorBrush> ico_brush;
			m_rt->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.75f, 1.0f), ico_brush.GetAddressOf());

			if (item.kind == StencilItemKind::Rectangle)
			{
				m_rt->DrawRectangle(D2D1::RectF(ico_x0, ico_y0, ico_x1, ico_y1),
									ico_brush.Get(), 1.5f);
			}
			else if (item.kind == StencilItemKind::Ellipse)
			{
				D2D1_ELLIPSE ico = D2D1::Ellipse(
					D2D1::Point2F((ico_x0 + ico_x1) * 0.5f, (ico_y0 + ico_y1) * 0.5f),
					(ico_x1 - ico_x0) * 0.5f, (ico_y1 - ico_y0) * 0.5f);
				m_rt->DrawEllipse(ico, ico_brush.Get(), 1.5f);
			}

			// Label
			if (m_label_fmt)
			{
				ComPtr<ID2D1SolidColorBrush> lbl_brush;
				m_rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f), lbl_brush.GetAddressOf());
				m_rt->DrawText(item.label, static_cast<UINT32>(wcslen(item.label)),
							   m_label_fmt.Get(), tile, lbl_brush.Get());
			}
		}

		m_rt->EndDraw();
	}

	LRESULT StencilWindow::handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_SIZE:
		{
			const UINT w = LOWORD(lParam);
			const UINT h = HIWORD(lParam);
			if (m_rt)
				m_rt->Resize(D2D1::SizeU(w, h));
			m_stencil.layout(static_cast<float>(h));
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			paint();
			EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			const float fx = static_cast<float>(LOWORD(lParam));
			const float fy = static_cast<float>(HIWORD(lParam));
			auto kind = m_stencil.hit_test(fx, fy);
			if (kind != StencilItemKind::None && on_tile_click)
				on_tile_click(kind);
			return 0;
		}

		case WM_CLOSE:
			// Hide instead of destroying so the user can re-open later
			ShowWindow(hwnd, SW_HIDE);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

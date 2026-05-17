module;

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

module ui.win32.stencil;

import <stdexcept>;
import <cmath>;

using Microsoft::WRL::ComPtr;

namespace ui::win32
{
	// ================================================================
	// Stencil (model)
	// ================================================================

	void Stencil::populate(const std::vector<ComPtr<IShapeClass>>& classes)
	{
		m_tiles.clear();
		m_active = npos;
		for (auto& cls : classes)
		{
			StencilTile tile;
			tile.shape_class = cls;
			BSTR name = nullptr;
			if (SUCCEEDED(cls->GetName(&name)) && name)
			{
				tile.name = name;
				SysFreeString(name);
			}
			m_tiles.push_back(std::move(tile));
		}
	}

	void Stencil::add_connector_tile()
	{
		StencilTile tile;
		tile.shape_class = nullptr;
		tile.name = L"Connector";
		m_tiles.push_back(std::move(tile));
	}

	void Stencil::layout(float panel_height)
	{
		float y = STENCIL_PADDING + 20.f;
		for (auto& tile : m_tiles)
		{
			tile.x0 = STENCIL_PADDING;
			tile.y0 = y;
			tile.x1 = STENCIL_WIDTH - STENCIL_PADDING;
			tile.y1 = y + STENCIL_TILE_H - STENCIL_PADDING;
			y += STENCIL_TILE_H;
			if (y > panel_height) break;
		}
	}

	std::size_t Stencil::hit_test(float px, float py) const
	{
		for (std::size_t i = 0; i < m_tiles.size(); ++i)
		{
			const auto& t = m_tiles[i];
			if (px >= t.x0 && px <= t.x1 && py >= t.y0 && py <= t.y1)
				return i;
		}
		return npos;
	}

	// ================================================================
	// StencilWindow
	// ================================================================

	LRESULT CALLBACK StencilWindow::WndProc(HWND hwnd, UINT msg,
											WPARAM wParam, LPARAM lParam)
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
			self = reinterpret_cast<StencilWindow*>(
				GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (self) return self->handle_message(hwnd, msg, wParam, lParam);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void StencilWindow::create(HWND owner, int x, int y)
	{
		HINSTANCE hInst = GetModuleHandle(nullptr);

		WNDCLASS wc{};
		wc.lpfnWndProc   = WndProc;
		wc.hInstance     = hInst;
		wc.lpszClassName = CLASS_NAME;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		RegisterClass(&wc);  // ignored if already registered

		const int w = static_cast<int>(STENCIL_WIDTH + 2.f * STENCIL_PADDING);
		const int h = static_cast<int>(20.f
			+ static_cast<float>(m_stencil.tiles().size()) * STENCIL_TILE_H
			+ STENCIL_PADDING);

		m_hwnd = CreateWindowEx(
			WS_EX_TOOLWINDOW, CLASS_NAME, L"Shapes",
			WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
			x, y, w, h, owner, nullptr, hInst, this);

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
		if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
	}

	void StencilWindow::populate(const std::vector<ComPtr<IShapeClass>>& classes)
	{
		m_stencil.populate(classes);
		m_stencil.add_connector_tile();
		if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
	}

	void StencilWindow::set_active(std::size_t tile_index)
	{
		m_stencil.set_active(tile_index);
		if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
	}

	void StencilWindow::clear_active()
	{
		m_stencil.clear_active();
		if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
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
			DWRITE_FONT_STRETCH_NORMAL, 11.f, L"en-us",
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

		// Header
		if (m_label_fmt)
		{
			ComPtr<ID2D1SolidColorBrush> hdr;
			m_rt->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.55f, 0.55f),
										hdr.GetAddressOf());
			m_rt->DrawText(L"SHAPES", 6, m_label_fmt.Get(),
						   D2D1::RectF(0.f, 4.f, sz.width, 18.f), hdr.Get());
		}

		for (std::size_t i = 0; i < m_stencil.tiles().size(); ++i)
		{
			const auto& tile     = m_stencil.tiles()[i];
			const bool  selected = (m_stencil.active_index() == i);

			// Tile background
			ComPtr<ID2D1SolidColorBrush> bg;
			m_rt->CreateSolidColorBrush(
				selected ? D2D1::ColorF(0.20f, 0.45f, 0.80f)
						 : D2D1::ColorF(0.20f, 0.20f, 0.20f),
				bg.GetAddressOf());
			D2D1_RECT_F tr = D2D1::RectF(tile.x0, tile.y0, tile.x1, tile.y1);
			m_rt->FillRectangle(tr, bg.Get());

			ComPtr<ID2D1SolidColorBrush> border;
			m_rt->CreateSolidColorBrush(
				selected ? D2D1::ColorF(0.40f, 0.65f, 1.0f)
						 : D2D1::ColorF(0.35f, 0.35f, 0.35f),
				border.GetAddressOf());
			m_rt->DrawRectangle(tr, border.Get(), 1.f);

			// Icon
			const float im = 12.f;
			const float ix0 = tile.x0 + im, iy0 = tile.y0 + im;
			const float ix1 = tile.x1 - im, iy1 = tile.y1 - 20.f;

			ComPtr<ID2D1SolidColorBrush> ico;
			m_rt->CreateSolidColorBrush(D2D1::ColorF(0.55f, 0.75f, 1.0f),
										ico.GetAddressOf());

			if (tile.shape_class)
			{
				ShapeGeometry geom{};
				tile.shape_class->GetStencilGeometry(&geom);

				// Scale the canonical stencil geometry into the icon area
				const float gw = geom.width  > 0 ? geom.width  : 1.f;
				const float gh = geom.height > 0 ? geom.height : 1.f;
				const float sx = (ix1 - ix0) / gw;
				const float sy = (iy1 - iy0) / gh;
				const float ox = ix0 - geom.x * sx;
				const float oy = iy0 - geom.y * sy;

				auto map = [&](float gx, float gy, D2D1_POINT_2F& p)
				{
					p = D2D1::Point2F(ox + gx * sx, oy + gy * sy);
				};

				if (geom.kind == ShapeGeometryKind::Rect)
				{
					m_rt->DrawRectangle(D2D1::RectF(ix0, iy0, ix1, iy1),
										ico.Get(), 1.5f);
				}
				else if (geom.kind == ShapeGeometryKind::Ellipse)
				{
					D2D1_ELLIPSE e = D2D1::Ellipse(
						D2D1::Point2F((ix0 + ix1) * 0.5f, (iy0 + iy1) * 0.5f),
						(ix1 - ix0) * 0.5f, (iy1 - iy0) * 0.5f);
					m_rt->DrawEllipse(e, ico.Get(), 1.5f);
				}
			}
			else
			{
				// Connector sentinel — diagonal arrow
				m_rt->DrawLine(D2D1::Point2F(ix0, iy1),
							   D2D1::Point2F(ix1, iy0), ico.Get(), 1.5f);
				m_rt->DrawLine(D2D1::Point2F(ix1, iy0),
							   D2D1::Point2F(ix1 - 8.f, iy0 + 3.f), ico.Get(), 1.5f);
				m_rt->DrawLine(D2D1::Point2F(ix1, iy0),
							   D2D1::Point2F(ix1 - 3.f, iy0 + 8.f), ico.Get(), 1.5f);
			}

			// Label
			if (m_label_fmt && !tile.name.empty())
			{
				ComPtr<ID2D1SolidColorBrush> lbl;
				m_rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f),
											lbl.GetAddressOf());
				m_rt->DrawText(tile.name.c_str(),
							   static_cast<UINT32>(tile.name.size()),
							   m_label_fmt.Get(), tr, lbl.Get());
			}
		}

		m_rt->EndDraw();
	}

	LRESULT StencilWindow::handle_message(HWND hwnd, UINT msg,
										  WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_SIZE:
		{
			const UINT w = LOWORD(lParam), h = HIWORD(lParam);
			if (m_rt) m_rt->Resize(D2D1::SizeU(w, h));
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
			std::size_t idx = m_stencil.hit_test(fx, fy);
			if (idx != Stencil::npos && on_tile_click)
				on_tile_click(idx);
			return 0;
		}
		case WM_CLOSE:
			ShowWindow(hwnd, SW_HIDE);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

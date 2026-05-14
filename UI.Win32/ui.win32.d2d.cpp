module;

#include <Windows.h>
#include <d2d1.h>

module ui.win32.d2d;

import <stdexcept>;
import <variant>;
import domain.diagram;

using Microsoft::WRL::ComPtr;


namespace ui::win32::d2d
{
	D2DContext create_d2d_context(HWND hwnd, const domain::Diagram* diagram)
	{
		D2DContext ctx{};

		D2D1_FACTORY_OPTIONS options{};
		auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			&options,
			reinterpret_cast<void**>(ctx.factory.GetAddressOf()));
		if (FAILED(hr)) throw std::runtime_error("Failed to create D2D factory");

		RECT rc{};
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		hr = ctx.factory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, size),
			ctx.renderTarget.GetAddressOf());
		if (FAILED(hr)) throw std::runtime_error("Failed to create HwndRenderTarget");

		// Dashed stroke style for lasso rectangle
		float dashes[] = { 4.f, 3.f };
		D2D1_STROKE_STYLE_PROPERTIES ssp = D2D1::StrokeStyleProperties();
		ssp.dashStyle = D2D1_DASH_STYLE_CUSTOM;
		ctx.factory->CreateStrokeStyle(ssp, dashes, 2, ctx.lasso_stroke_style.GetAddressOf());

		ctx.diagram = diagram;

		return ctx;
	}

	void resize(D2DContext& ctx, UINT width, UINT height)
	{
		if (ctx.renderTarget)
			ctx.renderTarget->Resize(D2D1::SizeU(width, height));
	}

	void render(D2DContext& ctx)
	{
		if (!ctx.renderTarget || !ctx.diagram)
			return;

		ctx.renderTarget->BeginDraw();
		ctx.renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		// --- shape fill brush (cornflower blue) ---
		ComPtr<ID2D1SolidColorBrush> shape_brush;
		ctx.renderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::CornflowerBlue), shape_brush.GetAddressOf());

		// --- selection highlight brush (orange, semi-transparent fill + solid stroke) ---
		ComPtr<ID2D1SolidColorBrush> sel_fill_brush;
		ctx.renderTarget->CreateSolidColorBrush(
			D2D1::ColorF(1.f, 0.55f, 0.f, 0.18f), sel_fill_brush.GetAddressOf());

		ComPtr<ID2D1SolidColorBrush> sel_stroke_brush;
		ctx.renderTarget->CreateSolidColorBrush(
			D2D1::ColorF(1.f, 0.55f, 0.f, 1.f), sel_stroke_brush.GetAddressOf());

		// --- lasso brush (blue dashed) ---
		ComPtr<ID2D1SolidColorBrush> lasso_brush;
		ctx.renderTarget->CreateSolidColorBrush(
			D2D1::ColorF(0.f, 0.47f, 1.f, 1.f), lasso_brush.GetAddressOf());

		// ── 1. Draw all shapes ───────────────────────────────────────────
		for (auto& shape : ctx.diagram->shapes())
		{
			std::visit([&](auto&& s)
			{
				using T = std::decay_t<decltype(s)>;

				if constexpr (std::is_same_v<T, domain::Rectangle>)
				{
					D2D1_RECT_F rect = D2D1::RectF(s.x, s.y, s.x + s.width, s.y + s.height);
					ctx.renderTarget->FillRectangle(rect, shape_brush.Get());
					ctx.renderTarget->DrawRectangle(rect, shape_brush.Get(), 2.f);
				}
				else if constexpr (std::is_same_v<T, domain::Ellipse>)
				{
					D2D1_ELLIPSE e = D2D1::Ellipse(
						D2D1::Point2F(s.x + s.width / 2.f, s.y + s.height / 2.f),
						s.width / 2.f, s.height / 2.f);
					ctx.renderTarget->FillEllipse(e, shape_brush.Get());
					ctx.renderTarget->DrawEllipse(e, shape_brush.Get(), 2.f);
				}
			}, shape);
		}

		// ── 2. Draw selection highlights ────────────────────────────────
		if (ctx.selected_shapes)
		{
			for (auto* sp : *ctx.selected_shapes)
			{
				std::visit([&](auto&& s)
				{
					using T = std::decay_t<decltype(s)>;

					if constexpr (std::is_same_v<T, domain::Rectangle>)
					{
						D2D1_RECT_F rect = D2D1::RectF(s.x, s.y, s.x + s.width, s.y + s.height);
						ctx.renderTarget->FillRectangle(rect, sel_fill_brush.Get());
						ctx.renderTarget->DrawRectangle(rect, sel_stroke_brush.Get(), 2.f);
					}
					else if constexpr (std::is_same_v<T, domain::Ellipse>)
					{
						D2D1_ELLIPSE e = D2D1::Ellipse(
							D2D1::Point2F(s.x + s.width / 2.f, s.y + s.height / 2.f),
							s.width / 2.f, s.height / 2.f);
						ctx.renderTarget->FillEllipse(e, sel_fill_brush.Get());
						ctx.renderTarget->DrawEllipse(e, sel_stroke_brush.Get(), 2.f);
					}
				}, *sp);
			}
		}

		// ── 3. Draw lasso rectangle overlay ─────────────────────────────
		if (ctx.lasso_active && ctx.lasso_stroke_style)
		{
			float x0 = ctx.lasso_x0 < ctx.lasso_x1 ? ctx.lasso_x0 : ctx.lasso_x1;
			float y0 = ctx.lasso_y0 < ctx.lasso_y1 ? ctx.lasso_y0 : ctx.lasso_y1;
			float x1 = ctx.lasso_x0 > ctx.lasso_x1 ? ctx.lasso_x0 : ctx.lasso_x1;
			float y1 = ctx.lasso_y0 > ctx.lasso_y1 ? ctx.lasso_y0 : ctx.lasso_y1;

			D2D1_RECT_F lasso_rect = D2D1::RectF(x0, y0, x1, y1);

			ComPtr<ID2D1SolidColorBrush> lasso_fill;
			ctx.renderTarget->CreateSolidColorBrush(
				D2D1::ColorF(0.f, 0.47f, 1.f, 0.07f), lasso_fill.GetAddressOf());
			ctx.renderTarget->FillRectangle(lasso_rect, lasso_fill.Get());
			ctx.renderTarget->DrawRectangle(lasso_rect, lasso_brush.Get(), 1.f,
				ctx.lasso_stroke_style.Get());
		}

		ctx.renderTarget->EndDraw();
	}
}

module;

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

export module ui.win32.stencil;

import <vector>;
import <functional>;

namespace ui::win32
{
	export constexpr float STENCIL_WIDTH      = 90.f;
	export constexpr float STENCIL_TILE_H     = 80.f;
	export constexpr float STENCIL_PADDING    = 10.f;

	export enum class StencilItemKind
	{
		None,
		Rectangle,
		Ellipse,
		Connector
	};

	// One entry in the stencil panel
	export struct StencilItem
	{
		StencilItemKind kind;
		const wchar_t*  label;

		// Bounding box in window coordinates — computed by Stencil::layout()
		float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	};

	export class Stencil
	{
	public:
		Stencil();

		// Recompute tile positions after a window resize
		void layout(float panel_height);

		// Returns the kind under (px, py), or None
		StencilItemKind hit_test(float px, float py) const;

		const std::vector<StencilItem>& items() const noexcept { return m_items; }

		StencilItemKind active() const noexcept   { return m_active; }
		void set_active(StencilItemKind k) noexcept { m_active = k; }

	private:
		std::vector<StencilItem> m_items;
		StencilItemKind          m_active = StencilItemKind::None;
	};

	// ----------------------------------------------------------------
	// Floating stencil palette window.
	// Create it after the main HWND exists; it is owned by the main
	// window so it always stays in front but never blocks Alt+F4.
	// ----------------------------------------------------------------
	export class StencilWindow
	{
	public:
		// callback invoked whenever the user clicks a tile
		std::function<void(StencilItemKind)> on_tile_click;

		// Create and show the floating window.
		// owner  – the main application HWND (for ownership / z-ordering)
		// x, y   – initial screen position
		void create(HWND owner, int x, int y);

		// Must be called when the owner window is destroyed.
		void destroy();

		// Update the highlighted tile (called by the main window after
		// a tool switch so the palette reflects the current selection).
		void set_active(StencilItemKind kind);

		StencilItemKind active() const noexcept { return m_stencil.active(); }

		HWND hwnd() const noexcept { return m_hwnd; }

		// Internal — called from the static WndProc
		LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		void paint();
		void init_d2d();

		HWND   m_hwnd  = nullptr;
		Stencil m_stencil;

		Microsoft::WRL::ComPtr<ID2D1Factory>          m_factory;
		Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_rt;
		Microsoft::WRL::ComPtr<IDWriteFactory>        m_dwrite;
		Microsoft::WRL::ComPtr<IDWriteTextFormat>     m_label_fmt;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static constexpr wchar_t CLASS_NAME[] = L"DiagramStencilPalette";
	};
}

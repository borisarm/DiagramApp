module;

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

export module ui.win32.stencil;

import <vector>;
import <functional>;
import <string>;

namespace ui::win32
{
	using Microsoft::WRL::ComPtr;

	export constexpr float STENCIL_WIDTH   = 90.f;
	export constexpr float STENCIL_TILE_H  = 80.f;
	export constexpr float STENCIL_PADDING = 10.f;

	// One tile in the stencil palette.
	// Tiles are built at runtime from IShapeClass objects — no hardcoded enum.
	export struct StencilTile
	{
		ComPtr<IShapeClass> shape_class;  // nullptr for the Connector sentinel tile
		std::wstring        name;         // cached from GetName()

		// Bounding box in stencil-window client coordinates (set by layout())
		float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	};

	// ----------------------------------------------------------------
	// Stencil model
	// ----------------------------------------------------------------
	export class Stencil
	{
	public:
		void populate(const std::vector<ComPtr<IShapeClass>>& classes);
		void add_connector_tile();

		void layout(float panel_height);

		static constexpr std::size_t npos = static_cast<std::size_t>(-1);
		std::size_t hit_test(float px, float py) const;

		const std::vector<StencilTile>& tiles() const noexcept { return m_tiles; }

		std::size_t active_index() const noexcept  { return m_active; }
		void set_active(std::size_t idx) noexcept  { m_active = idx; }
		void clear_active() noexcept               { m_active = npos; }

		IShapeClass* active_class() const noexcept
		{
			if (m_active == npos || m_active >= m_tiles.size()) return nullptr;
			return m_tiles[m_active].shape_class.Get();
		}

		bool active_is_connector() const noexcept
		{
			if (m_active == npos || m_active >= m_tiles.size()) return false;
			return m_tiles[m_active].shape_class == nullptr;
		}

	private:
		std::vector<StencilTile> m_tiles;
		std::size_t              m_active = npos;
	};

	// ----------------------------------------------------------------
	// Floating stencil palette window
	// ----------------------------------------------------------------
	export class StencilWindow
	{
	public:
		std::function<void(std::size_t tile_index)> on_tile_click;

		void create(HWND owner, int x, int y);
		void destroy();
		void populate(const std::vector<ComPtr<IShapeClass>>& classes);

		void set_active(std::size_t tile_index);
		void clear_active();

		const Stencil& stencil() const noexcept { return m_stencil; }
		HWND hwnd() const noexcept { return m_hwnd; }

		LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		void paint();
		void init_d2d();

		HWND    m_hwnd = nullptr;
		Stencil m_stencil;

		ComPtr<ID2D1Factory>          m_factory;
		ComPtr<ID2D1HwndRenderTarget> m_rt;
		ComPtr<IDWriteFactory>        m_dwrite;
		ComPtr<IDWriteTextFormat>     m_label_fmt;

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static constexpr wchar_t CLASS_NAME[] = L"DiagramStencilPalette";
	};
}

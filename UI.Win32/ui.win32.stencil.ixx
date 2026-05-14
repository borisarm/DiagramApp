module;

export module ui.win32.stencil;

import <vector>;

namespace ui::win32
{
	export constexpr float STENCIL_WIDTH      = 90.f;
	export constexpr float STENCIL_TILE_H     = 80.f;
	export constexpr float STENCIL_PADDING    = 10.f;

	export enum class StencilItemKind
	{
		None,
		Rectangle,
		Ellipse
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
}

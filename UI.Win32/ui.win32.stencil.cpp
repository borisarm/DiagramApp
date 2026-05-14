module ui.win32.stencil;

namespace ui::win32
{
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
}

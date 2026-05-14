module;

#include <Windows.h>

module ui.win32.addconnectortool;

import domain.shape;
import <variant>;

namespace ui::win32
{
	AddConnectorTool::AddConnectorTool(domain::Diagram& diagram)
		: m_diagram(diagram)
	{}

	void AddConnectorTool::on_activate()
	{
		m_state       = State::Idle;
		m_done        = false;
		m_has_preview = false;
		m_source      = nullptr;
	}

	void AddConnectorTool::on_deactivate()
	{
		m_has_preview = false;
		m_source      = nullptr;
		m_state       = State::Idle;
	}

	// -----------------------------------------------------------------------
	//  Helper: geometric centre of a shape
	// -----------------------------------------------------------------------
	void AddConnectorTool::shape_centre(const domain::Shape& s, float& cx, float& cy)
	{
		std::visit([&](const auto& r)
		{
			cx = r.x + r.width  * 0.5f;
			cy = r.y + r.height * 0.5f;
		}, s);
	}

	// -----------------------------------------------------------------------
	//  Input handlers
	// -----------------------------------------------------------------------
	void AddConnectorTool::on_pointer_down(const PointerEvent& e)
	{
		if (!e.left) return;

		auto candidates = m_diagram.hit_test_all(e.x, e.y);

		if (m_state == State::Idle)
		{
			if (candidates.empty()) return;

			m_source = candidates.back();
			shape_centre(*m_source, m_src_x, m_src_y);
			m_cur_x = e.x;
			m_cur_y = e.y;
			m_has_preview = true;
			m_state = State::SourceLocked;
		}
		else // SourceLocked — user clicks a target
		{
			// Cancel if user clicks on empty space
			if (candidates.empty())
			{
				on_deactivate();
				return;
			}

			const domain::Shape* target = candidates.back();
			if (target == m_source)
			{
				// same shape — cancel
				on_deactivate();
				return;
			}

			std::size_t src_idx = m_diagram.index_of(m_source);
			std::size_t tgt_idx = m_diagram.index_of(target);

			if (src_idx != domain::Diagram::npos && tgt_idx != domain::Diagram::npos)
			{
				m_diagram.commit();
				m_diagram.add_connector(src_idx, tgt_idx);
			}

			m_done        = true;
			m_has_preview = false;
			m_source      = nullptr;
			m_state       = State::Idle;
		}
	}

	void AddConnectorTool::on_pointer_move(const PointerEvent& e)
	{
		if (m_state == State::SourceLocked)
		{
			m_cur_x = e.x;
			m_cur_y = e.y;
		}
	}

	void AddConnectorTool::on_pointer_up(const PointerEvent&) {}

	void AddConnectorTool::on_key_down(const KeyEvent& e)
	{
		if (e.key == VK_ESCAPE)
			on_deactivate();
	}
}

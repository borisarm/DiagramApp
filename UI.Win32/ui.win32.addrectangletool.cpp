module ui.win32.addrectangletool;

import domain.shape;

namespace ui::win32
{
	AddRectangleTool::AddRectangleTool(domain::Diagram& diagram)
		: m_diagram(diagram)
	{}

	void AddRectangleTool::on_activate()
	{
		m_state = AddShapeState::Idle;
		m_done  = false;
	}

	void AddRectangleTool::on_deactivate()
	{
		m_state = AddShapeState::Idle;
	}

	void AddRectangleTool::on_pointer_down(const PointerEvent& e)
	{
		m_x0 = m_x1 = e.x;
		m_y0 = m_y1 = e.y;
		m_state = AddShapeState::Pressing;
	}

	void AddRectangleTool::on_pointer_move(const PointerEvent& e)
	{
		if (m_state == AddShapeState::Pressing ||
			m_state == AddShapeState::Previewing)
		{
			m_x1 = e.x;
			m_y1 = e.y;
			m_state = AddShapeState::Previewing;
		}
	}

	void AddRectangleTool::on_pointer_up(const PointerEvent&)
	{
		if (m_state == AddShapeState::Previewing)
		{
			float x = m_x0 < m_x1 ? m_x0 : m_x1;
			float y = m_y0 < m_y1 ? m_y0 : m_y1;
			float w = (m_x1 > m_x0 ? m_x1 - m_x0 : m_x0 - m_x1);
			float h = (m_y1 > m_y0 ? m_y1 - m_y0 : m_y0 - m_y1);

			if (w > 2.f && h > 2.f)
			{
				m_diagram.commit();
				m_diagram.add_shape(domain::Rectangle{ x, y, w, h });
			}
		}

		m_state = AddShapeState::Idle;
		m_done  = true;
	}

	void AddRectangleTool::on_key_down(const KeyEvent& e)
	{
		if (e.key == 27) // Escape — cancel
		{
			m_state = AddShapeState::Idle;
			m_done  = true;
		}
	}
}

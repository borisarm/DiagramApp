module ui.win32.addellipsetool;

import domain.shape;

namespace ui::win32
{
	AddEllipseTool::AddEllipseTool(domain::Diagram& diagram)
		: m_diagram(diagram)
	{}

	void AddEllipseTool::on_activate()
	{
		m_previewing = false;
		m_done       = false;
	}

	void AddEllipseTool::on_deactivate()
	{
		m_previewing = false;
	}

	void AddEllipseTool::on_pointer_down(const PointerEvent& e)
	{
		m_x0 = m_x1 = e.x;
		m_y0 = m_y1 = e.y;
		m_previewing = true;
	}

	void AddEllipseTool::on_pointer_move(const PointerEvent& e)
	{
		if (m_previewing)
		{
			m_x1 = e.x;
			m_y1 = e.y;
		}
	}

	void AddEllipseTool::on_pointer_up(const PointerEvent&)
	{
		if (m_previewing)
		{
			float x = m_x0 < m_x1 ? m_x0 : m_x1;
			float y = m_y0 < m_y1 ? m_y0 : m_y1;
			float w = (m_x1 > m_x0 ? m_x1 - m_x0 : m_x0 - m_x1);
			float h = (m_y1 > m_y0 ? m_y1 - m_y0 : m_y0 - m_y1);

			if (w > 2.f && h > 2.f)
			{
				m_diagram.commit();
				m_diagram.add_shape(domain::Ellipse{ x, y, w, h });
			}
		}

		m_previewing = false;
		m_done       = true;
	}

	void AddEllipseTool::on_key_down(const KeyEvent& e)
	{
		if (e.key == 27)
		{
			m_previewing = false;
			m_done       = true;
		}
	}
}

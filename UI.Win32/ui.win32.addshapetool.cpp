module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

module ui.win32.addshapetool;

namespace ui::win32
{
	using Microsoft::WRL::ComPtr;

	AddShapeTool::AddShapeTool(domain::Diagram& diagram, IShapeClass* shape_class)
		: m_diagram(diagram)
	{
		set_shape_class(shape_class);
	}

	void AddShapeTool::set_shape_class(IShapeClass* shape_class)
	{
		m_class = shape_class;

		// Cache the geometry kind for the preview renderer
		if (m_class)
		{
			ShapeGeometry geom{};
			m_class->GetStencilGeometry(&geom);
			m_preview_kind = geom.kind;
		}
	}

	void AddShapeTool::on_activate()
	{
		m_previewing = false;
		m_done       = false;
	}

	void AddShapeTool::on_deactivate()
	{
		m_previewing = false;
	}

	void AddShapeTool::on_pointer_down(const PointerEvent& e)
	{
		m_x0 = m_x1 = e.x;
		m_y0 = m_y1 = e.y;
		m_previewing = true;
	}

	void AddShapeTool::on_pointer_move(const PointerEvent& e)
	{
		if (m_previewing)
		{
			m_x1 = e.x;
			m_y1 = e.y;
		}
	}

	void AddShapeTool::on_pointer_up(const PointerEvent&)
	{
		if (!m_previewing || !m_class) return;

		float x = m_x0 < m_x1 ? m_x0 : m_x1;
		float y = m_y0 < m_y1 ? m_y0 : m_y1;
		float w = (m_x1 > m_x0 ? m_x1 - m_x0 : m_x0 - m_x1);
		float h = (m_y1 > m_y0 ? m_y1 - m_y0 : m_y0 - m_y1);

		if (w > 2.f && h > 2.f)
		{
			IShape* raw = nullptr;
			if (SUCCEEDED(m_class->CreateShape(x, y, w, h, &raw)) && raw)
			{
				m_diagram.commit();
				m_diagram.add_shape(ComPtr<IShape>(raw));
			}
			m_done = true;
		}

		m_previewing = false;
	}

	void AddShapeTool::on_key_down(const KeyEvent& e)
	{
		if (e.key == VK_ESCAPE)
		{
			m_previewing = false;
			m_done       = true;
		}
	}
}

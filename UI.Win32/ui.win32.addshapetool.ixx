module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

export module ui.win32.addshapetool;

import ui.win32.tool;
import domain.diagram;

namespace ui::win32
{
	using Microsoft::WRL::ComPtr;

	export class AddShapeTool : public ITool
	{
	public:
		// shape_class must remain alive as long as this tool is active
		// (it is owned by the ShapeRegistry / stencil, which outlives tools)
		AddShapeTool(domain::Diagram& diagram, IShapeClass* shape_class);

		// Switch to a different shape type without recreating the tool
		void set_shape_class(IShapeClass* shape_class);
		IShapeClass* shape_class() const noexcept { return m_class.Get(); }

		void on_activate()   override;
		void on_deactivate() override;

		void on_pointer_down(const PointerEvent& e) override;
		void on_pointer_move(const PointerEvent& e) override;
		void on_pointer_up  (const PointerEvent& e) override;
		void on_key_down    (const KeyEvent& e)     override;
		void on_key_up      (const KeyEvent&)       override {}
		void update()                               override {}
		void render_overlays()                      override {}

		ToolKind       kind()         const noexcept override { return ToolKind::AddRectangle; }
		const wchar_t* display_name() const noexcept override { return L"Add Shape"; }

		bool is_done()     const noexcept { return m_done; }
		void reset_done()        noexcept { m_done = false; }

		bool  has_preview() const noexcept { return m_previewing; }
		float preview_x0()  const noexcept { return m_x0 < m_x1 ? m_x0 : m_x1; }
		float preview_y0()  const noexcept { return m_y0 < m_y1 ? m_y0 : m_y1; }
		float preview_x1()  const noexcept { return m_x0 > m_x1 ? m_x0 : m_x1; }
		float preview_y1()  const noexcept { return m_y0 > m_y1 ? m_y0 : m_y1; }

		// Geometry kind for the preview overlay (drives renderer switch)
		ShapeGeometryKind preview_geometry_kind() const noexcept
		{ return m_preview_kind; }

	private:
		domain::Diagram&    m_diagram;
		ComPtr<IShapeClass> m_class;
		ShapeGeometryKind   m_preview_kind = ShapeGeometryKind::Rect;

		bool  m_previewing = false;
		bool  m_done       = false;
		float m_x0 = 0, m_y0 = 0;
		float m_x1 = 0, m_y1 = 0;
	};
}

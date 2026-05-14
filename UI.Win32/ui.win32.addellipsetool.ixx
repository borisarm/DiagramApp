module;

export module ui.win32.addellipsetool;

import ui.win32.tool;
import domain.diagram;

namespace ui::win32
{
	export class AddEllipseTool : public ITool
	{
	public:
		explicit AddEllipseTool(domain::Diagram& diagram);

		void on_activate()   override;
		void on_deactivate() override;

		void on_pointer_down(const PointerEvent& e) override;
		void on_pointer_move(const PointerEvent& e) override;
		void on_pointer_up  (const PointerEvent& e) override;

		void on_key_down(const KeyEvent& e) override;
		void on_key_up  (const KeyEvent&)   override {}

		void update()          override {}
		void render_overlays() override {}

		ToolKind       kind()         const noexcept override { return ToolKind::AddEllipse; }
		const wchar_t* display_name() const noexcept override { return L"Add Ellipse"; }

		bool is_done() const noexcept { return m_done; }
		void reset_done()    noexcept { m_done = false; }

		bool  has_preview() const noexcept { return m_previewing; }
		float preview_x0()  const noexcept { return m_x0 < m_x1 ? m_x0 : m_x1; }
		float preview_y0()  const noexcept { return m_y0 < m_y1 ? m_y0 : m_y1; }
		float preview_x1()  const noexcept { return m_x0 > m_x1 ? m_x0 : m_x1; }
		float preview_y1()  const noexcept { return m_y0 > m_y1 ? m_y0 : m_y1; }

	private:
		domain::Diagram& m_diagram;
		bool             m_previewing = false;
		bool             m_done       = false;

		float m_x0 = 0, m_y0 = 0;
		float m_x1 = 0, m_y1 = 0;
	};
}

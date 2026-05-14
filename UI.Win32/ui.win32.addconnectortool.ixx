module;

export module ui.win32.addconnectortool;

import ui.win32.tool;
import domain.diagram;
import domain.shape;

namespace ui::win32
{
	export class AddConnectorTool : public ITool
	{
	public:
		explicit AddConnectorTool(domain::Diagram& diagram);

		void on_activate()   override;
		void on_deactivate() override;

		void on_pointer_down(const PointerEvent& e) override;
		void on_pointer_move(const PointerEvent& e) override;
		void on_pointer_up  (const PointerEvent& e) override;
		void on_key_down    (const KeyEvent& e)     override;
		void on_key_up      (const KeyEvent&)       override {}
		void update()                               override {}
		void render_overlays()                      override {}

		ToolKind        kind()         const noexcept override { return ToolKind::Connector; }
		const wchar_t*  display_name() const noexcept override { return L"Add Connector"; }

		// Preview line — read by window.cpp to feed D2DContext
		bool  has_preview()    const noexcept { return m_has_preview; }
		float preview_x0()     const noexcept { return m_src_x; }
		float preview_y0()     const noexcept { return m_src_y; }
		float preview_x1()     const noexcept { return m_cur_x; }
		float preview_y1()     const noexcept { return m_cur_y; }

		bool  is_done()  const noexcept { return m_done; }
		void  reset_done()     noexcept { m_done = false; }

	private:
		domain::Diagram& m_diagram;

		enum class State { Idle, SourceLocked };
		State m_state = State::Idle;

		bool  m_done        = false;
		bool  m_has_preview = false;

		const domain::Shape* m_source = nullptr;
		float m_src_x = 0, m_src_y = 0;  // centre of source shape
		float m_cur_x = 0, m_cur_y = 0;  // current cursor position

		static void shape_centre(const domain::Shape& s, float& cx, float& cy);
	};
}

module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

export module ui.win32.addconnectortool;

import ui.win32.tool;
import domain.diagram;

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

		IShape* m_source = nullptr;  // non-owning; shape lives in Diagram
		float m_src_x = 0, m_src_y = 0;
		float m_cur_x = 0, m_cur_y = 0;

		static void shape_centre(IShape* s, float& cx, float& cy);
	};
}

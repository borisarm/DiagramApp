module;

export module ui.win32.toolmanager;

import ui.win32.tool;

namespace ui::win32
{
    export class ToolManager
    {
    public:
        void set_active_tool(ITool* tool);
        ITool* active_tool() const noexcept;

        void dispatch_pointer_down(const PointerEvent& e);
        void dispatch_pointer_move(const PointerEvent& e);
        void dispatch_pointer_up(const PointerEvent& e);

        void dispatch_key_down(const KeyEvent& e);
        void dispatch_key_up(const KeyEvent& e);

        void update();
        void render_overlays();

    private:
        ITool* current = nullptr;
    };
}

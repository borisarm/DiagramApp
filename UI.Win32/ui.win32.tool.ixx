module;

export module ui.win32.tool;

import domain.diagram;

namespace ui::win32
{
    export enum class ToolKind
    {
        Select,
        AddRectangle,
        AddEllipse,
        AddComposite,
        Subtract,
        Layer,
        Connector,
        Text,
        Group,
        Ungroup
        // … extend freely
    };

    export struct PointerEvent
    {
        float x;
        float y;
        bool left;
        bool right;
        bool shift;
        bool ctrl;
        bool alt;
    };

    export struct KeyEvent
    {
        int key;
        bool shift;
        bool ctrl;
        bool alt;
    };

    export class ITool
    {
    public:
        virtual ~ITool() = default;

        // Called when the tool becomes active
        virtual void on_activate() = 0;

        // Called when the tool is deactivated
        virtual void on_deactivate() = 0;

        // Pointer events (mouse)
        virtual void on_pointer_down(const PointerEvent& e) = 0;
        virtual void on_pointer_move(const PointerEvent& e) = 0;
        virtual void on_pointer_up(const PointerEvent& e) = 0;

        // Keyboard events
        virtual void on_key_down(const KeyEvent& e) = 0;
        virtual void on_key_up(const KeyEvent& e) = 0;

        // Called every frame before rendering (optional)
        virtual void update() = 0;

        // Called during rendering to draw tool overlays (lasso, previews, handles)
        virtual void render_overlays() = 0;

        // What kind of tool this is
        virtual ToolKind kind() const noexcept = 0;
    };
}

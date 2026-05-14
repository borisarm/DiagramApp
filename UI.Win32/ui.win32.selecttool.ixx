module;

export module ui.win32.selecttool;

import ui.win32.tool;
import domain.diagram;
import domain.shape;

import <vector>;
import <optional>;

namespace ui::win32
{
    export enum class SelectState
    {
        Idle,
        PressedOnShape,
        Dragging,
        LassoSelecting,
        Selected
    };

    export class SelectTool : public ITool
    {
    public:
        SelectTool(domain::Diagram& diagram);

        // ITool interface
        void on_activate() override;
        void on_deactivate() override;

        void on_pointer_down(const PointerEvent& e) override;
        void on_pointer_move(const PointerEvent& e) override;
        void on_pointer_up(const PointerEvent& e) override;

        void on_key_down(const KeyEvent& e) override;
        void on_key_up(const KeyEvent&) override {}

        void update() override {}
        void render_overlays() override;

        ToolKind kind() const noexcept override { return ToolKind::Select; }

        const std::vector<const domain::Shape*>& selected() const noexcept { return selected_shapes; }

        bool is_lasso_active()    const noexcept { return lasso_active; }
        float get_lasso_x0()     const noexcept { return lasso_x0; }
        float get_lasso_y0()     const noexcept { return lasso_y0; }
        float get_lasso_x1()     const noexcept { return lasso_x1; }
        float get_lasso_y1()     const noexcept { return lasso_y1; }

    private:
        domain::Diagram& diagram;
        SelectState state = SelectState::Idle;

        std::vector<const domain::Shape*> selected_shapes;
        const domain::Shape* pressed_shape = nullptr;

        float press_x = 0;
        float press_y = 0;

        // Dragging
        float drag_start_x = 0;
        float drag_start_y = 0;

        // Lasso
        bool lasso_active = false;
        float lasso_x0 = 0;
        float lasso_y0 = 0;
        float lasso_x1 = 0;
        float lasso_y1 = 0;

        bool movement_exceeds_threshold(float x, float y) const;
        void commit_selection(const domain::Shape* s, bool shift, bool ctrl);
        void update_drag(float x, float y);
        void update_lasso(float x, float y);
        void commit_lasso();
		void move_shape(const domain::Shape* s, float dx, float dy);
    };
}

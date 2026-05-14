module ui.win32.selecttool;

import <algorithm>;
import <cmath>;
import domain.diagram;

namespace ui::win32
{
    SelectTool::SelectTool(domain::Diagram& d)
        : diagram(d)
    {}

    void SelectTool::on_activate()
    {
        state = SelectState::Idle;
    }

    void SelectTool::on_deactivate()
    {
        selected_shapes.clear();
        state = SelectState::Idle;
    }

    // ------------------------------------------------------------
    // Pointer Down
    // ------------------------------------------------------------
    void SelectTool::on_pointer_down(const PointerEvent& e)
    {
        press_x = e.x;
        press_y = e.y;

        auto hits = diagram.hit_test_all(e.x, e.y);

        if (!hits.empty())
        {
            pressed_shape = hits.front();
            state = SelectState::PressedOnShape;
        }
        else
        {
            // Begin lasso
            lasso_active = true;
            lasso_x0 = lasso_x1 = e.x;
            lasso_y0 = lasso_y1 = e.y;
            state = SelectState::LassoSelecting;
        }
    }

    // ------------------------------------------------------------
    // Pointer Move
    // ------------------------------------------------------------
    void SelectTool::on_pointer_move(const PointerEvent& e)
    {
        switch (state)
        {
        case SelectState::PressedOnShape:
            if (movement_exceeds_threshold(e.x, e.y))
            {
                drag_start_x = e.x;
                drag_start_y = e.y;
                state = SelectState::Dragging;
            }
            break;

        case SelectState::Dragging:
            update_drag(e.x, e.y);
            break;

        case SelectState::LassoSelecting:
            update_lasso(e.x, e.y);
            break;

        default:
            break;
        }
    }

    // ------------------------------------------------------------
    // Pointer Up
    // ------------------------------------------------------------
    void SelectTool::on_pointer_up(const PointerEvent& e)
    {
        switch (state)
        {
        case SelectState::PressedOnShape:
            commit_selection(pressed_shape, e.shift, e.ctrl);
            break;

        case SelectState::Dragging:
            commit_selection(pressed_shape, false, false);
            break;

        case SelectState::LassoSelecting:
            commit_lasso();
            break;

        default:
            break;
        }

        pressed_shape = nullptr;
        lasso_active = false;
        state = SelectState::Selected;
    }

    // ------------------------------------------------------------
    // Key Down
    // ------------------------------------------------------------
    void SelectTool::on_key_down(const KeyEvent& e)
    {
        if (e.key == 'A' && e.ctrl)
        {
            selected_shapes.clear();
            for (auto& s : diagram.shapes())
                selected_shapes.push_back(&s);
            state = SelectState::Selected;
        }

        if (e.key == 27) // Escape
        {
            selected_shapes.clear();
            state = SelectState::Idle;
        }
    }

    // ------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------
    bool SelectTool::movement_exceeds_threshold(float x, float y) const
    {
        float dx = x - press_x;
        float dy = y - press_y;
        return (dx * dx + dy * dy) > 4.0f; // 2px threshold
    }

    void SelectTool::commit_selection(const domain::Shape* s, bool shift, bool ctrl)
    {
        if (!s)
        {
            selected_shapes.clear();
            return;
        }

        if (ctrl)
        {
            // Toggle
            auto it = std::find(selected_shapes.begin(), selected_shapes.end(), s);
            if (it == selected_shapes.end())
                selected_shapes.push_back(s);
            else
                selected_shapes.erase(it);
        }
        else if (shift)
        {
            // Add
            if (std::find(selected_shapes.begin(), selected_shapes.end(), s) == selected_shapes.end())
                selected_shapes.push_back(s);
        }
        else
        {
            // Replace
            selected_shapes.clear();
            selected_shapes.push_back(s);
        }
    }

    void SelectTool::update_drag(float x, float y)
    {
        float dx = x - drag_start_x;
        float dy = y - drag_start_y;

        for (auto* s : selected_shapes)
            diagram.move_shape(s, dx, dy);

        drag_start_x = x;
        drag_start_y = y;
    }

    void SelectTool::update_lasso(float x, float y)
    {
        lasso_x1 = x;
        lasso_y1 = y;
    }

    void SelectTool::commit_lasso()
    {
        selected_shapes.clear();

        float x0 = std::min(lasso_x0, lasso_x1);
        float y0 = std::min(lasso_y0, lasso_y1);
        float x1 = std::max(lasso_x0, lasso_x1);
        float y1 = std::max(lasso_y0, lasso_y1);

        for (auto& s : diagram.shapes())
        {
            if (diagram.shape_intersects_rect(s, x0, y0, x1, y1))
                selected_shapes.push_back(&s);
        }
    }


    // ------------------------------------------------------------
    // Overlays
    // ------------------------------------------------------------
    void SelectTool::render_overlays()
    {
        if (lasso_active)
        {
            // The renderer will draw the lasso rectangle
            // using the coordinates stored here.
        }
    }
}

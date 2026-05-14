module domain.diagram;

import domain.shape;
import <algorithm>;

namespace domain
{
    void Diagram::add_shape(const Shape& s)
    {
        m_shapes.push_back(s);
    }

    void Diagram::remove_shape(const Shape* s)
    {
        std::size_t idx = index_of(s);

        m_shapes.erase(
            std::remove_if(m_shapes.begin(), m_shapes.end(),
                [s](const Shape& shape) { return &shape == s; }),
            m_shapes.end());

        // Remove connectors that referenced the deleted shape, and
        // fix up indices that shifted due to the erasure.
        if (idx != npos)
        {
            m_connectors.erase(
                std::remove_if(m_connectors.begin(), m_connectors.end(),
                    [idx](const Connector& c)
                    {
                        return c.source_index == idx || c.target_index == idx;
                    }),
                m_connectors.end());

            for (auto& c : m_connectors)
            {
                if (c.source_index > idx) --c.source_index;
                if (c.target_index > idx) --c.target_index;
            }
        }
    }

    std::size_t Diagram::index_of(const Shape* s) const noexcept
    {
        for (std::size_t i = 0; i < m_shapes.size(); ++i)
            if (&m_shapes[i] == s) return i;
        return npos;
    }

    void Diagram::add_connector(std::size_t src, std::size_t tgt)
    {
        if (src < m_shapes.size() && tgt < m_shapes.size() && src != tgt)
            m_connectors.push_back({ src, tgt });
    }

    const std::vector<Connector>& Diagram::connectors() const noexcept
    {
        return m_connectors;
    }

    void Diagram::commit()
    {
        if (static_cast<int>(m_undo_stack.size()) >= MAX_UNDO)
            m_undo_stack.erase(m_undo_stack.begin());

        m_undo_stack.push_back({ m_shapes, m_connectors });
        m_redo_stack.clear();
    }

    void Diagram::undo()
    {
        if (m_undo_stack.empty()) return;
        m_redo_stack.push_back({ m_shapes, m_connectors });
        m_shapes     = m_undo_stack.back().shapes;
        m_connectors = m_undo_stack.back().connectors;
        m_undo_stack.pop_back();
    }

    void Diagram::redo()
    {
        if (m_redo_stack.empty()) return;
        m_undo_stack.push_back({ m_shapes, m_connectors });
        m_shapes     = m_redo_stack.back().shapes;
        m_connectors = m_redo_stack.back().connectors;
        m_redo_stack.pop_back();
    }

    bool Diagram::can_undo() const noexcept { return !m_undo_stack.empty(); }
    bool Diagram::can_redo() const noexcept { return !m_redo_stack.empty(); }

    const std::vector<Shape>& Diagram::shapes() const noexcept
    {
        return m_shapes;
    }

    std::vector<const Shape*> Diagram::hit_test_all(float px, float py) const
    {
        std::vector<const Shape*> result;

        // Reverse iteration = topmost first
        for (auto it = m_shapes.rbegin(); it != m_shapes.rend(); ++it)
        {
            if (hit_test(*it, px, py))
                result.push_back(&(*it));
        }

        return result;
    }

	void Diagram::move_shape(const Shape* s, float dx, float dy)
	{
		// This is a bit hacky - we need to find the shape in our vector and modify it
		for (auto& shape : m_shapes)
		{
			if (&shape == s)
			{
				std::visit([&](auto&& sh)
					{
						using T = std::decay_t<decltype(sh)>;
						if constexpr (std::is_same_v<T, Rectangle> || std::is_same_v<T, Ellipse>)
						{
							sh.x += dx;
							sh.y += dy;
						}
					}, shape);
				break;
			}
		}
	}

    bool Diagram::shape_intersects_rect(const Shape& s, float x0, float y0, float x1, float y1) const
	{
		return std::visit([&](auto&& shape)
			{
				using T = std::decay_t<decltype(shape)>;
				if constexpr (std::is_same_v<T, Rectangle>)
				{
					float sx0 = shape.x;
					float sy0 = shape.y;
					float sx1 = shape.x + shape.width;
					float sy1 = shape.y + shape.height;
					return !(sx1 < x0 || sx0 > x1 || sy1 < y0 || sy0 > y1);
				}
				else if constexpr (std::is_same_v<T, Ellipse>)
				{
					// Approximate ellipse as bounding box for intersection test
					float sx0 = shape.x;
					float sy0 = shape.y;
					float sx1 = shape.x + shape.width;
					float sy1 = shape.y + shape.height;
					return !(sx1 < x0 || sx0 > x1 || sy1 < y0 || sy0 > y1);
				}
			}, s);
	}
}

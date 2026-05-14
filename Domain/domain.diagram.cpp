module domain.diagram;

import domain.shape;

namespace domain
{
    void Diagram::add_shape(const Shape& s)
    {
        m_shapes.push_back(s);
    }

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

module;

module domain.shape;

import <cmath>;

namespace domain
{
    bool hit_test(const Shape& s, float px, float py)
    {
        return std::visit([&](auto&& shape)
            {
                using T = std::decay_t<decltype(shape)>;

                if constexpr (std::is_same_v<T, Rectangle>)
                {
                    return px >= shape.x &&
                        px <= shape.x + shape.width &&
                        py >= shape.y &&
                        py <= shape.y + shape.height;
                }
                else if constexpr (std::is_same_v<T, Ellipse>)
                {
                    float cx = shape.x + shape.width / 2.f;
                    float cy = shape.y + shape.height / 2.f;
                    float rx = shape.width / 2.f;
                    float ry = shape.height / 2.f;

                    float dx = (px - cx) / rx;
                    float dy = (py - cy) / ry;

                    return dx * dx + dy * dy <= 1.f;
                }

            }, s);
    }
}

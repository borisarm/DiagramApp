module;

export module domain.shape;

import <variant>;

namespace domain
{
    export struct Rectangle
    {
        float x;
        float y;
        float width;
        float height;
    };

    export struct Ellipse
    {
        float x;
        float y;
        float width;
        float height;
    };

    export using Shape = std::variant<Rectangle, Ellipse>;
    export bool hit_test(const Shape& s, float px, float py);
    

}

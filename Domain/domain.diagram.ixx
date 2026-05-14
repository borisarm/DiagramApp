module;

export module domain.diagram;

import <vector>;
import domain.shape;

namespace domain
{
    export class Diagram
    {
    public:
		void add_shape(const Shape& s);
		void remove_shape(const Shape* s);
		const std::vector<Shape>& shapes() const noexcept;
		std::vector<const Shape*> hit_test_all(float px, float py) const;
		void move_shape(const Shape* s, float dx, float dy);
		bool shape_intersects_rect(const Shape& s, float x0, float y0, float x1, float y1) const;
		


    private:
        std::vector<Shape> m_shapes;
    };
}

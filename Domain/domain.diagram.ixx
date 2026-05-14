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

		// Undo / redo
		void commit();          // snapshot current state before a mutation
		void undo();
		void redo();
		bool can_undo() const noexcept;
		bool can_redo() const noexcept;

	private:
		static constexpr int MAX_UNDO = 50;

		std::vector<Shape> m_shapes;
		std::vector<std::vector<Shape>> m_undo_stack;
		std::vector<std::vector<Shape>> m_redo_stack;
	};
}

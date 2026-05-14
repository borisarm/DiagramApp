module;

export module domain.diagram;

import <vector>;
import <cstddef>;
import domain.shape;

namespace domain
{
	// A directed connector between two shapes, stored as indices into shapes().
	export struct Connector
	{
		std::size_t source_index = 0;
		std::size_t target_index = 0;
	};

	export class Diagram
	{
	public:
		// Shapes
		void add_shape(const Shape& s);
		void remove_shape(const Shape* s);
		const std::vector<Shape>& shapes() const noexcept;
		std::vector<const Shape*> hit_test_all(float px, float py) const;
		void move_shape(const Shape* s, float dx, float dy);
		bool shape_intersects_rect(const Shape& s, float x0, float y0, float x1, float y1) const;

		// Returns the index of shape s, or npos if not found.
		static constexpr std::size_t npos = static_cast<std::size_t>(-1);
		std::size_t index_of(const Shape* s) const noexcept;

		// Connectors
		void add_connector(std::size_t source_index, std::size_t target_index);
		const std::vector<Connector>& connectors() const noexcept;

		// Undo / redo
		void commit();
		void undo();
		void redo();
		bool can_undo() const noexcept;
		bool can_redo() const noexcept;

	private:
		static constexpr int MAX_UNDO = 50;

		struct Snapshot
		{
			std::vector<Shape>     shapes;
			std::vector<Connector> connectors;
		};

		std::vector<Shape>     m_shapes;
		std::vector<Connector> m_connectors;

		std::vector<Snapshot>  m_undo_stack;
		std::vector<Snapshot>  m_redo_stack;
	};
}

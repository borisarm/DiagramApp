module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

export module domain.diagram;

import <vector>;
import <cstddef>;

namespace domain
{
	using Microsoft::WRL::ComPtr;

	// A directed connector between two shapes, stored as indices into shapes().
	export struct Connector
	{
		std::size_t source_index = 0;
		std::size_t target_index = 0;
	};

	export class Diagram
	{
	public:
		// ── Shapes ──────────────────────────────────────────────────
		void add_shape(ComPtr<IShape> shape);
		void remove_shape(IShape* s);

		const std::vector<ComPtr<IShape>>& shapes() const noexcept;

		// Returns all shapes whose hit-test passes for (px, py).
		// Results are raw non-owning pointers; valid until next mutation.
		std::vector<IShape*> hit_test_all(float px, float py) const;

		void move_shape(IShape* s, float dx, float dy);
		bool shape_intersects_rect(IShape* s,
								   float x0, float y0,
								   float x1, float y1) const;

		// ── Index lookup ────────────────────────────────────────────
		static constexpr std::size_t npos = static_cast<std::size_t>(-1);
		std::size_t index_of(const IShape* s) const noexcept;

		// ── Connectors ──────────────────────────────────────────────
		void add_connector(std::size_t source_index, std::size_t target_index);
		const std::vector<Connector>& connectors() const noexcept;

		// ── Undo / redo ─────────────────────────────────────────────
		void commit();
		void undo();
		void redo();
		bool can_undo() const noexcept;
		bool can_redo() const noexcept;

	private:
		static constexpr int MAX_UNDO = 50;

		// Deep-copy the current shape list via IShape::Clone
		std::vector<ComPtr<IShape>> clone_shapes() const;

		struct Snapshot
		{
			std::vector<ComPtr<IShape>> shapes;
			std::vector<Connector>      connectors;
		};

		std::vector<ComPtr<IShape>> m_shapes;
		std::vector<Connector>      m_connectors;

		std::vector<Snapshot> m_undo_stack;
		std::vector<Snapshot> m_redo_stack;
	};
}

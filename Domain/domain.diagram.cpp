module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

module domain.diagram;

import <algorithm>;

namespace domain
{
	using Microsoft::WRL::ComPtr;

	// ── Helpers ─────────────────────────────────────────────────────────────

	std::vector<ComPtr<IShape>> Diagram::clone_shapes() const
	{
		std::vector<ComPtr<IShape>> copy;
		copy.reserve(m_shapes.size());
		for (auto& s : m_shapes)
		{
			IShape* cloned = nullptr;
			if (SUCCEEDED(s->Clone(&cloned)) && cloned)
				copy.emplace_back(cloned);
		}
		return copy;
	}

	// ── Shapes ───────────────────────────────────────────────────────────────

	void Diagram::add_shape(ComPtr<IShape> shape)
	{
		if (shape) m_shapes.push_back(std::move(shape));
	}

	void Diagram::remove_shape(IShape* s)
	{
		std::size_t idx = index_of(s);

		m_shapes.erase(
			std::remove_if(m_shapes.begin(), m_shapes.end(),
				[s](const ComPtr<IShape>& p) { return p.Get() == s; }),
			m_shapes.end());

		if (idx != npos)
		{
			m_connectors.erase(
				std::remove_if(m_connectors.begin(), m_connectors.end(),
					[idx](const Connector& c)
					{ return c.source_index == idx || c.target_index == idx; }),
				m_connectors.end());

			for (auto& c : m_connectors)
			{
				if (c.source_index > idx) --c.source_index;
				if (c.target_index > idx) --c.target_index;
			}
		}
	}

	const std::vector<ComPtr<IShape>>& Diagram::shapes() const noexcept
	{
		return m_shapes;
	}

	std::vector<IShape*> Diagram::hit_test_all(float px, float py) const
	{
		std::vector<IShape*> result;
		for (auto it = m_shapes.rbegin(); it != m_shapes.rend(); ++it)
		{
			BOOL hit = FALSE;
			if (SUCCEEDED((*it)->HitTest(px, py, &hit)) && hit)
				result.push_back(it->Get());
		}
		return result;
	}

	void Diagram::move_shape(IShape* s, float dx, float dy)
	{
		s->Move(dx, dy);
	}

	bool Diagram::shape_intersects_rect(IShape* s,
										float x0, float y0,
										float x1, float y1) const
	{
		BOOL result = FALSE;
		s->IntersectsRect(x0, y0, x1, y1, &result);
		return result != FALSE;
	}

	std::size_t Diagram::index_of(const IShape* s) const noexcept
	{
		for (std::size_t i = 0; i < m_shapes.size(); ++i)
			if (m_shapes[i].Get() == s) return i;
		return npos;
	}

	// ── Connectors ───────────────────────────────────────────────────────────

	void Diagram::add_connector(std::size_t src, std::size_t tgt)
	{
		if (src < m_shapes.size() && tgt < m_shapes.size() && src != tgt)
			m_connectors.push_back({ src, tgt });
	}

	const std::vector<Connector>& Diagram::connectors() const noexcept
	{
		return m_connectors;
	}

	// ── Undo / redo ──────────────────────────────────────────────────────────

	void Diagram::commit()
	{
		if (static_cast<int>(m_undo_stack.size()) >= MAX_UNDO)
			m_undo_stack.erase(m_undo_stack.begin());

		m_undo_stack.push_back({ clone_shapes(), m_connectors });
		m_redo_stack.clear();
	}

	void Diagram::undo()
	{
		if (m_undo_stack.empty()) return;
		m_redo_stack.push_back({ clone_shapes(), m_connectors });
		m_shapes     = std::move(m_undo_stack.back().shapes);
		m_connectors = std::move(m_undo_stack.back().connectors);
		m_undo_stack.pop_back();
	}

	void Diagram::redo()
	{
		if (m_redo_stack.empty()) return;
		m_undo_stack.push_back({ clone_shapes(), m_connectors });
		m_shapes     = std::move(m_redo_stack.back().shapes);
		m_connectors = std::move(m_redo_stack.back().connectors);
		m_redo_stack.pop_back();
	}

	bool Diagram::can_undo() const noexcept { return !m_undo_stack.empty(); }
	bool Diagram::can_redo() const noexcept { return !m_redo_stack.empty(); }
}

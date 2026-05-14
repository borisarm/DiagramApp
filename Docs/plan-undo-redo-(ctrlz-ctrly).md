# 🎯 Undo / Redo (Ctrl+Z / Ctrl+Y)

## Understanding
Add undo/redo via a snapshot stack in `Diagram`. Every mutating operation commits a snapshot beforehand. `SelectTool` handles the keyboard shortcuts and clears stale selection pointers after a state swap.

## Assumptions
- Snapshot = full copy of `m_shapes` (cheap: small vector of value types)
- Max undo depth: 50 to bound memory
- `commit()` clears the redo stack (standard undo semantics)
- Tools call `commit()` right before: add_shape, remove_shape, and drag start
- After undo/redo, selection is cleared (pointers would be stale)
- `SelectTool` owns Ctrl+Z/Y handling; add tools ignore it (they're modal)

## Approach
1. `domain.diagram.ixx` — add `commit()`, `undo()`, `redo()`, `can_undo()`, `can_redo()` and private stacks
2. `domain.diagram.cpp` — implement the snapshot methods
3. `ui.win32.addrectangletool.cpp` — call `m_diagram.commit()` before `add_shape`
4. `ui.win32.addellipsetool.cpp` — call `m_diagram.commit()` before `add_shape`
5. `ui.win32.selecttool.cpp` — call `commit()` before delete/drag, handle Ctrl+Z/Ctrl+Y in `on_key_down`

## Key Files
- `Domain\domain.diagram.ixx` — public undo/redo API
- `Domain\domain.diagram.cpp` — snapshot stack implementation
- `UI.Win32\ui.win32.addrectangletool.cpp` — commit before add
- `UI.Win32\ui.win32.addellipsetool.cpp` — commit before add
- `UI.Win32\ui.win32.selecttool.cpp` — commit before delete/drag, Ctrl+Z/Y handlers

**Progress**: 33% [███░░░░░░░]

**Last Updated**: 2026-05-14 23:40:19

## 📝 Plan Steps
- ✅ **Update `domain.diagram.ixx` — add undo/redo declarations and private stacks**
- ✅ **Update `domain.diagram.cpp` — implement commit/undo/redo**
- 🔄 **Update `ui.win32.addrectangletool.cpp` — commit before add_shape**
-  **Update `ui.win32.addellipsetool.cpp` — commit before add_shape**
-  **Update `ui.win32.selecttool.cpp` — commit before delete and drag start; handle Ctrl+Z/Y**
-  **Build and fix any errors**


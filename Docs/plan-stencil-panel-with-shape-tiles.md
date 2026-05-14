# 🎯 Stencil Panel with Shape Tiles

## Understanding
Add a left-side stencil panel showing Rectangle and Ellipse tiles. Clicking a tile switches the active tool to the corresponding add tool. Dragging on the canvas creates a shape preview and commits it on release. Canvas shrinks to the right of the panel. Status bar reflects the active stencil selection.

## Assumptions
- Stencil panel is 90px wide, fixed to the left edge
- AddRectangleTool and AddEllipseTool don't exist yet — they must be created
- After committing a shape the add tool signals "done" so window.cpp can revert to SelectTool
- Preview overlay (dashed rect/ellipse) is rendered during drag using the same D2DContext data-sync pattern already established
- `d2d.ixx` imports `ui.win32.stencil` so `D2DContext` can hold `StencilItemKind` fields

## Key Files
- `UI.Win32\ui.win32.stencil.ixx/.cpp` — StencilItemKind enum + Stencil class (layout + hit-test, no D2D deps)
- `UI.Win32\ui.win32.addrectangletool.ixx/.cpp` — AddRectangleTool FSM
- `UI.Win32\ui.win32.addellipsetool.ixx/.cpp` — AddEllipseTool FSM
- `UI.Win32\ui.win32.d2d.ixx` — add stencil/preview fields, stencil_label_format, update canvas_rect
- `UI.Win32\ui.win32.d2d.cpp` — render stencil panel pass + preview overlay pass
- `UI.Win32\ui.win32.window.cpp` — wire stencil + add tools, handle stencil clicks, sync state

**Progress**: 100% [██████████]

**Last Updated**: 2026-05-14 23:02:46

## 📝 Plan Steps
- ✅ **Create ui.win32.stencil.ixx and ui.win32.stencil.cpp**
- ✅ **Create ui.win32.addrectangletool.ixx and ui.win32.addrectangletool.cpp**
- ✅ **Create ui.win32.addellipsetool.ixx and ui.win32.addellipsetool.cpp**
- ✅ **Update d2d.ixx — import stencil, add stencil/preview fields and label text format**
- ✅ **Update d2d.cpp — add stencil panel render pass and preview overlay pass, offset canvas_rect**
- ✅ **Update window.cpp — wire stencil, add tools, click routing, state sync**
- ✅ **Build and verify**


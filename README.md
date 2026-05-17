# DiagramApp

A modular, domain-driven diagramming engine built in modern C++20 with a Win32 shell, a Direct2D renderer, a Blender-style tool system, and a COM-like runtime shape plugin model.

---

## 🚀 Overview

DiagramApp is a **from-scratch diagramming engine** designed with:

- **C++20 modules** for clean compilation boundaries
- **Domain-Driven Design (DDD)** for purity and testability
- **A Win32 shell** (no XAML, no codegen, no hidden MSBuild magic)
- **A Direct2D / DirectWrite renderer** for fast, deterministic drawing
- **A Tool-based UI architecture** inspired by Blender, Figma, and CAD systems
- **A finite-state-machine (FSM) interaction model** for precise, predictable behavior
- **A COM-like runtime shape plugin system** for open-ended extensibility

The project is intentionally minimal, explicit, and deterministic.  
No global state leaks. No hidden frameworks. No magic.

---

## 🧱 Architecture

The solution is split into two Visual Studio projects:

```
Domain  (pure C++ — no Windows UI)
  shape_interfaces.h        COM-like ABI: IShapePlugin, IShapeClass, IShape
  domain.shape_impls        Built-in Rectangle and Ellipse COM implementations
  domain.shape_registry     Runtime plugin registry (in-process + DLL discovery)
  domain.diagram            Shape container, hit-testing, undo/redo, connectors

UI.Win32  (Win32 shell + Direct2D renderer + tools)
  ui.win32.window           Main window, message loop, tool/registry wiring
  ui.win32.d2d              Direct2D context, renderer, overlays, status bar
  ui.win32.stencil          Floating palette window (runtime-populated tiles)
  ui.win32.tool             ITool interface, PointerEvent, KeyEvent
  ui.win32.toolmanager      Active-tool dispatcher
  ui.win32.selecttool       Select / drag / lasso FSM
  ui.win32.addshapetool     Generic shape-placement FSM (IShapeClass-driven)
  ui.win32.addconnectortool Connector creation FSM
```

---

## 🔌 COM-like Shape Plugin Model

Shape types are **not hardcoded**. The engine discovers them at runtime through a
three-level COM-like ABI defined in `Domain/shape_interfaces.h`:

| Interface | Role |
|---|---|
| `IShapePlugin` | One per provider (DLL or in-process). Enumerates `IShapeClass` objects. |
| `IShapeClass` | One per shape type. Provides metadata (id, display name, stencil geometry) and acts as a factory (`CreateShape`). |
| `IShape` | One per diagram instance. Owns bounding box, hit-test, geometry, clone, and serialization. |

**Built-in shapes** (`domain.shape_impls`) are registered directly:

```cpp
IShapePlugin* p = nullptr;
builtin_rectangle_plugin(&p);
registry.register_plugin(p);
```

**External shapes** are loaded from DLLs that export `DllGetShapePlugin`:

```cpp
registry.discover(L"plugins\\");
```

The stencil palette is auto-populated from `registry.classes()` — no hardcoded enum anywhere.

---

## 🗂 Domain Layer

### `shape_interfaces.h`
The shared ABI header. Defines `ShapeGeometryKind` (`Rect`, `Ellipse`, `Path`),
`ShapeGeometry`, `ShapeBounds`, and the three COM interfaces above.
No C++ runtime types cross this boundary.

### `domain.shape_registry`
- `register_plugin(IShapePlugin*)` — in-process registration (built-ins)
- `discover(const wchar_t* directory)` — DLL scanning via `GetProcAddress`
- `classes()` — flat `vector<ComPtr<IShapeClass>>` driving the stencil
- `find_class(const wchar_t* id)` — deserialization lookup

### `domain.diagram`
- Stores `vector<ComPtr<IShape>>`
- `add_shape`, `remove_shape`, `hit_test_all`, `move_shape`, `shape_intersects_rect`
- `index_of(const IShape*)` — connector indexing
- `connectors()` — source/target index pairs
- `commit` / `undo` / `redo` — snapshot-based undo stack (deep-copies via `IShape::Clone()`)

---

## 🎮 Interaction Model

The UI is driven by a hierarchical state machine:

- **Global mode** — which tool is active (`ToolManager`)
- **Tool FSM** — per-tool interaction logic
  - `SelectTool`: Idle → PressedOnShape → Dragging / LassoSelecting → Selected
  - `AddShapeTool`: Idle → Dragging → Done (calls `IShapeClass::CreateShape`)
  - `AddConnectorTool`: Idle → SourceLocked → Done (records source/target index)

All tools receive abstract `PointerEvent` and `KeyEvent` — no Win32 types in tool logic.

### Keyboard shortcuts

| Key | Action |
|---|---|
| `Del` / `Backspace` | Delete selected shapes |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+A` | Select all |
| `Esc` | Deselect |

---

## 🖼 Rendering

`ui.win32.d2d` owns the Direct2D context and renders in a single `render()` call:

1. All diagram shapes via `IShape::GetGeometry()` — renderer is shape-type agnostic
2. Selection highlights (orange fill + stroke)
3. Lasso rectangle (dashed blue)
4. Committed connectors with arrowheads
5. Connector drag preview (dashed orange line)
6. Shape placement preview (dashed blue rectangle or ellipse)
7. Status bar (tool name, active stencil tile, shape count, selection count)

The renderer **never mutates the domain**.

---

## 🖼 Stencil Palette

A floating Win32 child window (`ui.win32.stencil`) shows one tile per registered shape class plus a Connector sentinel tile. Tiles are painted using `IShapeClass::GetStencilGeometry()` — no hardcoded icons. Clicking a tile activates the matching tool.

---

## 🧪 Testing Philosophy

- Domain is fully testable without UI — `IShape` / `IShapeClass` are pure interfaces
- Tools are testable via event simulation
- Renderer is deterministic and stateless
- Shape plugins can be validated in isolation before loading

---

## 🛠 Build Requirements

- **Visual Studio 2026 or later**
- **C++20 modules enabled** (`/std:c++20`)
- **Direct2D / DirectWrite** (Windows SDK)
- **WRL** (`wrl/client.h`) for `ComPtr`
- No external dependencies

---

## 📦 Implemented Features

- [x] Diagram container with `ComPtr<IShape>` storage
- [x] COM-like shape plugin ABI (`IShapePlugin` / `IShapeClass` / `IShape`)
- [x] Built-in Rectangle and Ellipse shape implementations
- [x] Runtime shape registry with DLL discovery
- [x] Runtime-populated stencil palette
- [x] Select tool (click, drag-move, lasso, multi-select, Ctrl-toggle)
- [x] Generic add-shape tool (drag to place any `IShapeClass`)
- [x] Connector tool (click source → click target, arrowhead rendering)
- [x] Delete selected shapes
- [x] Snapshot undo / redo (`Ctrl+Z` / `Ctrl+Y`)
- [x] Status bar (tool, stencil, shape count, selection count)
- [x] Floating stencil palette window

## 🗺 Roadmap

- [ ] Zoom and pan
- [ ] Shape resize handles
- [ ] Grouping / composite shapes
- [ ] Boolean operations
- [ ] Layers
- [ ] Serialization / deserialization (JSON via `IShape::Serialize`)
- [ ] External shape plugin DLLs
- [ ] Snapping and alignment guides
- [ ] Text labels on shapes

---

## 🤝 Contributing

Contributions follow the **Spec-Driven Development** workflow:

- Specs define behavior
- Tools implement FSMs
- Domain remains pure and UI-agnostic
- Renderer remains stateless
- Shape types are added as plugins, not hardcoded

---

## 📄 License

MIT License (or your preferred license).


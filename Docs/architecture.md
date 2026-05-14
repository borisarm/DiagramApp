# Architecture Overview  
DiagramApp — A Modular, Domain‑Driven Diagramming Engine in C++20

This document describes the architectural structure, boundaries, and interaction flows of DiagramApp.  
It is intended for contributors, maintainers, and automated agents participating in the development workflow.

---

## 1. Architectural Goals

DiagramApp is designed around the following principles:

- **Domain‑Driven Design (DDD)**  
  The domain is pure, deterministic, and independent of UI or rendering.

- **Strict Layering**  
  Each layer depends only on the layer below it. No cycles.

- **C++20 Modules**  
  All boundaries are enforced at compile time.

- **Explicitness Over Magic**  
  No hidden codegen, no XAML, no WinRT projections, no global state leaks.

- **Tool‑Based Interaction Model**  
  Inspired by Blender, Figma, and CAD systems.  
  Each tool is a self‑contained finite state machine (FSM).

- **Deterministic Rendering**  
  Direct2D renderer is stateless and driven entirely by domain data.

- **Agent‑Friendly Structure**  
  Clear module boundaries, explicit responsibilities, and predictable behavior.

---

## 2. High‑Level Architecture

```
+-------------------------+
|        UI.Win32         |
|  - Window shell         |
|  - Event dispatch       |
|  - ToolManager          |
+-----------+-------------+
            |
            v
+-------------------------+
|          Tools          |
|  - SelectTool (FSM)     |
|  - AddShape tools       |
|  - Composite tools      |
|  - Layering tools       |
|  - Connector tools      |
+-----------+-------------+
            |
            v
+-------------------------+
|      Infrastructure     |
|  - Direct2D renderer    |
|  - Device resources     |
|  - Rendering context    |
+-----------+-------------+
            |
            v
+-------------------------+
|         Domain          |
|  - Shapes               |
|  - Diagram              |
|  - Hit-testing          |
|  - Geometry             |
+-------------------------+
```

Each layer depends only on the one below it.  
The domain depends on nothing.

---

## 3. Domain Layer

The **domain** is the core of the application.  
It contains:

- Shape variants (`Rectangle`, `Ellipse`, `Composite`, etc.)
- `Diagram` (collection of shapes)
- Hit‑testing
- Geometry utilities
- Mutation operations (move, resize, group, etc.)

### Domain Rules

- No UI code  
- No rendering code  
- No Win32 or Direct2D  
- No global state  
- Pure C++20 modules  
- Deterministic behavior  

The domain is fully testable in isolation.

---

## 4. Infrastructure Layer (Rendering)

The **infrastructure** layer provides:

- Direct2D device creation  
- Render target management  
- Shape‑specific drawing  
- Overlay drawing for tools  
- Device‑independent resources  

### Responsibilities

- Convert domain shapes into Direct2D primitives  
- Maintain rendering resources  
- Draw overlays (lasso, previews, handles)  
- Remain stateless between frames  

### Non‑Responsibilities

- No input handling  
- No selection logic  
- No domain mutation  

The renderer is a pure consumer of domain data.

---

## 5. UI.Win32 Layer (Shell)

The **UI.Win32** layer is the minimal Win32 shell:

- Creates the window  
- Owns the message loop  
- Dispatches input events  
- Owns the global `Diagram` instance  
- Owns the `ToolManager`  
- Passes domain pointers to the renderer  

### Responsibilities

- Translate Win32 events into abstract events  
- Forward events to the active tool  
- Trigger redraws  
- Manage window lifecycle  

### Non‑Responsibilities

- No domain logic  
- No rendering logic  
- No tool logic  

The shell is intentionally thin.

---

## 6. Tools Layer (Modal Interaction System)

Tools are the heart of the interaction model.  
Each tool is a **self‑contained finite state machine (FSM)**.

### Tool Interface

All tools implement:

- `on_pointer_down`
- `on_pointer_move`
- `on_pointer_up`
- `on_key_down`
- `on_key_up`
- `update`
- `render_overlays`
- `on_activate`
- `on_deactivate`

### ToolManager

The `ToolManager`:

- Owns the active tool  
- Routes events to it  
- Handles tool switching  
- Calls `update()` and `render_overlays()`  

### Example Tools

- **SelectTool**  
  Full FSM: press, drag, lasso, multi‑select, toggle, etc.

- **AddRectangleTool**  
  Click‑drag to create shapes.

- **CompositeTool**  
  Build composite shapes from selections.

- **SubtractTool**  
  Boolean subtraction.

- **LayerTool**  
  Bring forward/back, send to front/back.

- **ConnectorTool**  
  Create connectors between shapes.

Each tool is isolated and testable.

---

## 7. Event Flow

### Pointer Event Flow

```
Win32 → UI.Win32 → ToolManager → ActiveTool → Domain → Renderer
```

### Rendering Flow

```
UI.Win32 → Renderer → (Domain + Tool Overlays)
```

### Domain Mutation Flow

```
Tool → Domain → UI.Win32 invalidates → Renderer redraws
```

---

## 8. Finite State Machines (FSMs)

FSMs exist at multiple levels:

### Global Mode FSM
Which tool is active.

### Tool FSM
Each tool defines its own interaction states.

### Selection FSM
Inside SelectTool:
- Idle
- PressedOnShape
- Dragging
- LassoSelecting
- Selected

FSMs ensure deterministic, predictable behavior.

---

## 9. Extensibility

The architecture supports:

- New shapes  
- New tools  
- New rendering primitives  
- New domain operations  
- Undo/redo  
- Zoom/pan  
- Layers  
- Connectors  
- Composite shapes  
- Boolean operations  
- Serialization  

All without breaking existing modules.

---

## 10. Design Philosophy

- **Explicit boundaries**  
- **No hidden magic**  
- **No global state leaks**  
- **Deterministic behavior**  
- **Testable components**  
- **Agent‑friendly structure**  
- **Long‑term maintainability**  

DiagramApp is built to scale — in features, complexity, and contributors.

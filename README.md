# DiagramApp  
A modular, domain‑driven diagramming engine built in modern C++20 with a clean Win32 shell, a Direct2D renderer, and a Blender‑style tool system.

---

## 🚀 Overview

DiagramApp is a **from‑scratch diagramming engine** designed with:

- **C++20 modules** for clean boundaries  
- **Domain‑Driven Design (DDD)** for purity and testability  
- **A Win32 AppWindow shell** (no XAML, no codegen, no hidden MSBuild magic)  
- **A Direct2D renderer** for fast, deterministic drawing  
- **A Tool‑based UI architecture** inspired by Blender, Figma, and CAD systems  
- **A finite‑state‑machine (FSM) interaction model** for precise, predictable behavior  

The project is intentionally minimal, explicit, and deterministic.  
No global state leaks. No hidden frameworks. No magic.

---

## 🧱 Architecture

The solution is organized into four major layers:

```
Domain (pure C++)
↑
Infrastructure (rendering, utilities)
↑
UI.Win32 (window shell, event dispatch)
↑
Tools (modal interaction system)
```

### **Domain**
Pure C++20 modules containing:

- `Shape` variants (Rectangle, Ellipse, Composite, etc.)
- `Diagram` (collection of shapes)
- Hit‑testing
- Geometry utilities
- Mutation operations (move, resize, group, etc.)

The domain is **UI‑agnostic** and **renderer‑agnostic**.

### **Infrastructure**
Contains:

- Direct2D context creation
- Rendering primitives
- Device‑independent resources

This layer knows the domain but not the UI.

### **UI.Win32**
A minimal Win32 shell that:

- Creates the window  
- Owns the main message loop  
- Dispatches input events  
- Owns the global `Diagram` instance  
- Passes domain pointers into the renderer  

No XAML. No WinRT projections. No hidden codegen.

### **Tools**
A Blender‑style modal system:

- Each tool is a **self‑contained FSM**
- Tools receive abstract events (`PointerEvent`, `KeyEvent`)
- Tools manipulate the domain
- Tools draw overlays (lasso, previews, handles)
- Tools are activated/deactivated by the `ToolManager`

Current tools:

- **SelectTool** (full FSM: press, drag, lasso, multi‑select)
- More tools coming (AddRectangleTool, CompositeTool, SubtractTool, LayerTool…)

---

## 🎮 Interaction Model

The UI is driven by a **hierarchical state machine**:

- **Global Mode** (which tool is active)
- **Tool FSM** (per‑tool interaction logic)
- **Selection FSM** (inside SelectTool)
- **Dragging FSM**
- **Lasso FSM**

This architecture scales to:

- grouping  
- boolean operations  
- connectors  
- layering  
- text editing  
- composite shapes  
- multi‑selection  
- snapping  
- constraints  

---

## 🖼 Rendering

Rendering is handled by a dedicated Direct2D context:

- Device‑independent resources  
- Shape‑specific drawing  
- Overlay rendering for tools  
- Deterministic, stateless draw calls  

The renderer never mutates the domain.

---

## 🧪 Testing Philosophy

- Domain is fully testable without UI  
- Tools are testable via event simulation  
- Renderer is deterministic and stateless  
- No global state except the diagram instance owned by the window  

---

## 🛠 Build Requirements

- **Visual Studio 2022**  
- **C++20 modules enabled**  
- **Windows App SDK (for AppWindow)**  
- **Direct2D / DirectWrite**  

---

## 📦 Project Goals

- Build a clean, modular diagramming engine  
- Maintain strict architectural purity  
- Provide a foundation for agent‑driven development  
- Enable future features:
  - connectors
  - grouping
  - composite shapes
  - boolean operations
  - layers
  - zoom/pan
  - undo/redo
  - serialization

---

## 🤝 Contributing

Contributions follow the **Spec‑Driven Development** workflow:

- Specs define behavior  
- Tools implement FSMs  
- Domain remains pure  
- Renderer remains stateless  
- UI remains minimal  

---

## 📄 License

MIT License (or your preferred license).


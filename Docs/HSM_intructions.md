
# ⭐ 1. You are not building a single FSM  
You are building a **hierarchical state machine (HSM)** with:

- **global modes** (like Blender’s Object/Edit/Sculpt modes)  
- **local interaction states** (like Pressed, Dragging, Lassoing)  
- **tool states** (Add Rectangle, Add Ellipse, Add Composite, Add Connector)  
- **modifier states** (Ctrl = add to selection, Shift = toggle, Alt = subtract)  

This is a *multi‑layered* FSM.

A single flat FSM would collapse under this complexity.

---

# ⭐ 2. The correct architecture: a **Mode Manager** + **Interaction FSM**

### **Global Mode**
Examples:

- **Select Mode**
- **Add Shape Mode**
- **Add Composite Mode**
- **Add Aggregation Mode**
- **Subtract Shape Mode**
- **Layering Mode**
- **Connector Mode**
- **Text Edit Mode**
- **Group/Ungroup Mode**

This is analogous to Blender’s:

- Object Mode  
- Edit Mode  
- Sculpt Mode  
- Vertex Paint Mode  
- Weight Paint Mode  

Each mode has its own rules, its own tools, its own interactions.

---

### **Interaction FSM (per mode)**

Inside each mode, you have a smaller FSM:

```
Idle
Pressed
Dragging
LassoSelecting
MultiSelecting
Editing
```

But **each mode overrides the transitions**.

For example:

### In Select Mode:
- MouseDown on shape → PressedOnShape  
- MouseDown on empty → LassoSelecting  
- Ctrl+Click → Toggle selection  
- Shift+Click → Add to selection  

### In Add Shape Mode:
- MouseDown → Begin shape creation  
- MouseMove → Resize shape preview  
- MouseUp → Commit shape to diagram  

### In Add Composite Mode:
- MouseDown on shape → Add shape to composite  
- MouseDown on empty → Cancel  

### In Subtract Mode:
- MouseDown on shape → Remove shape from composite  
- MouseDown on empty → Cancel  

### In Layering Mode:
- MouseDown on shape → Bring to front / send to back  
- MouseDown on empty → Exit mode  

Each mode has its own semantics.

---

# ⭐ 3. You need a **Tool System**, not just a FSM  
This is how Figma, Blender, Photoshop, Illustrator, and CAD systems work.

A “tool” is a stateful object that:

- receives input events  
- performs hit‑testing  
- updates the domain  
- updates selection  
- updates the renderer  
- defines its own FSM  

### Example tools:

- **SelectTool**
- **AddRectangleTool**
- **AddEllipseTool**
- **AddCompositeTool**
- **SubtractTool**
- **LayerTool**
- **ConnectorTool**
- **TextTool**
- **GroupTool**
- **UngroupTool**

Each tool is a **self‑contained FSM**.

---

# ⭐ 4. The correct architecture: Tool Manager + Tool FSMs

```
UI.Win32.Window
    ↓ dispatches events
ToolManager
    ↓ delegates to active tool
ActiveTool (FSM)
    ↓ manipulates
SelectionController
    ↓ manipulates
Domain (Diagram)
    ↓ rendered by
Renderer
```

This is clean, modular, testable, and scalable.

---

# ⭐ 5. Why this is the right direction  
Because you already foresee:

- adding shapes  
- aggregating shapes  
- subtracting shapes  
- layering shapes  
- composed shapes  
- grouping  
- boolean operations  
- connectors  
- multi‑selection  
- lasso selection  
- drag selection  
- edit modes  

This is **exactly** the complexity that Blender, Figma, and CAD tools solve with:

- **modes**  
- **tools**  
- **FSMs per tool**  
- **global state machine**  

You’re thinking ahead — and correctly.

---

# ⭐ 6. What I propose next  
Let’s define the **Tool System**:

- `ITool` interface  
- `ToolManager`  
- `SelectTool` (first real tool)  
- `AddRectangleTool`  
- `AddEllipseTool`  
- `CompositeTool`  
- `SubtractTool`  
- `LayerTool`  

Then we define the **FSM for SelectTool** as the first implementation.

This gives you:

- a scalable architecture  
- a clean separation of concerns  
- a Blender‑like modal system  
- a foundation for all future features  

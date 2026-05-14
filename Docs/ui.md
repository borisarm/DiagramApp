# UI Layer (Win32 Shell)
Minimal, Deterministic, Event‑Driven, Tool‑Oriented

The **UI layer** is the thinnest layer in DiagramApp.  
It provides a minimal Win32 shell responsible for:

- creating the application window  
- dispatching input events  
- managing the message loop  
- routing events to the ToolManager  
- triggering redraws  
- owning the global Diagram instance  
- owning the D2DContext (renderer state)  

The UI layer contains **no domain logic**, **no rendering logic**, and **no tool logic**.  
Its only job is to translate OS events into abstract events and forward them.

---

## 1. Design Principles

### **1. Minimalism**
The UI layer is intentionally tiny.  
It does not interpret input — it only forwards it.

### **2. Deterministic Event Routing**
All input events follow the same path:

```
Win32 → UI.Win32 → ToolManager → ActiveTool → Domain → Renderer
```

### **3. No Hidden Frameworks**
- No XAML  
- No WinRT projections  
- No message pumps hidden behind frameworks  
- No code generation  

### **4. Explicit Ownership**
The UI layer owns:

- the main window  
- the message loop  
- the `Diagram` instance  
- the `D2DContext`  
- the `ToolManager`  

### **5. Pure Translation**
Win32 events are translated into:

- `PointerEvent`
- `KeyEvent`

These are passed to the active tool.

---

## 2. Window Lifecycle

The UI layer creates a Win32 window using:

- `AppWindow` (Windows App SDK)  
- a custom window procedure (`WndProc`)  

### Responsibilities

- Create the window  
- Initialize Direct2D resources  
- Initialize the ToolManager  
- Initialize the Diagram  
- Enter the message loop  

### Non‑Responsibilities

- No domain logic  
- No rendering logic  
- No tool logic  

---

## 3. Message Loop

The message loop is a standard Win32 loop:

```cpp
while (GetMessage(&msg, nullptr, 0, 0))
{
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}
```

The loop does not contain any application logic.  
All logic is handled by tools and the renderer.

---

## 4. Event Routing

The window procedure (`WndProc`) receives raw Win32 messages:

- `WM_LBUTTONDOWN`
- `WM_MOUSEMOVE`
- `WM_LBUTTONUP`
- `WM_KEYDOWN`
- `WM_KEYUP`
- `WM_SIZE`
- `WM_PAINT`

These are translated into abstract events:

### PointerEvent

```cpp
struct PointerEvent
{
    float x;
    float y;
    bool left;
    bool right;
    bool shift;
    bool ctrl;
    bool alt;
};
```

### KeyEvent

```cpp
struct KeyEvent
{
    int key;
    bool shift;
    bool ctrl;
    bool alt;
};
```

### Event Flow

```
WndProc → ToolManager → ActiveTool → Domain → UI invalidates → Renderer redraws
```

The UI layer never interprets the meaning of events.

---

## 5. ToolManager Integration

The UI layer owns a `ToolManager` instance.

### On pointer events:

```cpp
tool_manager.dispatch_pointer_down(e);
tool_manager.dispatch_pointer_move(e);
tool_manager.dispatch_pointer_up(e);
```

### On key events:

```cpp
tool_manager.dispatch_key_down(e);
tool_manager.dispatch_key_up(e);
```

### On each frame:

```cpp
tool_manager.update();
tool_manager.render_overlays();
```

The UI layer does not know which tool is active or what it does.

---

## 6. Diagram Ownership

The UI layer owns the global `Diagram` instance:

```cpp
domain::Diagram g_diagram;
```

This instance is passed to:

- the renderer (read‑only)
- the active tool (read/write)

The UI layer does not modify the diagram.

---

## 7. Rendering Integration

Rendering is triggered by:

- `WM_PAINT`
- `InvalidateRect` calls from tools or UI

### Rendering Flow

```
WndProc (WM_PAINT)
    ↓
D2DContext.BeginDraw()
    ↓
Renderer draws domain shapes
    ↓
Renderer draws tool overlays
    ↓
D2DContext.EndDraw()
```

The UI layer does not draw anything itself.

---

## 8. Resize Handling

On `WM_SIZE`:

1. The UI layer resizes the Direct2D render target  
2. The renderer uses the new size on the next frame  
3. No domain or tool state is affected  

---

## 9. Input Model

The UI layer normalizes Win32 input into a consistent model:

### Pointer Input

- absolute coordinates  
- left/right button state  
- modifier keys  
- no double‑click logic (tools decide)  
- no drag logic (tools decide)  

### Keyboard Input

- raw key codes  
- modifier state  
- no text input (future TextTool will handle this)  

---

## 10. Responsibilities Summary

### The UI layer **does**:

- create the window  
- manage the message loop  
- translate Win32 events  
- dispatch events to tools  
- trigger redraws  
- own the diagram  
- own the renderer context  

### The UI layer **does not**:

- interpret input  
- manage selection  
- move shapes  
- draw shapes  
- draw overlays  
- manage tools  
- mutate the domain  

---

## 11. Future Extensions

The UI layer is designed to support:

- DPI scaling  
- multi‑window support  
- detachable tool panels  
- keyboard shortcuts  
- command palette  
- status bar  
- zoom/pan gestures  
- touch input  

All without breaking existing architecture.

---

## 12. Summary

The UI layer is:

- minimal  
- deterministic  
- event‑driven  
- tool‑oriented  
- domain‑agnostic  
- renderer‑agnostic  

It is the glue between the OS and the application logic, and nothing more.

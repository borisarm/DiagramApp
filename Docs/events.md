# Event Model  
Unified, Abstract, Deterministic Event Routing for Tools and UI

DiagramApp uses a **clean, abstract event model** that decouples the Win32 shell from the tool system and the domain.  
This document describes how events are represented, how they flow through the system, and how tools consume them.

---

## 1. Design Goals

The event system is designed to be:

### **1. Abstract**
Tools never see raw Win32 messages.  
They receive normalized, platform‑agnostic events.

### **2. Deterministic**
Event ordering and semantics are consistent across platforms and tools.

### **3. Minimal**
Only essential information is included in events.

### **4. Tool‑Driven**
Events are routed to the active tool, which owns the interaction FSM.

### **5. Extensible**
Future input types (touch, pen, gestures) can be added without breaking tools.

---

## 2. Event Flow Overview

The event pipeline is:

```
Win32 → UI.Win32 → ToolManager → ActiveTool → Domain → UI invalidates → Renderer
```

### Breakdown:

1. **Win32**  
   Receives raw OS messages (`WM_LBUTTONDOWN`, `WM_MOUSEMOVE`, etc.)

2. **UI.Win32**  
   Translates them into abstract events (`PointerEvent`, `KeyEvent`).

3. **ToolManager**  
   Dispatches events to the active tool.

4. **ActiveTool**  
   Runs its FSM, mutates the domain, requests redraws.

5. **Domain**  
   Updates shapes, selection, geometry.

6. **UI.Win32**  
   Invalidates the window.

7. **Renderer**  
   Draws domain + overlays.

---

## 3. Abstract Event Types

The UI layer defines two event types:

### 3.1 PointerEvent

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

Represents:

- mouse position  
- button state  
- modifier keys  

### 3.2 KeyEvent

```cpp
struct KeyEvent
{
    int key;
    bool shift;
    bool ctrl;
    bool alt;
};
```

Represents:

- raw key code  
- modifier state  

---

## 4. Event Translation (Win32 → Abstract)

The UI layer converts Win32 messages into abstract events.

### Pointer Down

```
WM_LBUTTONDOWN → PointerEvent { left = true }
WM_RBUTTONDOWN → PointerEvent { right = true }
```

### Pointer Move

```
WM_MOUSEMOVE → PointerEvent { x, y, left, right }
```

### Pointer Up

```
WM_LBUTTONUP → PointerEvent { left = false }
WM_RBUTTONUP → PointerEvent { right = false }
```

### Key Down

```
WM_KEYDOWN → KeyEvent { key, shift, ctrl, alt }
```

### Key Up

```
WM_KEYUP → KeyEvent { key, shift, ctrl, alt }
```

### Resize

```
WM_SIZE → triggers render target resize
```

### Paint

```
WM_PAINT → triggers rendering
```

The UI layer does **not** interpret the meaning of events.

---

## 5. ToolManager Dispatch

The UI layer forwards events to the ToolManager:

```cpp
tool_manager.dispatch_pointer_down(e);
tool_manager.dispatch_pointer_move(e);
tool_manager.dispatch_pointer_up(e);

tool_manager.dispatch_key_down(e);
tool_manager.dispatch_key_up(e);
```

The ToolManager then forwards them to the active tool:

```cpp
active_tool->on_pointer_down(e);
active_tool->on_pointer_move(e);
active_tool->on_pointer_up(e);

active_tool->on_key_down(e);
active_tool->on_key_up(e);
```

The ToolManager does **not** interpret events.

---

## 6. Tool Event Handling

Each tool implements:

```cpp
void on_pointer_down(const PointerEvent&);
void on_pointer_move(const PointerEvent&);
void on_pointer_up(const PointerEvent&);

void on_key_down(const KeyEvent&);
void on_key_up(const KeyEvent&);
```

Tools:

- run their own FSM  
- mutate the domain  
- request redraws  
- draw overlays  

Examples:

### SelectTool

- press → pressed state  
- move → drag or lasso  
- release → commit selection  

### AddRectangleTool

- press → start rectangle  
- move → preview  
- release → commit  

### CompositeTool

- click shapes → add to composite  
- enter → commit  
- escape → cancel  

---

## 7. Redraw Requests

Tools do not draw directly.  
They request redraws by calling:

```cpp
InvalidateRect(hwnd, nullptr, FALSE);
```

The UI layer then triggers a `WM_PAINT`, which:

- begins a Direct2D draw pass  
- calls the renderer  
- draws domain shapes  
- draws tool overlays  

---

## 8. Event Ordering Guarantees

The system guarantees:

- pointer events are delivered in order  
- key events are delivered in order  
- no event is lost  
- no event is duplicated  
- tools receive consistent modifier state  
- tools receive consistent pointer state  

This ensures deterministic behavior.

---

## 9. Future Extensions

The event system is designed to support:

### **1. Touch Input**
- multi‑touch  
- pinch zoom  
- pan gestures  

### **2. Pen Input**
- pressure  
- tilt  
- eraser  

### **3. Gesture Events**
- zoom  
- rotate  
- swipe  

### **4. Command Events**
- undo/redo  
- copy/paste  
- tool switching  

### **5. UI Events**
- menu actions  
- toolbar actions  
- context menus  

All without breaking existing tools.

---

## 10. Summary

The event system is:

- abstract  
- deterministic  
- minimal  
- tool‑driven  
- domain‑agnostic  
- renderer‑agnostic  

It provides a clean separation between:

- OS events  
- UI shell  
- tool FSMs  
- domain mutations  
- rendering  

This is the backbone of DiagramApp’s interaction model.


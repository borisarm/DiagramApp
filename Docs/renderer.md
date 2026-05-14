# Renderer Architecture  
Direct2D‑Based, Stateless, Deterministic Rendering Pipeline

The renderer is part of the **Infrastructure** layer.  
It is responsible for converting domain shapes into Direct2D draw calls and rendering tool overlays.  
It is intentionally **stateless**, **deterministic**, and **domain‑driven**.

This document describes the renderer’s structure, responsibilities, and integration with the rest of the system.

---

## 1. Design Principles

The renderer follows these principles:

### **1. Stateless**
The renderer does not store domain state.  
It receives all required data each frame.

### **2. Deterministic**
Rendering is a pure function of:
- the domain (`Diagram`)
- the active tool’s overlays
- the current window size

### **3. Layered**
The renderer depends on:
- Direct2D
- Domain shapes

It does **not** depend on:
- Tools
- UI.Win32
- Input events
- Global state

### **4. Device‑Independent Resources**
Brushes, stroke styles, and text formats are created once and reused.

### **5. Separation of Concerns**
- Domain owns shapes  
- Tools own overlays  
- Renderer draws both  

---

## 2. Rendering Pipeline Overview

The rendering pipeline consists of:

```
UI.Win32 → D2DContext → Renderer → Direct2D
```

### Steps per frame:

1. **UI.Win32** invalidates the window  
2. **D2DContext** begins a Direct2D draw pass  
3. **Renderer** draws:
   - domain shapes
   - tool overlays
4. **D2DContext** ends the draw pass  
5. Direct2D presents the frame  

The renderer does not retain any state between frames.

---

## 3. D2DContext

`D2DContext` is a lightweight struct that owns:

- `ID2D1Factory`
- `ID2D1HwndRenderTarget`
- `ID2D1SolidColorBrush` instances
- Device‑independent resources

It also stores:

- pointer to the domain `Diagram`
- pointer to the active tool (for overlays)
- window size

### Responsibilities

- Initialize Direct2D resources  
- Handle resize events  
- Begin/end draw passes  
- Provide drawing helpers  

### Non‑Responsibilities

- No domain logic  
- No selection logic  
- No tool logic  

---

## 4. Renderer Responsibilities

The renderer performs three tasks:

### **1. Draw domain shapes**
Each shape type has a dedicated draw function:

```cpp
void draw_rectangle(const Rectangle& r);
void draw_ellipse(const Ellipse& e);
```

Shapes are drawn in **z‑order**, back to front.

### **2. Draw selection highlights**
The renderer receives a list of selected shapes from the active tool.

Highlights are drawn as:

- stroked outlines  
- semi‑transparent overlays  
- handles (future)  

### **3. Draw tool overlays**
Tools may draw:

- lasso rectangles  
- shape previews  
- drag previews  
- connector previews  
- composite outlines  

The renderer calls:

```cpp
active_tool->render_overlays();
```

Tools do not draw directly; they request drawing through the context.

---

## 5. Rendering Flow in Detail

### 5.1 Begin Draw

```cpp
ctx.target->BeginDraw();
ctx.target->Clear(background_color);
```

### 5.2 Draw Shapes

```cpp
for (auto& shape : diagram.shapes())
    std::visit([&](auto&& s) { draw_shape(s); }, shape);
```

### 5.3 Draw Selection

```cpp
for (auto* s : selected_shapes)
    draw_selection_outline(*s);
```

### 5.4 Draw Tool Overlays

```cpp
active_tool->render_overlays(ctx);
```

### 5.5 End Draw

```cpp
ctx.target->EndDraw();
```

---

## 6. Shape Rendering

### Rectangle

```cpp
ctx.target->FillRectangle(rect, fill_brush);
ctx.target->DrawRectangle(rect, stroke_brush, stroke_width);
```

### Ellipse

```cpp
ctx.target->FillEllipse(ellipse, fill_brush);
ctx.target->DrawEllipse(ellipse, stroke_brush, stroke_width);
```

### Composite Shapes (future)

Composite shapes will be rendered by:

- iterating child shapes  
- applying transforms  
- drawing bounding boxes  

### Connectors (future)

Connectors will be rendered using:

- `ID2D1PathGeometry`
- `ID2D1GeometrySink`

---

## 7. Overlays

Overlays are drawn **after** domain shapes and **before** selection outlines.

Examples:

### Lasso Rectangle

```cpp
ctx.target->DrawRectangle(lasso_rect, overlay_brush, 1.0f);
```

### Shape Preview

```cpp
ctx.target->DrawRectangle(preview_rect, preview_brush, 1.5f);
```

### Drag Preview

```cpp
ctx.target->DrawRectangle(drag_rect, drag_brush, 1.0f);
```

Overlays never mutate the domain.

---

## 8. Device Resources

The renderer creates:

- Solid color brushes  
- Stroke styles  
- Text formats (future)  
- Path geometries (future)  

Resources are created once and reused.

### Resource Lifetime

- Created on context initialization  
- Recreated on device loss  
- Released on shutdown  

---

## 9. Resize Handling

When the window resizes:

1. `WM_SIZE` triggers a resize event  
2. `D2DContext` resizes the render target  
3. Renderer uses the new size on the next frame  

No domain or tool state is affected.

---

## 10. Future Extensions

The renderer is designed to support:

### **1. Layers**
- Per‑layer opacity  
- Per‑layer visibility  
- Layer ordering  

### **2. Composite Shapes**
- Nested transforms  
- Clipping  
- Boolean operations  

### **3. Connectors**
- Arrowheads  
- Routing algorithms  
- Orthogonal connectors  

### **4. Text Rendering**
- DirectWrite integration  
- Rich text shapes  

### **5. Zoom & Pan**
- World → screen transforms  
- DPI scaling  

### **6. Hit‑Testing Visualization**
- Debug overlays  
- Bounding boxes  

---

## 11. Summary

The renderer is:

- **stateless**
- **deterministic**
- **domain‑driven**
- **tool‑aware**
- **UI‑agnostic**
- **Direct2D‑based**

It draws:

- domain shapes  
- selection outlines  
- tool overlays  

It does not:

- mutate the domain  
- handle input  
- manage tools  
- own application state  

It is a pure rendering engine.


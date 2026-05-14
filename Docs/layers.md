# **Layers**

## **1. Purpose**
Layers provide **structured, deterministic organization** of diagram content.  
They allow the editor to separate concerns such as background, grid, shapes, annotations, and temporary UI overlays.

Layers are implemented as **Layered Composites** in the domain model.  
They are not a UI feature; they are a **core domain abstraction** with full lifecycle, events, and invariants.

---

## **2. Core Principles**

### **2.1 Domain‑Level Concept**
- A layer is a **Composite** with additional semantics.  
- The diagram root is a **LayeredComposite** containing multiple layers.  
- Layers are ordered and the order is meaningful.

### **2.2 Deterministic Rendering**
- Layers define strict z‑order.  
- Rendering always traverses layers from bottom to top.  
- Hit‑testing always traverses layers from top to bottom.

### **2.3 Isolation**
- Shapes belong to exactly one layer.  
- Tools operate on the **active layer** unless explicitly in cross‑layer mode.  
- Layers cannot contain other layers (no nesting).

---

## **3. Layer Types**

### **3.1 Background Layer**
- Lowest z‑order  
- Non‑interactive  
- Used for:
  - Background color  
  - Background images  
  - Watermarks  

### **3.2 Grid Layer**
- Optional  
- Non‑interactive  
- Drawn using renderer primitives  
- Does not contain shapes  

### **3.3 Shapes Layer**
- Default layer for user‑created shapes  
- Fully interactive  
- Supports composites, grouping, boolean shapes, etc.

### **3.4 Annotation Layer**
- Contains notes, comments, guides, markers  
- Interactive but lower priority than shapes  
- Often semi‑transparent

### **3.5 Overlay Layer**
- Highest z‑order  
- Temporary UI elements (selection boxes, handles, previews)  
- Not persisted  
- Not part of the domain model (renderer‑only)

---

## **4. Invariants**

### **4.1 Structural**
- Layers are stored in a deterministic order.  
- A shape must belong to exactly one layer.  
- A layer cannot be empty if it is required (e.g., background).  
- No cycles (layers cannot contain layers).

### **4.2 Rendering**
- Renderer must traverse layers in ascending z‑order.  
- Each layer must push its transform (usually identity).  
- Overlay layer is drawn last and is not persisted.

### **4.3 Hit‑Testing**
- Hit‑testing must traverse layers in descending z‑order.  
- The first hit shape wins.  
- Non‑interactive layers must be skipped.

---

## **5. Domain Model (C++20 Modules)**

### **5.1 Layer Interface**
```cpp
export module domain.layers.layer;

import domain.shapes.composite;
import std.core;

export struct Layer : Composite {
    bool interactive = true;
    std::string name;

    explicit Layer(std::string name, bool interactive = true)
        : name(std::move(name)), interactive(interactive) {}
};
```

### **5.2 LayeredComposite**
```cpp
export module domain.layers.layered_composite;

import domain.layers.layer;
import std.core;

export struct LayeredComposite : Composite {
    std::vector<std::unique_ptr<Layer>> layers;

    Layer& add_layer(std::unique_ptr<Layer> layer);
    Layer& layer_at(size_t index);
    size_t layer_count() const;

    HitResult hit_test(Point p) const override;
    Rect bounds() const override;
};
```

### **5.3 Responsibilities**
- Maintain ordered list of layers  
- Delegate hit‑testing to layers  
- Compute bounds as union of layer bounds  
- Emit domain events (LayerAdded, LayerRemoved, ActiveLayerChanged)

---

## **6. FSM Integration**

### **6.1 Active Layer**
The FSM maintains:
- `active_layer_index`
- `cross_layer_mode` flag

Rules:
- Tools operate on the active layer by default.  
- Selection tool may optionally hit across layers.  
- Add tool always adds to the active layer.

### **6.2 Layer Switching**
FSM must support:
- Next/previous layer  
- Direct layer selection  
- Lock/unlock layer  
- Hide/show layer  

### **6.3 Layer‑Aware Tools**
Tools must respect:
- Layer interactivity  
- Layer visibility  
- Layer locking  

Examples:
- Selection tool ignores hidden or locked layers.  
- Add tool refuses to add to non‑interactive layers.  
- Move tool cannot move shapes across layers unless in cross‑layer mode.

---

## **7. Rendering Model**

### **7.1 Traversal**
Renderer must:
1. Clear background  
2. Render background layer  
3. Render grid layer  
4. Render shapes layer  
5. Render annotation layer  
6. Render overlay layer (renderer‑only)

### **7.2 Layer Transforms**
- Layers typically use identity transform  
- Composite transforms apply inside layers  
- Renderer must push/pop transforms per layer

### **7.3 Z‑Order Rules**
- Layer index defines z‑order  
- Shapes inside a layer follow composite z‑order rules  
- Overlay layer always wins hit‑testing but is not part of domain

---

## **8. Events**

### **8.1 Layer Events**
- `LayerAdded`
- `LayerRemoved`
- `LayerRenamed`
- `LayerReordered`
- `LayerVisibilityChanged`
- `LayerLockChanged`
- `ActiveLayerChanged`

### **8.2 Event Rules**
- Emitted by domain, not UI  
- Deterministic  
- Testable in isolation  

---

## **9. Examples**

### **9.1 Default Diagram Structure**
```
DiagramRoot (LayeredComposite)
 ├── BackgroundLayer
 ├── GridLayer
 ├── ShapesLayer
 └── AnnotationLayer
```

### **9.2 Adding a Shape**
- User draws a rectangle  
- FSM routes to active layer (ShapesLayer)  
- Rectangle is added as a child of ShapesLayer  

### **9.3 Hit‑Testing**
- User clicks  
- Overlay layer checked first  
- Then AnnotationLayer  
- Then ShapesLayer  
- Grid and Background skipped (non‑interactive)

---

## **10. Testing Strategy**

### **10.1 Unit Tests**
- Layer ordering  
- Hit‑testing across layers  
- Visibility and locking  
- Bounds computation  
- Event emission  

### **10.2 Integration Tests**
- FSM layer switching  
- Adding shapes to layers  
- Rendering order  
- Cross‑layer selection  

---

## **11. Future Extensions**
- Per‑layer snapping rules  
- Per‑layer grids  
- Per‑layer opacity  
- Layer groups  
- Layer templates (UML, BPMN, ERD presets)  

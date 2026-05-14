# **Composites**

## **1. Purpose**
Composites represent **hierarchical graphical structures** in the diagram domain.  
They allow shapes to be grouped, nested, aggregated, layered, and manipulated as a single logical unit while preserving individual geometry, events, and rendering responsibilities.

Composites are a **first‑class domain concept**, not a UI convenience.  
They must be fully represented in the domain model, persisted, rendered, and manipulated through the tool FSM.

---

## **2. Core Principles**

### **2.1 Domain Purity**
- A composite is a **Shape**.  
- A composite contains **zero or more Shapes**.  
- A composite exposes **the same interface** as any other shape (hit‑testing, bounds, transformations, events).

### **2.2 No UI Leakage**
- The domain does not know about Win32, D2D, or UI tools.  
- Composites are manipulated through domain commands and events.  
- Rendering is delegated to the renderer layer.

### **2.3 Deterministic Structure**
- Composite membership is explicit and ordered.  
- Transformations propagate deterministically.  
- Hit‑testing respects z‑order and composite boundaries.

---

## **3. Composite Types**

### **3.1 Simple Composite**
A group of shapes treated as a single unit.

Characteristics:
- Shared transform  
- Shared selection  
- Shared bounding box  
- Children maintain relative transforms  

### **3.2 Aggregating Composite**
A composite that **owns** its children.

Characteristics:
- Children lifecycle tied to the composite  
- Deleting the composite deletes children  
- Used for UML constructs like:
  - Class with compartments  
  - Package with nested elements  

### **3.3 Layered Composite**
A composite that organizes children into **layers**.

Characteristics:
- Each layer has its own z‑order  
- Hit‑testing respects layer ordering  
- Used for:
  - Complex diagrams  
  - Background grids  
  - Annotation layers  

### **3.4 Boolean Composites (Future)**
Composites formed by boolean operations:
- Union  
- Subtract  
- Intersect  

These are renderer‑level composites but must be represented in the domain.

---

## **4. Invariants**

### **4.1 Structural Invariants**
- A composite must not contain itself (no cycles).  
- A shape may belong to **only one composite**.  
- Composite children must be stored in deterministic order.

### **4.2 Transform Invariants**
- Composite transform is applied **before** child transforms.  
- Child transforms are relative to the composite origin.  
- Bounding box = union of transformed child bounding boxes.

### **4.3 Hit‑Testing Invariants**
- Hit‑testing must recurse depth‑first.  
- The deepest child hit wins.  
- If no child is hit, the composite may still be hit if it has a visible frame.

---

## **5. FSM Integration**

Composites interact with the **Tool FSM** (SelectTool, AddTool, etc.) as follows:

### **5.1 Selection**
- Selecting a child selects the composite if the tool is in *Group Selection Mode*.  
- Selecting a child selects only the child if in *Direct Selection Mode*.  
- FSM must support toggling between these modes (like Blender).

### **5.2 Manipulation**
- Move/scale/rotate operations apply to the composite transform.  
- Direct manipulation mode applies transforms to the child only.

### **5.3 Creation**
Tools must support:
- Creating a composite from selected shapes  
- Creating aggregating composites (e.g., UML class compartments)  
- Creating layered composites  

### **5.4 Decomposition**
FSM must support:
- Ungrouping  
- Extracting a child from a composite  
- Flattening nested composites  

---

## **6. Domain Model (C++20 Modules)**

### **6.1 Interfaces**
```cpp
export module domain.shapes.composite;

import domain.shapes.shape;
import std.core;

export struct Composite : Shape {
    std::vector<std::unique_ptr<Shape>> children;

    void add_child(std::unique_ptr<Shape> child);
    void remove_child(const Shape& child);

    Rect bounds() const override;
    HitResult hit_test(Point p) const override;

    void transform(const Transform& t) override;
};
```

### **6.2 Responsibilities**
- Maintain child list  
- Compute bounds  
- Delegate hit‑testing  
- Apply transforms recursively  
- Emit domain events (CompositeCreated, ChildAdded, etc.)

---

## **7. Rendering Model**

### **7.1 Renderer Responsibilities**
- Push composite transform  
- Render children in order  
- Pop transform  
- Render composite frame if applicable  

### **7.2 Z‑Order**
- Composite z‑order is the minimum of its children  
- Children z‑order is relative to composite  

---

## **8. Events**

### **8.1 Composite Events**
- `CompositeCreated`
- `CompositeDeleted`
- `CompositeTransformed`
- `CompositeChildAdded`
- `CompositeChildRemoved`
- `CompositeFlattened`

### **8.2 Event Rules**
- Events must be deterministic  
- Events must be emitted by the domain, not UI  
- Events must be testable in isolation  

---

## **9. Layers and Composites**

Layers are implemented as **LayeredComposite**.

Rules:
- Each layer is a composite  
- The diagram root is a composite of layers  
- Tools operate on the active layer unless in cross‑layer mode  

---

## **10. Examples**

### **10.1 UML Class**
A UML class is an **AggregatingComposite**:

- Frame rectangle  
- Name compartment  
- Attributes compartment  
- Methods compartment  

### **10.2 Group of Shapes**
User selects 3 shapes → presses Group → creates SimpleComposite.

### **10.3 Diagram Root**
The diagram root is a **LayeredComposite**:
- Background layer  
- Grid layer  
- Shapes layer  
- Annotation layer  

---

## **11. Testing Strategy**

### **11.1 Unit Tests**
- Composite bounds  
- Hit‑testing recursion  
- Transform propagation  
- Child lifecycle  
- Event emission  

### **11.2 Integration Tests**
- FSM selection modes  
- Group/ungroup operations  
- Layer switching  
- Renderer composite traversal  

---

## **12. Future Extensions**
- Boolean geometry composites  
- Constraint‑based composites  
- Auto‑layout composites  
- Undo/redo composite operations  

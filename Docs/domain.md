# Domain Layer  
Pure C++20 Modules — Deterministic, UI‑Agnostic, Renderer‑Agnostic

The **domain layer** is the core of DiagramApp.  
It defines the conceptual model of diagrams, shapes, geometry, and operations.  
It is intentionally **pure**, **deterministic**, and **independent** of UI, rendering, or platform concerns.

This document describes the structure, responsibilities, invariants, and extension points of the domain.

---

## 1. Design Principles

The domain layer follows these principles:

### **1. Pure C++20 Modules**
No headers, no macros, no global state.  
All boundaries are explicit and enforced by the compiler.

### **2. UI‑Agnostic**
The domain does not know about:
- Win32
- Direct2D
- Tools
- Events
- Rendering

### **3. Deterministic**
All operations are:
- pure functions or controlled mutations
- free of side effects outside the domain
- reproducible

### **4. No Hidden State**
No singletons, no global variables, no implicit dependencies.

### **5. Testable**
Every domain operation can be tested without a window or renderer.

---

## 2. Domain Modules

The domain is composed of several modules:

```
domain.shape
domain.diagram
domain.geometry
domain.hit_test
domain.composite (future)
domain.connector (future)
domain.serialization (future)
```

Each module has a single responsibility.

---

## 3. Shapes

Shapes are represented as a **closed variant type**:

```cpp
export using Shape = std::variant<
    Rectangle,
    Ellipse
    // Composite, Connector, etc. will be added later
>;
```

### 3.1 Rectangle

```cpp
struct Rectangle
{
    float x;
    float y;
    float width;
    float height;
};
```

### 3.2 Ellipse

```cpp
struct Ellipse
{
    float x;
    float y;
    float width;
    float height;
};
```

### 3.3 Future Shapes

The architecture supports adding:

- Composite shapes  
- Boolean shapes (union, subtract, intersect)  
- Connectors  
- Groups  
- Text shapes  
- Layered shapes  

All without breaking existing code.

---

## 4. Diagram

`Diagram` is the root aggregate of the domain.

### Responsibilities

- Own all shapes  
- Provide shape iteration  
- Provide hit‑testing  
- Provide mutation operations  
- Maintain z‑order  
- Provide grouping/composite operations (future)

### Interface (conceptual)

```cpp
class Diagram
{
public:
    const std::vector<Shape>& shapes() const noexcept;

    // Mutation
    void add_shape(Shape s);
    void remove_shape(const Shape* s);
    void move_shape(const Shape* s, float dx, float dy);

    // Hit testing
    std::vector<const Shape*> hit_test_all(float px, float py) const;
    const Shape* hit_test_topmost(float px, float py) const;

    // Geometry
    bool shape_intersects_rect(const Shape& s,
                               float x0, float y0,
                               float x1, float y1) const;
};
```

### Invariants

- Shapes are stored in z‑order (back → front).
- Reverse iteration yields topmost shapes first.
- Shape pointers remain stable unless removed.

---

## 5. Hit‑Testing

Hit‑testing is implemented in two layers:

### 5.1 Shape‑level hit‑testing

```cpp
bool hit_test(const Shape& s, float px, float py);
```

Implemented via `std::visit`.

### 5.2 Diagram‑level hit‑testing

```cpp
std::vector<const Shape*> hit_test_all(float px, float py) const;
```

Returns all shapes under the cursor, topmost first.

This enables:

- selection cycling  
- multi‑selection  
- lasso selection  
- connector endpoint detection  
- handle detection  

---

## 6. Geometry Utilities

The domain provides geometry helpers:

- point‑in‑rectangle  
- point‑in‑ellipse  
- rectangle intersection  
- bounding boxes  
- shape extents  

These utilities are pure functions.

---

## 7. Mutation Operations

The domain provides controlled mutation:

### 7.1 Move

```cpp
void move_shape(const Shape* s, float dx, float dy);
```

### 7.2 Add

```cpp
void add_shape(Shape s);
```

### 7.3 Remove

```cpp
void remove_shape(const Shape* s);
```

### 7.4 Composite / Boolean (future)

Planned operations:

- `make_composite({shapes})`
- `subtract(a, b)`
- `intersect(a, b)`
- `ungroup(composite)`

These will be implemented as domain‑level transformations.

---

## 8. Z‑Order

Z‑order is implicit in the vector order:

- index 0 = back  
- last index = front  

Operations that affect z‑order:

- bring to front  
- send to back  
- raise  
- lower  

These will be implemented as domain operations, not UI hacks.

---

## 9. Domain and Tools

Tools manipulate the domain through:

- hit‑testing  
- mutation operations  
- geometry queries  

Tools **never** modify shapes directly.  
They always call domain methods.

This ensures:

- invariants are preserved  
- domain remains the single source of truth  
- tools remain interchangeable  

---

## 10. Domain and Rendering

The renderer consumes:

- `Diagram::shapes()`
- shape geometry
- z‑order

The renderer does **not** mutate the domain.  
The domain does **not** know the renderer exists.

This separation guarantees determinism.

---

## 11. Future Extensions

The domain is designed to support:

### Shapes
- Composite shapes  
- Boolean shapes  
- Connectors  
- Text shapes  
- Groups  
- Layers  

### Operations
- Undo/redo  
- Constraints  
- Snapping  
- Alignment  
- Auto‑layout  

### Persistence
- Serialization to JSON  
- Import/export formats  

All without breaking existing modules.

---

## 12. Summary

The domain layer is:

- pure  
- deterministic  
- modular  
- UI‑agnostic  
- renderer‑agnostic  
- tool‑friendly  
- agent‑friendly  

It is the foundation of DiagramApp and the source of truth for all higher layers.

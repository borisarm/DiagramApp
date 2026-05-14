# Geometry Subsystem  
Deterministic, Pure, Domain‑Level Geometry Utilities

The **geometry subsystem** provides all geometric operations used by the domain, tools, and renderer.  
It is intentionally **pure**, **deterministic**, and **UI‑agnostic**.  
No Win32, no Direct2D, no rendering types — only math.

Geometry is a foundational layer for:

- hit‑testing  
- selection  
- dragging  
- lasso selection  
- bounding boxes  
- composite shapes  
- boolean operations  
- connectors  
- snapping  
- alignment  
- layout algorithms  

This document describes the geometry utilities, invariants, and extension points.

---

## 1. Design Principles

### **1. Pure Functions**
All geometry functions are pure:

- no side effects  
- no global state  
- no mutation  

### **2. Deterministic**
Given the same inputs, geometry functions always produce the same outputs.

### **3. Domain‑Level**
Geometry operates on domain primitives:

- `Rectangle`
- `Ellipse`
- `Shape` (variant)
- raw floats

### **4. Renderer‑Agnostic**
Geometry does not know about:

- Direct2D  
- Win32  
- Tools  
- UI  

### **5. Extensible**
Geometry is designed to support:

- composite shapes  
- boolean operations  
- connectors  
- transforms  
- snapping  
- constraints  

---

## 2. Coordinate System

DiagramApp uses a **device‑independent, Cartesian coordinate system**:

- origin `(0,0)` is the top‑left of the diagram  
- +x goes right  
- +y goes down  
- units are floating‑point values (pixels in screen space)  

This matches Direct2D’s coordinate system and simplifies rendering.

---

## 3. Geometry Primitives

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

### 3.3 Bounding Box

All shapes have an axis‑aligned bounding box (AABB):

```cpp
struct Bounds
{
    float x0, y0;
    float x1, y1;
};
```

---

## 4. Hit‑Testing

Hit‑testing is implemented in two layers:

### 4.1 Point‑in‑Rectangle

```cpp
bool point_in_rect(float px, float py,
                   float x, float y,
                   float w, float h);
```

### 4.2 Point‑in‑Ellipse

Ellipse hit‑testing uses the normalized equation:

```
(dx / rx)^2 + (dy / ry)^2 <= 1
```

Where:

- `dx = px - center_x`
- `dy = py - center_y`
- `rx = width / 2`
- `ry = height / 2`

### 4.3 Shape‑level Hit‑Testing

```cpp
bool hit_test(const Shape& s, float px, float py);
```

Implemented via `std::visit`.

### 4.4 Diagram‑level Hit‑Testing

```cpp
std::vector<const Shape*> hit_test_all(float px, float py) const;
```

Returns all shapes under the cursor, topmost first.

---

## 5. Rectangle Intersection

Used for:

- lasso selection  
- bounding box selection  
- composite operations  
- boolean operations (future)  

### Axis‑Aligned Rectangle Intersection

```cpp
bool rect_intersects(float ax0, float ay0, float ax1, float ay1,
                     float bx0, float by0, float bx1, float by1);
```

### Shape‑Rectangle Intersection

```cpp
bool shape_intersects_rect(const Shape& s,
                           float x0, float y0,
                           float x1, float y1);
```

Ellipse intersection uses bounding box approximation for performance.

---

## 6. Bounding Boxes

Every shape has a bounding box:

```cpp
Bounds bounds_of(const Shape& s);
```

Used for:

- selection outlines  
- lasso selection  
- drag previews  
- composite shapes  
- connectors  
- layout algorithms  

---

## 7. Movement and Translation

Movement is a pure geometric operation:

```cpp
void translate(Rectangle& r, float dx, float dy);
void translate(Ellipse& e, float dx, float dy);
```

Tools call domain methods that wrap these operations.

---

## 8. Distance and Thresholds

Used for:

- drag threshold detection  
- snapping (future)  
- connector routing (future)  

### Euclidean Distance

```cpp
float distance(float x1, float y1, float x2, float y2);
```

### Squared Distance

Used for threshold checks without `sqrt`.

---

## 9. Future Extensions

The geometry subsystem is designed to support:

### **1. Composite Shapes**
- union of bounding boxes  
- child transforms  
- hierarchical hit‑testing  

### **2. Boolean Operations**
- union  
- subtract  
- intersect  
- XOR  

### **3. Connectors**
- line intersection  
- orthogonal routing  
- anchor point detection  

### **4. Transforms**
- rotation  
- scaling  
- local coordinate systems  

### **5. Snapping**
- to grid  
- to guides  
- to shape edges  
- to anchor points  

### **6. Constraints**
- alignment  
- equal spacing  
- aspect ratio locks  

### **7. Layout Algorithms**
- tree layout  
- graph layout  
- force‑directed layout  

---

## 10. Summary

The geometry subsystem is:

- pure  
- deterministic  
- domain‑level  
- renderer‑agnostic  
- tool‑agnostic  
- extensible  

It provides the mathematical foundation for:

- hit‑testing  
- selection  
- dragging  
- overlays  
- composite shapes  
- boolean operations  
- connectors  
- snapping  
- layout  

It is one of the most critical subsystems in DiagramApp.


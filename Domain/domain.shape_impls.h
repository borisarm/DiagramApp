#pragma once

// domain.shape_impls.h
// In-tree COM implementations of IShapePlugin / IShapeClass / IShape for
// the built-in Rectangle and Ellipse shapes.
//
// These are compiled directly into the Domain static library.
// At startup the host calls:
//   builtin_rectangle_plugin(ppPlugin)
//   builtin_ellipse_plugin(ppPlugin)
// and registers the returned IShapePlugin with ShapeRegistry.

#include "shape_interfaces.h"

// Returns an IShapePlugin that vends exactly one IShapeClass: Rectangle
extern "C" HRESULT builtin_rectangle_plugin(IShapePlugin** ppPlugin);

// Returns an IShapePlugin that vends exactly one IShapeClass: Ellipse
extern "C" HRESULT builtin_ellipse_plugin(IShapePlugin** ppPlugin);

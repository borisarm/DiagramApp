#pragma once

// shape_interfaces.h
// COM-like ABI shared between the host (DiagramApp) and shape plugin DLLs.
// No C++ runtime types cross this boundary — only vtable pointers, POD structs,
// COM primitives (HRESULT, BSTR, IUnknown) and WRL ComPtr on the host side.

#include <Unknwn.h>
#include <wtypes.h>

// ---------------------------------------------------------------------------
// Geometry token — the only shape-description type the renderer needs.
// Plugin DLLs fill this in; the host renderer switches on `kind`.
// ---------------------------------------------------------------------------
enum class ShapeGeometryKind : UINT32
{
	Rect    = 0,
	Ellipse = 1,
	Path    = 2,   // future: polygon / bezier
};

struct ShapeGeometry
{
	ShapeGeometryKind kind;
	float x, y, width, height;  // bounding box (Rect & Ellipse)
	// PATH: reserved for future extension
};

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
struct IShapeClass;
struct IShape;

// ---------------------------------------------------------------------------
// IShapePlugin  — one per DLL, returned by DllGetShapePlugin().
// Enumerates all shape types the DLL provides.
// ---------------------------------------------------------------------------
// {3F8A1B20-4C7E-4D91-B3A2-1E6F9D0C5B84}
MIDL_INTERFACE("3F8A1B20-4C7E-4D91-B3A2-1E6F9D0C5B84")
IShapePlugin : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetShapeClassCount(UINT32* pCount) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetShapeClass(UINT32 index,
													IShapeClass** ppClass) = 0;
};

// ---------------------------------------------------------------------------
// IShapeClass  — describes one shape type; serves as its factory.
// One instance lives for the lifetime of the plugin.
// ---------------------------------------------------------------------------
// {A7D54E31-82FB-4B12-9CE0-3A1D7F8B2C65}
MIDL_INTERFACE("A7D54E31-82FB-4B12-9CE0-3A1D7F8B2C65")
IShapeClass : public IUnknown
{
	// Stable reverse-DNS identifier, e.g. L"com.diagramapp.rectangle"
	virtual HRESULT STDMETHODCALLTYPE GetId  (BSTR* pId)   = 0;

	// Human-readable name shown in the stencil, e.g. L"Rectangle"
	virtual HRESULT STDMETHODCALLTYPE GetName(BSTR* pName) = 0;

	// Representative geometry used to draw the stencil tile icon
	virtual HRESULT STDMETHODCALLTYPE GetStencilGeometry(ShapeGeometry* pGeom) = 0;

	// Factory: create a new shape instance with the given bounding box
	virtual HRESULT STDMETHODCALLTYPE CreateShape(float x, float y,
												  float w, float h,
												  IShape** ppShape) = 0;
};

// ---------------------------------------------------------------------------
// IShape  — one per diagram shape instance.
// ---------------------------------------------------------------------------
// {C2E96A47-15D3-4F80-AE71-82B6540D9E3F}
MIDL_INTERFACE("C2E96A47-15D3-4F80-AE71-82B6540D9E3F")
IShape : public IUnknown
{
	// Bounding box
	virtual HRESULT STDMETHODCALLTYPE GetBounds(float* pX, float* pY,
												float* pW, float* pH) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBounds(float  x,  float  y,
												float  w,  float  h)  = 0;

	// Translate by (dx, dy)
	virtual HRESULT STDMETHODCALLTYPE Move(float dx, float dy) = 0;

	// Point-in-shape hit test; sets *pHit = TRUE if (px,py) is inside
	virtual HRESULT STDMETHODCALLTYPE HitTest(float px, float py,
											  BOOL* pHit) = 0;

	// Axis-aligned rectangle intersection (for lasso selection)
	virtual HRESULT STDMETHODCALLTYPE IntersectsRect(float x0, float y0,
													 float x1, float y1,
													 BOOL* pResult) = 0;

	// Geometry for rendering
	virtual HRESULT STDMETHODCALLTYPE GetGeometry(ShapeGeometry* pGeom) = 0;

	// Deep copy
	virtual HRESULT STDMETHODCALLTYPE Clone(IShape** ppClone) = 0;

	// Serialization (JSON fragment; implementer allocates BSTR)
	virtual HRESULT STDMETHODCALLTYPE Serialize  (BSTR* pOut) = 0;
	virtual HRESULT STDMETHODCALLTYPE Deserialize(BSTR  in)   = 0;

	// Back-pointer to the class that created this instance
	virtual HRESULT STDMETHODCALLTYPE GetShapeClass(IShapeClass** ppClass) = 0;
};

// ---------------------------------------------------------------------------
// DLL export contract — every shape plugin DLL exports exactly this symbol.
// The host calls it via GetProcAddress; no COM registry needed.
// ---------------------------------------------------------------------------
typedef HRESULT (STDAPICALLTYPE *PFN_DllGetShapePlugin)(IShapePlugin** ppPlugin);
// extern "C" __declspec(dllexport)
// HRESULT STDAPICALLTYPE DllGetShapePlugin(IShapePlugin** ppPlugin);

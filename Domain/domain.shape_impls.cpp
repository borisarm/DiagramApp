// domain.shape_impls.cpp
// In-tree COM implementations of the built-in Rectangle and Ellipse shapes.

#include "domain.shape_impls.h"
#include <cmath>
#include <new>
#include <stdio.h>
#include <string>
#include <wchar.h>

// ---------------------------------------------------------------------------
// Minimal IUnknown base — not thread-safe (single-threaded app)
// ---------------------------------------------------------------------------
template<typename TInterface>
struct ComBase : public TInterface
{
	ULONG m_refCount = 1;

	ULONG STDMETHODCALLTYPE AddRef()  override { return ++m_refCount; }
	ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG r = --m_refCount;
		if (r == 0) delete this;
		return r;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(TInterface))
		{
			*ppv = static_cast<TInterface*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
};

// ---------------------------------------------------------------------------
// Helper: duplicate a string literal as a BSTR
// ---------------------------------------------------------------------------
static BSTR bstr(const wchar_t* s) { return SysAllocString(s); }

// ---------------------------------------------------------------------------
// Shared style bag — embedded in every shape instance
// ---------------------------------------------------------------------------
struct ShapeStyle
{
	UINT32       fill_color   = 0xFF6495ED;   // cornflower blue  (0xAARRGGBB)
	UINT32       stroke_color = 0xFF6495ED;
	std::wstring label;
};

// ---------------------------------------------------------------------------
// Property descriptors shared by all shapes
// ---------------------------------------------------------------------------
static const ShapePropertyDescriptor k_common_style_props[] =
{
	{ L"fill_color",   L"Fill Color",   ShapePropertyType::Color,  ShapePropertyCategory::Style,   FALSE },
	{ L"stroke_color", L"Stroke Color", ShapePropertyType::Color,  ShapePropertyCategory::Style,   FALSE },
	{ L"label",        L"Label",        ShapePropertyType::String, ShapePropertyCategory::Text,    FALSE },
};
static constexpr UINT32 k_common_count = 3;

static const ShapePropertyDescriptor k_rect_geom_props[] =
{
	{ L"x", L"X",      ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"y", L"Y",      ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"w", L"Width",  ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"h", L"Height", ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
};
static constexpr UINT32 k_rect_geom_count = 4;

static const ShapePropertyDescriptor k_ellipse_geom_props[] =
{
	{ L"x",  L"X",            ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"y",  L"Y",            ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"rx", L"Radius X",     ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
	{ L"ry", L"Radius Y",     ShapePropertyType::Float, ShapePropertyCategory::Geometry, FALSE },
};
static constexpr UINT32 k_ellipse_geom_count = 4;

// ===========================================================================
// Rectangle
// ===========================================================================

struct CRectangleShapeClass;   // forward

struct CRectangleShape : public ComBase<IShape>
					   , public IShapeProperties
{
	float m_x, m_y, m_w, m_h;
	ShapeStyle m_style;
	CRectangleShapeClass* m_class;  // non-owning (class outlives instances)

	CRectangleShape(float x, float y, float w, float h,
					CRectangleShapeClass* cls)
		: m_x(x), m_y(y), m_w(w), m_h(h), m_class(cls) {}

	// ---- IUnknown (override to expose IShapeProperties) ----
	ULONG STDMETHODCALLTYPE AddRef()  override { return ComBase<IShape>::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComBase<IShape>::Release(); }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
	{
		if (riid == __uuidof(IShapeProperties))
		{
			*ppv = static_cast<IShapeProperties*>(this);
			AddRef();
			return S_OK;
		}
		return ComBase<IShape>::QueryInterface(riid, ppv);
	}

	// ---- IShape ----
	HRESULT STDMETHODCALLTYPE GetBounds(float* px, float* py,
										float* pw, float* ph) override
	{ *px = m_x; *py = m_y; *pw = m_w; *ph = m_h; return S_OK; }

	HRESULT STDMETHODCALLTYPE SetBounds(float x, float y,
										float w, float h) override
	{ m_x = x; m_y = y; m_w = w; m_h = h; return S_OK; }

	HRESULT STDMETHODCALLTYPE Move(float dx, float dy) override
	{ m_x += dx; m_y += dy; return S_OK; }

	HRESULT STDMETHODCALLTYPE HitTest(float px, float py, BOOL* pHit) override
	{
		*pHit = (px >= m_x && px <= m_x + m_w &&
				 py >= m_y && py <= m_y + m_h) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE IntersectsRect(float x0, float y0,
											 float x1, float y1,
											 BOOL* pResult) override
	{
		*pResult = !(m_x + m_w < x0 || m_x > x1 ||
					 m_y + m_h < y0 || m_y > y1) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetGeometry(ShapeGeometry* pGeom) override
	{
		pGeom->kind   = ShapeGeometryKind::Rect;
		pGeom->x      = m_x;
		pGeom->y      = m_y;
		pGeom->width  = m_w;
		pGeom->height = m_h;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IShape** ppClone) override;  // defined below

	HRESULT STDMETHODCALLTYPE Serialize(BSTR* pOut) override
	{
		wchar_t buf[512];
		swprintf_s(buf,
			L"{\"class\":\"com.diagramapp.rectangle\","
			L"\"x\":%.4f,\"y\":%.4f,\"w\":%.4f,\"h\":%.4f,"
			L"\"fill_color\":%u,\"stroke_color\":%u,\"label\":\"%s\"}",
			m_x, m_y, m_w, m_h,
			m_style.fill_color, m_style.stroke_color,
			m_style.label.c_str());
		*pOut = bstr(buf);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE Deserialize(BSTR in) override
	{
		// Parsed externally by the serializer; this path is not used directly.
		(void)in; return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetShapeClass(IShapeClass** ppClass) override;

	// ---- IShapeProperties ----
	HRESULT STDMETHODCALLTYPE GetPropertyCount(UINT32* pCount) override
	{ *pCount = k_rect_geom_count + k_common_count; return S_OK; }

	HRESULT STDMETHODCALLTYPE GetDescriptor(UINT32 index, ShapePropertyDescriptor* pDesc) override
	{
		if (index < k_rect_geom_count)          { *pDesc = k_rect_geom_props[index]; return S_OK; }
		index -= k_rect_geom_count;
		if (index < k_common_count)              { *pDesc = k_common_style_props[index]; return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetFloat(const wchar_t* key, float* pValue) override
	{
		if (wcscmp(key, L"x") == 0) { *pValue = m_x; return S_OK; }
		if (wcscmp(key, L"y") == 0) { *pValue = m_y; return S_OK; }
		if (wcscmp(key, L"w") == 0) { *pValue = m_w; return S_OK; }
		if (wcscmp(key, L"h") == 0) { *pValue = m_h; return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetFloat(const wchar_t* key, float value) override
	{
		if (wcscmp(key, L"x") == 0) { m_x = value; return S_OK; }
		if (wcscmp(key, L"y") == 0) { m_y = value; return S_OK; }
		if (wcscmp(key, L"w") == 0) { m_w = (value > 1.f ? value : 1.f); return S_OK; }
		if (wcscmp(key, L"h") == 0) { m_h = (value > 1.f ? value : 1.f); return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetColor(const wchar_t* key, UINT32* pValue) override
	{
		if (wcscmp(key, L"fill_color")   == 0) { *pValue = m_style.fill_color;   return S_OK; }
		if (wcscmp(key, L"stroke_color") == 0) { *pValue = m_style.stroke_color; return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetColor(const wchar_t* key, UINT32 value) override
	{
		if (wcscmp(key, L"fill_color")   == 0) { m_style.fill_color   = value; return S_OK; }
		if (wcscmp(key, L"stroke_color") == 0) { m_style.stroke_color = value; return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetString(const wchar_t* key, BSTR* pValue) override
	{
		if (wcscmp(key, L"label") == 0) { *pValue = bstr(m_style.label.c_str()); return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetString(const wchar_t* key, const wchar_t* value) override
	{
		if (wcscmp(key, L"label") == 0) { m_style.label = value ? value : L""; return S_OK; }
		return E_INVALIDARG;
	}
};

struct CRectangleShapeClass : public ComBase<IShapeClass>
{
	HRESULT STDMETHODCALLTYPE GetId(BSTR* pId) override
	{ *pId = bstr(L"com.diagramapp.rectangle"); return S_OK; }

	HRESULT STDMETHODCALLTYPE GetName(BSTR* pName) override
	{ *pName = bstr(L"Rectangle"); return S_OK; }

	HRESULT STDMETHODCALLTYPE GetStencilGeometry(ShapeGeometry* pGeom) override
	{
		pGeom->kind = ShapeGeometryKind::Rect;
		pGeom->x = 8.f; pGeom->y = 20.f; pGeom->width = 54.f; pGeom->height = 36.f;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CreateShape(float x, float y, float w, float h,
										  IShape** ppShape) override
	{
		*ppShape = new (std::nothrow) CRectangleShape(x, y, w, h, this);
		return *ppShape ? S_OK : E_OUTOFMEMORY;
	}
};

// Deferred definitions that need CRectangleShapeClass to be complete
HRESULT CRectangleShape::Clone(IShape** ppClone)
{
	auto* p = new (std::nothrow) CRectangleShape(m_x, m_y, m_w, m_h, m_class);
	if (!p) return E_OUTOFMEMORY;
	p->m_style = m_style;
	*ppClone = p;
	return S_OK;
}
HRESULT CRectangleShape::GetShapeClass(IShapeClass** ppClass)
{
	*ppClass = m_class;
	m_class->AddRef();
	return S_OK;
}

struct CRectanglePlugin : public ComBase<IShapePlugin>
{
	CRectangleShapeClass m_class;  // owns the single class instance (embedded)

	// Keep ref count stable — the embedded class's lifetime matches ours
	CRectanglePlugin() { m_class.AddRef(); }   // balance the base ctor's +1 ...
	// actually just let the embedded object live with us; override Release to skip delete
	// Simpler: give the class a manual refcount that doesn't delete:

	HRESULT STDMETHODCALLTYPE GetShapeClassCount(UINT32* pCount) override
	{ *pCount = 1; return S_OK; }

	HRESULT STDMETHODCALLTYPE GetShapeClass(UINT32 index, IShapeClass** ppClass) override
	{
		if (index != 0) return E_INVALIDARG;
		*ppClass = &m_class;
		m_class.AddRef();
		return S_OK;
	}
};

// ===========================================================================
// Ellipse
// ===========================================================================

struct CEllipseShapeClass;

struct CEllipseShape : public ComBase<IShape>
					 , public IShapeProperties
{
	float m_x, m_y, m_w, m_h;
	ShapeStyle m_style;
	CEllipseShapeClass* m_class;

	CEllipseShape(float x, float y, float w, float h, CEllipseShapeClass* cls)
		: m_x(x), m_y(y), m_w(w), m_h(h), m_class(cls) {}

	// ---- IUnknown ----
	ULONG STDMETHODCALLTYPE AddRef()  override { return ComBase<IShape>::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComBase<IShape>::Release(); }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
	{
		if (riid == __uuidof(IShapeProperties))
		{
			*ppv = static_cast<IShapeProperties*>(this);
			AddRef();
			return S_OK;
		}
		return ComBase<IShape>::QueryInterface(riid, ppv);
	}

	// ---- IShape ----
	HRESULT STDMETHODCALLTYPE GetBounds(float* px, float* py,
										float* pw, float* ph) override
	{ *px = m_x; *py = m_y; *pw = m_w; *ph = m_h; return S_OK; }

	HRESULT STDMETHODCALLTYPE SetBounds(float x, float y,
										float w, float h) override
	{ m_x = x; m_y = y; m_w = w; m_h = h; return S_OK; }

	HRESULT STDMETHODCALLTYPE Move(float dx, float dy) override
	{ m_x += dx; m_y += dy; return S_OK; }

	HRESULT STDMETHODCALLTYPE HitTest(float px, float py, BOOL* pHit) override
	{
		float cx = m_x + m_w * 0.5f, cy = m_y + m_h * 0.5f;
		float rx = m_w * 0.5f,       ry = m_h * 0.5f;
		if (rx < 1e-4f || ry < 1e-4f) { *pHit = FALSE; return S_OK; }
		float dx = (px - cx) / rx, dy = (py - cy) / ry;
		*pHit = (dx * dx + dy * dy <= 1.f) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE IntersectsRect(float x0, float y0,
											 float x1, float y1,
											 BOOL* pResult) override
	{
		*pResult = !(m_x + m_w < x0 || m_x > x1 ||
					 m_y + m_h < y0 || m_y > y1) ? TRUE : FALSE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetGeometry(ShapeGeometry* pGeom) override
	{
		pGeom->kind   = ShapeGeometryKind::Ellipse;
		pGeom->x      = m_x;
		pGeom->y      = m_y;
		pGeom->width  = m_w;
		pGeom->height = m_h;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IShape** ppClone) override;

	HRESULT STDMETHODCALLTYPE Serialize(BSTR* pOut) override
	{
		wchar_t buf[512];
		swprintf_s(buf,
			L"{\"class\":\"com.diagramapp.ellipse\","
			L"\"x\":%.4f,\"y\":%.4f,\"w\":%.4f,\"h\":%.4f,"
			L"\"fill_color\":%u,\"stroke_color\":%u,\"label\":\"%s\"}",
			m_x, m_y, m_w, m_h,
			m_style.fill_color, m_style.stroke_color,
			m_style.label.c_str());
		*pOut = bstr(buf);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE Deserialize(BSTR in) override
	{
		(void)in; return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetShapeClass(IShapeClass** ppClass) override;

	// ---- IShapeProperties ----
	HRESULT STDMETHODCALLTYPE GetPropertyCount(UINT32* pCount) override
	{ *pCount = k_ellipse_geom_count + k_common_count; return S_OK; }

	HRESULT STDMETHODCALLTYPE GetDescriptor(UINT32 index, ShapePropertyDescriptor* pDesc) override
	{
		if (index < k_ellipse_geom_count)      { *pDesc = k_ellipse_geom_props[index]; return S_OK; }
		index -= k_ellipse_geom_count;
		if (index < k_common_count)             { *pDesc = k_common_style_props[index]; return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetFloat(const wchar_t* key, float* pValue) override
	{
		if (wcscmp(key, L"x")  == 0) { *pValue = m_x;         return S_OK; }
		if (wcscmp(key, L"y")  == 0) { *pValue = m_y;         return S_OK; }
		if (wcscmp(key, L"rx") == 0) { *pValue = m_w * 0.5f;  return S_OK; }
		if (wcscmp(key, L"ry") == 0) { *pValue = m_h * 0.5f;  return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetFloat(const wchar_t* key, float value) override
	{
		if (wcscmp(key, L"x")  == 0) { m_x = value;                              return S_OK; }
		if (wcscmp(key, L"y")  == 0) { m_y = value;                              return S_OK; }
		if (wcscmp(key, L"rx") == 0) { m_w = (value > 0.5f ? value : 0.5f)*2.f; return S_OK; }
		if (wcscmp(key, L"ry") == 0) { m_h = (value > 0.5f ? value : 0.5f)*2.f; return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetColor(const wchar_t* key, UINT32* pValue) override
	{
		if (wcscmp(key, L"fill_color")   == 0) { *pValue = m_style.fill_color;   return S_OK; }
		if (wcscmp(key, L"stroke_color") == 0) { *pValue = m_style.stroke_color; return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetColor(const wchar_t* key, UINT32 value) override
	{
		if (wcscmp(key, L"fill_color")   == 0) { m_style.fill_color   = value; return S_OK; }
		if (wcscmp(key, L"stroke_color") == 0) { m_style.stroke_color = value; return S_OK; }
		return E_INVALIDARG;
	}

	HRESULT STDMETHODCALLTYPE GetString(const wchar_t* key, BSTR* pValue) override
	{
		if (wcscmp(key, L"label") == 0) { *pValue = bstr(m_style.label.c_str()); return S_OK; }
		return E_INVALIDARG;
	}
	HRESULT STDMETHODCALLTYPE SetString(const wchar_t* key, const wchar_t* value) override
	{
		if (wcscmp(key, L"label") == 0) { m_style.label = value ? value : L""; return S_OK; }
		return E_INVALIDARG;
	}
};

struct CEllipseShapeClass : public ComBase<IShapeClass>
{
	HRESULT STDMETHODCALLTYPE GetId(BSTR* pId) override
	{ *pId = bstr(L"com.diagramapp.ellipse"); return S_OK; }

	HRESULT STDMETHODCALLTYPE GetName(BSTR* pName) override
	{ *pName = bstr(L"Ellipse"); return S_OK; }

	HRESULT STDMETHODCALLTYPE GetStencilGeometry(ShapeGeometry* pGeom) override
	{
		pGeom->kind = ShapeGeometryKind::Ellipse;
		pGeom->x = 8.f; pGeom->y = 18.f; pGeom->width = 54.f; pGeom->height = 38.f;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CreateShape(float x, float y, float w, float h,
										  IShape** ppShape) override
	{
		*ppShape = new (std::nothrow) CEllipseShape(x, y, w, h, this);
		return *ppShape ? S_OK : E_OUTOFMEMORY;
	}
};

HRESULT CEllipseShape::Clone(IShape** ppClone)
{
	auto* p = new (std::nothrow) CEllipseShape(m_x, m_y, m_w, m_h, m_class);
	if (!p) return E_OUTOFMEMORY;
	p->m_style = m_style;
	*ppClone = p;
	return S_OK;
}
HRESULT CEllipseShape::GetShapeClass(IShapeClass** ppClass)
{
	*ppClass = m_class;
	m_class->AddRef();
	return S_OK;
}

struct CEllipsePlugin : public ComBase<IShapePlugin>
{
	CEllipseShapeClass m_class;

	CEllipsePlugin() { m_class.AddRef(); }

	HRESULT STDMETHODCALLTYPE GetShapeClassCount(UINT32* pCount) override
	{ *pCount = 1; return S_OK; }

	HRESULT STDMETHODCALLTYPE GetShapeClass(UINT32 index, IShapeClass** ppClass) override
	{
		if (index != 0) return E_INVALIDARG;
		*ppClass = &m_class;
		m_class.AddRef();
		return S_OK;
	}
};

// ===========================================================================
// Public factory functions
// ===========================================================================
extern "C" HRESULT builtin_rectangle_plugin(IShapePlugin** ppPlugin)
{
	*ppPlugin = new (std::nothrow) CRectanglePlugin();
	return *ppPlugin ? S_OK : E_OUTOFMEMORY;
}

extern "C" HRESULT builtin_ellipse_plugin(IShapePlugin** ppPlugin)
{
	*ppPlugin = new (std::nothrow) CEllipsePlugin();
	return *ppPlugin ? S_OK : E_OUTOFMEMORY;
}

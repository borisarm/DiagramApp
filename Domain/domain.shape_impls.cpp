// domain.shape_impls.cpp
// In-tree COM implementations of the built-in Rectangle and Ellipse shapes.

#include "domain.shape_impls.h"
#include <cmath>
#include <new>
#include <stdio.h>

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

// ===========================================================================
// Rectangle
// ===========================================================================

struct CRectangleShapeClass;   // forward

struct CRectangleShape : public ComBase<IShape>
{
	float m_x, m_y, m_w, m_h;
	CRectangleShapeClass* m_class;  // non-owning (class outlives instances)

	CRectangleShape(float x, float y, float w, float h,
					CRectangleShapeClass* cls)
		: m_x(x), m_y(y), m_w(w), m_h(h), m_class(cls) {}

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
		wchar_t buf[256];
		swprintf_s(buf, L"{\"type\":\"rectangle\","
						L"\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f}",
				   m_x, m_y, m_w, m_h);
		*pOut = bstr(buf);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE Deserialize(BSTR) override { return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE GetShapeClass(IShapeClass** ppClass) override;
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
	*ppClone = new (std::nothrow) CRectangleShape(m_x, m_y, m_w, m_h, m_class);
	return *ppClone ? S_OK : E_OUTOFMEMORY;
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
{
	float m_x, m_y, m_w, m_h;
	CEllipseShapeClass* m_class;

	CEllipseShape(float x, float y, float w, float h, CEllipseShapeClass* cls)
		: m_x(x), m_y(y), m_w(w), m_h(h), m_class(cls) {}

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
		wchar_t buf[256];
		swprintf_s(buf, L"{\"type\":\"ellipse\","
						L"\"x\":%.2f,\"y\":%.2f,\"w\":%.2f,\"h\":%.2f}",
				   m_x, m_y, m_w, m_h);
		*pOut = bstr(buf);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE Deserialize(BSTR) override { return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE GetShapeClass(IShapeClass** ppClass) override;
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
	*ppClone = new (std::nothrow) CEllipseShape(m_x, m_y, m_w, m_h, m_class);
	return *ppClone ? S_OK : E_OUTOFMEMORY;
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

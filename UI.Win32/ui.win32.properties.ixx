module;

#include <Windows.h>
#include "../Domain/shape_interfaces.h"

export module ui.win32.properties;

import <string>;
import <vector>;
import <functional>;

namespace ui::win32
{
	// -----------------------------------------------------------------------
	// One row in the property grid.  Mirrors one ShapePropertyDescriptor
	// together with its runtime editing state.
	// -----------------------------------------------------------------------
	export struct PropertyRow
	{
		std::wstring            key;
		std::wstring            label;
		ShapePropertyType       type     = ShapePropertyType::Float;
		ShapePropertyCategory   category = ShapePropertyCategory::Geometry;
		bool                    read_only = false;

		// Current text shown in the EDIT control (all types serialised as text).
		std::wstring            text;

		// For Color rows we keep three child EDIT handles (R G B).
		HWND edit_r = nullptr;
		HWND edit_g = nullptr;
		HWND edit_b = nullptr;

		// For Float / String rows a single EDIT control.
		HWND edit = nullptr;

		// Cached colour value (Color rows only) – displayed as a swatch.
		UINT32 color_value = 0;
	};

	// -----------------------------------------------------------------------
	// Floating properties panel window.
	// Call populate() whenever the selection changes.
	// The window fires on_commit whenever the user edits a value.
	// -----------------------------------------------------------------------
	export class PropertiesWindow
	{
	public:
		// Called after the user edits a value and presses Enter / loses focus.
		// The host should commit the diagram snapshot at this point.
		std::function<void()> on_commit;

		void create(HWND owner, int x, int y);
		void destroy();

		// Show properties for a single shape (or clear if nullptr).
		void populate(IShape* shape);

		// Clear all rows and show an empty panel.
		void clear();

		bool is_visible() const { return m_hwnd && IsWindowVisible(m_hwnd); }
		HWND hwnd()       const { return m_hwnd; }

	private:
		static constexpr wchar_t CLASS_NAME[] = L"DiagramPropertiesWindow";
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

		LRESULT handle_message(HWND, UINT, WPARAM, LPARAM);

		void rebuild_controls();
		void destroy_controls();

		// Read all EDIT controls back into the shape, then fire on_commit.
		void flush_to_shape();
		void flush_row(PropertyRow& row);

		void paint(HWND hwnd);
		void on_resize();

		HWND                    m_hwnd   = nullptr;
		IShape*                 m_shape  = nullptr;   // non-owning, may be null
		std::vector<PropertyRow> m_rows;

		static constexpr int ROW_H    = 24;
		static constexpr int LABEL_W  = 110;
		static constexpr int SWATCH_W = 22;
		static constexpr int MARGIN   = 6;
	};
}

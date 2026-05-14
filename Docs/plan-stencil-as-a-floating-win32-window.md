# 🎯 Stencil as a Floating Win32 Window

## Understanding
Move the stencil panel from a fixed painted strip in the main window into its own child `WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME` window that floats alongside the main window. Clicks on tiles are routed back to the same `activate_tool_for_stencil` logic. The main canvas expands to fill the full client area (no more left strip).

## Assumptions
- The floating window is a regular Win32 popup owned by the main window (always-on-top relative to parent, draggable, closeable without quitting the app).
- Its content is painted with the same Direct2D tile rendering already in `d2d.cpp`, but in a dedicated `ID2D1HwndRenderTarget` for that HWND.
- `Stencil` keeps its model role; `StencilItemKind`, `hit_test`, `layout`, `items()` are unchanged.
- `D2DContext` no longer needs a `stencil` pointer or stencil-paint code; those move to the new `StencilWindow`.
- `STENCIL_WIDTH` stays in `stencil.ixx` but `canvas_from_size` in `d2d.cpp` no longer offsets by it.
- `on_lbutton_down` in `window.cpp` no longer guards `fx < STENCIL_WIDTH`; stencil events are handled entirely in the float window's own WndProc.

## Key Files
- `UI.Win32\ui.win32.stencil.ixx` – add `StencilWindow` class declaration
- `UI.Win32\ui.win32.stencil.cpp` – implement `StencilWindow` (register/create/WndProc/paint with D2D)
- `UI.Win32\ui.win32.d2d.cpp` – remove stencil panel paint code; remove stencil ptr from `canvas_from_size`
- `UI.Win32\ui.win32.d2d.ixx` – remove `stencil` ptr and `stencil_label_format` from `D2DContext`
- `UI.Win32\ui.win32.window.cpp` – create `StencilWindow`, remove stencil guard from input handlers, remove stencil layout calls, wire tile-click callback

## Risks & Open Questions
- The float window needs a callback from its WndProc back to `activate_tool_for_stencil`. Use a `std::function<void(StencilItemKind)>` stored in `StencilWindow`.
- The float window's D2D render target must be separate from the main window's.

**Progress**: 100% [██████████]

**Last Updated**: 2026-05-14 23:19:04

## 📝 Plan Steps
- ✅ **Update `ui.win32.stencil.ixx` — add `StencilWindow` class with its own D2D members and a `on_tile_click` callback**
- ✅ **Implement `StencilWindow` in `ui.win32.stencil.cpp` — register class, create popup window, own D2D render target, paint tiles, handle `WM_LBUTTONDOWN`**
- ✅ **Update `ui.win32.d2d.ixx` — remove `stencil` pointer and `stencil_label_format` from `D2DContext`**
- ✅ **Update `ui.win32.d2d.cpp` — remove stencil panel paint block, remove left-offset from `canvas_from_size`**
- ✅ **Update `ui.win32.window.cpp` — create `StencilWindow`, remove stencil guards from input handlers, remove stencil layout from `on_create`/`on_size`**
- ✅ **Build and fix any errors**


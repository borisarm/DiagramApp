module;

export module ui.win32.window;

import <windows.h>;
import ui.win32.d2d;
import domain.diagram;

export namespace ui::win32
{
    export extern domain::Diagram g_diagram; // declaration only
    export int run();
}

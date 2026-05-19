module;

#include <Windows.h>
#include "../Domain/shape_interfaces.h"

export module domain.serializer;

import domain.diagram;
import domain.shape_registry;
import <string>;

namespace domain
{
	// Writes the diagram to a UTF-8 JSON file at `path`.
	// Returns true on success.
	export bool save_diagram(const Diagram& diagram,
							 const wchar_t* path);

	// Loads a diagram from a JSON file at `path`, replacing the current
	// diagram contents (undo stack is cleared).
	// `registry` is used to look up IShapeClass by ID for reconstruction.
	// Returns true on success.
	export bool load_diagram(Diagram& diagram,
							 const wchar_t* path,
							 ShapeRegistry& registry);
}

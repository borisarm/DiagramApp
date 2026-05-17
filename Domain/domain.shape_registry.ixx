module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

export module domain.shape_registry;

import <vector>;
import <string>;

namespace domain
{
	using Microsoft::WRL::ComPtr;

	// One loaded plugin (either from a DLL or registered in-process)
	export struct PluginEntry
	{
		HMODULE              module = nullptr;  // nullptr for in-process registrations
		ComPtr<IShapePlugin> plugin;
	};

	export class ShapeRegistry
	{
	public:
		// Register an already-created IShapePlugin (used for built-in shapes)
		void register_plugin(IShapePlugin* plugin);

		// Scan a directory for DLLs that export DllGetShapePlugin and load each one.
		// Pass e.g. L"plugins\\" relative to the executable.
		void discover(const wchar_t* directory);

		// Flat list of all IShapeClass objects across all registered plugins.
		// This drives stencil population — no hardcoded enum needed.
		const std::vector<ComPtr<IShapeClass>>& classes() const noexcept
		{ return m_classes; }

		// Find a class by its stable string ID (used by deserializer)
		ComPtr<IShapeClass> find_class(const wchar_t* id) const;

	private:
		void load_plugin(HMODULE hmod, IShapePlugin* plugin);

		std::vector<PluginEntry>          m_plugins;
		std::vector<ComPtr<IShapeClass>>  m_classes;
	};
}

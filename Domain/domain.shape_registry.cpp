module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

module domain.shape_registry;

import <string>;

namespace domain
{
	using Microsoft::WRL::ComPtr;

	void ShapeRegistry::register_plugin(IShapePlugin* plugin)
	{
		if (!plugin) return;
		plugin->AddRef();
		PluginEntry entry;
		entry.module = nullptr;
		entry.plugin.Attach(plugin);
		load_plugin(nullptr, plugin);
		m_plugins.push_back(std::move(entry));
	}

	void ShapeRegistry::discover(const wchar_t* directory)
	{
		std::wstring pattern = directory;
		if (!pattern.empty() && pattern.back() != L'\\')
			pattern += L'\\';
		pattern += L"*ShapePlugin.dll";

		WIN32_FIND_DATAW fd{};
		HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
		if (h == INVALID_HANDLE_VALUE) return;

		std::wstring dir = directory;
		if (!dir.empty() && dir.back() != L'\\') dir += L'\\';

		do
		{
			std::wstring path = dir + fd.cFileName;
			HMODULE hmod = LoadLibraryW(path.c_str());
			if (!hmod) continue;

			auto pfn = reinterpret_cast<PFN_DllGetShapePlugin>(
				GetProcAddress(hmod, "DllGetShapePlugin"));
			if (!pfn) { FreeLibrary(hmod); continue; }

			IShapePlugin* raw = nullptr;
			if (FAILED(pfn(&raw)) || !raw) { FreeLibrary(hmod); continue; }

			PluginEntry entry;
			entry.module = hmod;
			entry.plugin.Attach(raw);
			load_plugin(hmod, raw);
			m_plugins.push_back(std::move(entry));

		} while (FindNextFileW(h, &fd));

		FindClose(h);
	}

	void ShapeRegistry::load_plugin(HMODULE /*hmod*/, IShapePlugin* plugin)
	{
		UINT32 count = 0;
		if (FAILED(plugin->GetShapeClassCount(&count))) return;

		for (UINT32 i = 0; i < count; ++i)
		{
			IShapeClass* cls = nullptr;
			if (SUCCEEDED(plugin->GetShapeClass(i, &cls)) && cls)
				m_classes.emplace_back(cls);  // ComPtr takes ownership (Attach)
		}
	}

	ComPtr<IShapeClass> ShapeRegistry::find_class(const wchar_t* id) const
	{
		for (auto& cls : m_classes)
		{
			BSTR bid = nullptr;
			if (SUCCEEDED(cls->GetId(&bid)) && bid)
			{
				bool match = (wcscmp(bid, id) == 0);
				SysFreeString(bid);
				if (match) return cls;
			}
		}
		return nullptr;
	}
}

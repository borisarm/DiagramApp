module;

#include <Windows.h>
#include <wrl/client.h>
#include "../Domain/shape_interfaces.h"

module domain.serializer;

import <string>;
import <vector>;
import <fstream>;
import <sstream>;
import <optional>;
import <charconv>;
import <cstdio>;
import <algorithm>;

namespace domain
{
	using Microsoft::WRL::ComPtr;

	// =========================================================================
	// Tiny JSON writer helpers
	// =========================================================================

	static std::string escape_json(const wchar_t* ws)
	{
		// Convert wchar_t* to UTF-8 and escape " and backslash
		std::string out;
		while (*ws)
		{
			wchar_t ch = *ws++;
			if (ch == L'"')  { out += "\\\""; }
			else if (ch == L'\\') { out += "\\\\"; }
			else if (ch < 0x80)  { out += static_cast<char>(ch); }
			else
			{
				// Simple UTF-8 encode
				if (ch < 0x800)
				{
					out += static_cast<char>(0xC0 | (ch >> 6));
					out += static_cast<char>(0x80 | (ch & 0x3F));
				}
				else
				{
					out += static_cast<char>(0xE0 | (ch >> 12));
					out += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
					out += static_cast<char>(0x80 | (ch & 0x3F));
				}
			}
		}
		return out;
	}

	// =========================================================================
	// Save
	// =========================================================================

	bool save_diagram(const Diagram& diagram, const wchar_t* path)
	{
		std::string json;
		json.reserve(4096);
		json += "{\n  \"shapes\": [\n";

		const auto& shapes = diagram.shapes();
		for (std::size_t i = 0; i < shapes.size(); ++i)
		{
			IShape* s = shapes[i].Get();

			// Class ID
			IShapeClass* cls = nullptr;
			s->GetShapeClass(&cls);
			BSTR classId = nullptr;
			if (cls) { cls->GetId(&classId); cls->Release(); }

			// Geometry
			float x = 0, y = 0, w = 0, h = 0;
			s->GetBounds(&x, &y, &w, &h);

			// Style via IShapeProperties
			UINT32 fill_color = 0xFFFFFFFF, stroke_color = 0xFF000000;
			std::string label_u8;

			IShapeProperties* props = nullptr;
			if (SUCCEEDED(s->QueryInterface(__uuidof(IShapeProperties),
											reinterpret_cast<void**>(&props))) && props)
			{
				props->GetColor(L"fill_color",   &fill_color);
				props->GetColor(L"stroke_color", &stroke_color);
				BSTR lbl = nullptr;
				if (SUCCEEDED(props->GetString(L"label", &lbl)) && lbl)
				{
					label_u8 = escape_json(lbl);
					SysFreeString(lbl);
				}
				props->Release();
			}

			char buf[512];
			std::snprintf(buf, sizeof(buf),
				"    {\"class\":\"%s\","
				"\"x\":%.4f,\"y\":%.4f,\"w\":%.4f,\"h\":%.4f,"
				"\"fill_color\":%u,\"stroke_color\":%u,"
				"\"label\":\"%s\"}",
				classId ? escape_json(classId).c_str() : "",
				x, y, w, h,
				fill_color, stroke_color,
				label_u8.c_str());

			if (classId) SysFreeString(classId);

			json += buf;
			if (i + 1 < shapes.size()) json += ",";
			json += "\n";
		}

		json += "  ],\n  \"connectors\": [\n";

		const auto& conns = diagram.connectors();
		for (std::size_t i = 0; i < conns.size(); ++i)
		{
			char buf[64];
			std::snprintf(buf, sizeof(buf),
				"    {\"source\":%zu,\"target\":%zu}",
				conns[i].source_index, conns[i].target_index);
			json += buf;
			if (i + 1 < conns.size()) json += ",";
			json += "\n";
		}

		json += "  ]\n}\n";

		// Write as UTF-8 file
		HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, nullptr,
								   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		DWORD written = 0;
		BOOL ok = WriteFile(hFile, json.data(), static_cast<DWORD>(json.size()),
							&written, nullptr);
		CloseHandle(hFile);
		return ok && written == static_cast<DWORD>(json.size());
	}

	// =========================================================================
	// Minimal JSON parser helpers
	// =========================================================================

	struct JsonObject
	{
		// Stores all key-value pairs as strings (values include quotes for strings)
		std::vector<std::pair<std::string, std::string>> entries;

		std::optional<std::string> get(const char* key) const
		{
			for (auto& e : entries)
				if (e.first == key) return e.second;
			return std::nullopt;
		}
	};

	static void skip_ws(const char*& p)
	{
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
	}

	// Returns the raw content of a JSON string (without outer quotes, unescaped)
	static std::string parse_string(const char*& p)
	{
		if (*p != '"') return {};
		++p;
		std::string out;
		while (*p && *p != '"')
		{
			if (*p == '\\' && *(p + 1))
			{
				++p;
				if (*p == '"')  out += '"';
				else if (*p == '\\') out += '\\';
				else if (*p == 'n')  out += '\n';
				else if (*p == 't')  out += '\t';
				else out += *p;
			}
			else out += *p;
			++p;
		}
		if (*p == '"') ++p;
		return out;
	}

	// Parse a simple JSON value (string, number — enough for our format)
	static std::string parse_value(const char*& p)
	{
		skip_ws(p);
		if (*p == '"') return parse_string(p);
		// number or other literal — read until delimiter
		const char* start = p;
		while (*p && *p != ',' && *p != '}' && *p != ']' && *p != '\n') ++p;
		std::string val(start, p);
		// trim trailing whitespace
		while (!val.empty() && (val.back() == ' ' || val.back() == '\r' || val.back() == '\n'))
			val.pop_back();
		return val;
	}

	// Parse one JSON object {key:value,...} into a JsonObject
	static std::optional<JsonObject> parse_object(const char*& p)
	{
		skip_ws(p);
		if (*p != '{') return std::nullopt;
		++p;
		JsonObject obj;
		while (true)
		{
			skip_ws(p);
			if (*p == '}') { ++p; break; }
			if (*p == ',') { ++p; continue; }
			if (*p != '"') return std::nullopt;
			std::string key = parse_string(p);
			skip_ws(p);
			if (*p != ':') return std::nullopt;
			++p;
			skip_ws(p);
			std::string val = parse_value(p);
			obj.entries.emplace_back(std::move(key), std::move(val));
		}
		return obj;
	}

	// Parse array of objects [...] and return them
	static std::vector<JsonObject> parse_object_array(const char*& p)
	{
		skip_ws(p);
		std::vector<JsonObject> result;
		if (*p != '[') return result;
		++p;
		while (true)
		{
			skip_ws(p);
			if (*p == ']') { ++p; break; }
			if (*p == ',') { ++p; continue; }
			if (auto obj = parse_object(p)) result.push_back(std::move(*obj));
			else break;
		}
		return result;
	}

	// Convert UTF-8 std::string → std::wstring
	static std::wstring utf8_to_wstring(const std::string& s)
	{
		if (s.empty()) return {};
		int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring ws(len, 0);
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
		if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
		return ws;
	}

	static float stof_safe(const std::string& s)
	{
		float v = 0.f;
		std::from_chars(s.data(), s.data() + s.size(), v);
		return v;
	}

	static unsigned long stoul_safe(const std::string& s)
	{
		unsigned long v = 0;
		std::from_chars(s.data(), s.data() + s.size(), v);
		return v;
	}

	// =========================================================================
	// Load
	// =========================================================================

	bool load_diagram(Diagram& diagram, const wchar_t* path,
					  ShapeRegistry& registry)
	{
		// Read whole file
		HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
								   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		DWORD size = GetFileSize(hFile, nullptr);
		std::string buf(size, '\0');
		DWORD read = 0;
		BOOL ok = ReadFile(hFile, buf.data(), size, &read, nullptr);
		CloseHandle(hFile);
		if (!ok || read != size) return false;

		// Minimal top-level parse: find "shapes" array and "connectors" array
		const char* p = buf.c_str();
		skip_ws(p);
		if (*p != '{') return false;
		++p;

		std::vector<JsonObject> shapes_arr, conns_arr;

		while (true)
		{
			skip_ws(p);
			if (*p == '}' || *p == '\0') break;
			if (*p == ',') { ++p; continue; }
			if (*p != '"') { ++p; continue; }
			std::string key = parse_string(p);
			skip_ws(p);
			if (*p != ':') return false;
			++p;
			skip_ws(p);
			if (key == "shapes")
				shapes_arr = parse_object_array(p);
			else if (key == "connectors")
				conns_arr = parse_object_array(p);
			else
				parse_value(p); // skip unknown keys
		}

		// Rebuild diagram
		// Clear current contents (we swap in a clean state)
		Diagram fresh;
		// (We'll fill `fresh` then swap — but Diagram has no clear() API.
		//  Instead rebuild a temp vector and call add_shape on fresh.)

		std::vector<ComPtr<IShape>> new_shapes;
		new_shapes.reserve(shapes_arr.size());

		for (const auto& obj : shapes_arr)
		{
			auto cls_id_opt = obj.get("class");
			if (!cls_id_opt) continue;

			std::wstring cls_id = utf8_to_wstring(*cls_id_opt);
			ComPtr<IShapeClass> cls = registry.find_class(cls_id.c_str());
			if (!cls) continue; // plugin not loaded — skip

			float x = stof_safe(obj.get("x").value_or("0"));
			float y = stof_safe(obj.get("y").value_or("0"));
			float w = stof_safe(obj.get("w").value_or("50"));
			float h = stof_safe(obj.get("h").value_or("30"));

			IShape* raw = nullptr;
			if (FAILED(cls->CreateShape(x, y, w, h, &raw)) || !raw) continue;
			ComPtr<IShape> shape(raw);

			// Restore style via IShapeProperties
			IShapeProperties* props = nullptr;
			if (SUCCEEDED(shape->QueryInterface(__uuidof(IShapeProperties),
												reinterpret_cast<void**>(&props))) && props)
			{
				if (auto fc = obj.get("fill_color"))
					props->SetColor(L"fill_color",
									static_cast<UINT32>(stoul_safe(*fc)));
				if (auto sc = obj.get("stroke_color"))
					props->SetColor(L"stroke_color",
									static_cast<UINT32>(stoul_safe(*sc)));
				if (auto lbl = obj.get("label"))
				{
					std::wstring wlbl = utf8_to_wstring(*lbl);
					props->SetString(L"label", wlbl.c_str());
				}
				props->Release();
			}

			new_shapes.push_back(std::move(shape));
		}

		// Collect connectors
		std::vector<Connector> new_conns;
		new_conns.reserve(conns_arr.size());
		for (const auto& obj : conns_arr)
		{
			std::size_t src = static_cast<std::size_t>(
				std::stoul(obj.get("source").value_or("0")));
			std::size_t tgt = static_cast<std::size_t>(
				std::stoul(obj.get("target").value_or("0")));
			if (src < new_shapes.size() && tgt < new_shapes.size() && src != tgt)
				new_conns.push_back({ src, tgt });
		}

		// Replace diagram contents
		// Remove all existing shapes (iterate backwards to avoid index shifting)
		while (!diagram.shapes().empty())
			diagram.remove_shape(diagram.shapes().back().Get());

		for (auto& s : new_shapes)
			diagram.add_shape(std::move(s));

		for (auto& c : new_conns)
			diagram.add_connector(c.source_index, c.target_index);

		return true;
	}
}

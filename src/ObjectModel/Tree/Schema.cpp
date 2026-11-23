/*
 * Schema.cpp
 */

#include "Schema.h"
#include "Debug.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace OM
{
	namespace
	{
		/** The type of an item of array type `type`, or nothing if it isn't an array */
		std::optional<std::string> itemOf(const std::string& type)
		{
			if (type == "any")
			{
				return type;
			}
			if (type.ends_with("[]"))
			{
				return type.substr(0, type.size() - 2);
			}
			return std::nullopt;
		}

		/** The type of a field of dictionary type `type` (`{T}`), or nothing if it isn't one */
		std::optional<std::string> entryOf(const std::string& type)
		{
			if (type.size() > 2 && type.front() == '{' && type.back() == '}')
			{
				return type.substr(1, type.size() - 2);
			}
			return std::nullopt;
		}
	} // namespace

	Schema& Schema::get()
	{
		static Schema s_schema;
		return s_schema;
	}

	bool Schema::load(const std::filesystem::path& file)
	{
		m_source.clear();
		m_root.clear();
		m_types.clear();
		m_enums.clear();

		std::ifstream in(file);
		const auto json = nlohmann::json::parse(in, nullptr, false);
		if (!in.is_open() || json.is_discarded() || !json.is_object())
		{
			LOG_WARN("Object model schema '{:s}' couldn't be read", file.string());
			return false;
		}
		m_source = json.value("source", "");
		m_root = json.value("root", "");
		for (const auto& [type, fields] : json.value("types", nlohmann::json::object()).items())
		{
			auto& entry = m_types[type];
			for (const auto& [field, fieldType] : fields.items())
			{
				if (fieldType.is_string())
				{
					entry.emplace(field, fieldType.get<std::string>());
				}
			}
		}
		for (const auto& [type, values] : json.value("enums", nlohmann::json::object()).items())
		{
			auto& entry = m_enums[type];
			for (const auto& value : values)
			{
				if (value.is_string())
				{
					entry.push_back(value.get<std::string>());
				}
			}
		}
		if (!m_types.contains(m_root))
		{
			LOG_WARN("Object model schema '{:s}' has no root type '{:s}'", file.string(), m_root);
			m_types.clear();
			m_enums.clear();
			return false;
		}
		return true;
	}

	std::optional<std::string> Schema::typeAt(const Path& path) const
	{
		if (empty())
		{
			return std::nullopt;
		}
		std::string type = m_root;
		for (const auto& segment : path.segments())
		{
			if (type == "any")
			{
				return type;
			}
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				if (const auto entry = entryOf(type))
				{
					type = *entry;
					continue;
				}
				const auto it = m_types.find(type);
				if (it == m_types.end())
				{
					return std::nullopt;
				}
				const auto field = it->second.find(*name);
				if (field == it->second.end())
				{
					return std::nullopt;
				}
				type = field->second;
			}
			else
			{
				const auto item = itemOf(type);
				if (!item)
				{
					return std::nullopt;
				}
				type = *item;
			}
		}
		return type;
	}

	const std::vector<std::string>* Schema::enumValues(std::string_view type) const
	{
		const auto it = m_enums.find(type);
		return it == m_enums.end() ? nullptr : &it->second;
	}

	void Schema::forEachField(const std::function<void(const std::string&, const std::string&)>& visit) const
	{
		std::vector<std::string> entered;
		if (!empty())
		{
			visitFields(m_root, "", entered, visit);
		}
	}

	void Schema::visitFields(const std::string& type,
							 const std::string& prefix,
							 std::vector<std::string>& entered,
							 const std::function<void(const std::string&, const std::string&)>& visit) const
	{
		const auto it = m_types.find(type);
		if (it == m_types.end() || std::find(entered.begin(), entered.end(), type) != entered.end())
		{
			return;
		}
		entered.push_back(type);
		for (const auto& [field, fieldType] : it->second)
		{
			// The path to the field, then to what it holds: `[]` per array level, `{}` for a dictionary's entries
			std::string path = prefix.empty() ? field : prefix + "." + field;
			std::string inner = fieldType;
			std::string held = path;
			for (;;)
			{
				if (const auto item = itemOf(inner); item && inner != "any")
				{
					held += "[]";
					inner = *item;
				}
				else if (const auto entry = entryOf(inner))
				{
					held += ".{}";
					inner = *entry;
				}
				else
				{
					break;
				}
			}
			visit(path, fieldType);
			visitFields(inner, held, entered, visit);
		}
		entered.pop_back();
	}
} // namespace OM

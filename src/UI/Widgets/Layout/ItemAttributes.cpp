/*
 * ItemAttributes.cpp
 */

#include "ItemAttributes.h"

namespace UI::Xml
{
	ItemAttributes ItemAttributes::from(const char** attrs)
	{
		ItemAttributes result;
		for (size_t i = 0; attrs != nullptr && attrs[i] != nullptr; i += 2)
		{
			const std::string_view name = attrs[i];
			if (name.starts_with(PREFIX) && name.size() > PREFIX.size())
			{
				result.m_attrs.emplace_back(name.substr(PREFIX.size()), attrs[i + 1]);
			}
		}
		return result;
	}

	std::vector<const char*> ItemAttributes::attrsFor(const std::string& name) const
	{
		std::vector<const char*> list{"name", name.c_str()};
		for (const auto& [attribute, value] : m_attrs)
		{
			list.push_back(attribute.c_str());
			list.push_back(value.c_str());
		}
		list.push_back(nullptr);
		list.push_back(nullptr);
		return list;
	}
} // namespace UI::Xml

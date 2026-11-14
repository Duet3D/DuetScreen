/*
 * OmName.cpp
 */

#include "OmName.h"

namespace UI::Xml
{
	std::optional<OmName> OmName::parse(std::string_view name)
	{
		OmName parsed;
		if (name.starts_with(PREFIX))
		{
			parsed.absolute = true;
			name.remove_prefix(PREFIX.size());
		}

		// The converter starts at the first '|' outside brackets and braces (an index may hold a name with its own)
		size_t bar = std::string_view::npos;
		int depth = 0;
		for (size_t i = 0; i < name.size(); i++)
		{
			const char c = name[i];
			if (c == '[' || c == '{')
			{
				depth++;
			}
			else if (c == ']' || c == '}')
			{
				depth--;
			}
			else if (c == '|' && depth == 0)
			{
				bar = i;
				break;
			}
		}

		parsed.path = name.substr(0, bar);
		if (parsed.path.empty())
		{
			return std::nullopt;
		}
		if (bar != std::string_view::npos)
		{
			const std::string_view converter = name.substr(bar + 1);
			const size_t colon = converter.find(':');
			parsed.converter = converter.substr(0, colon);
			if (colon != std::string_view::npos)
			{
				parsed.argument = converter.substr(colon + 1);
			}
			if (parsed.converter.empty())
			{
				return std::nullopt;
			}
		}
		return parsed;
	}
} // namespace UI::Xml

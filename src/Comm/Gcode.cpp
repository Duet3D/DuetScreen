/*
 * Gcode.cpp
 */

#include "Gcode.h"
#include "tracy/Tracy.hpp"

namespace Comm
{
	std::string quoted(std::string_view text)
	{
		ZoneScoped;
		std::string result;
		result.reserve(text.size() + 2);
		result += '"';
		for (const char c : text)
		{
			if (c == '"' || c == '\'')
			{
				result += c; // written twice
			}
			result += c;
		}
		result += '"';
		return result;
	}
} // namespace Comm

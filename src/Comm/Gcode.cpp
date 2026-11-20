/*
 * Gcode.cpp
 */

#include "Gcode.h"

namespace Comm
{
	std::string quoted(std::string_view text)
	{
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

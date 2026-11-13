/*
 * Path.cpp
 */

#include "Path.h"
#include <charconv>

namespace OM
{
	namespace
	{
		/** Whether `c` may appear in a field name: the object model's names are letters, digits and '_' */
		bool isFieldChar(char c)
		{
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
		}
	} // namespace

	std::optional<Path> Path::parse(std::string_view text)
	{
		Path path;
		size_t i = 0;
		bool expectField = true; // At the start, and after '.', a field name must follow
		while (i < text.size())
		{
			if (text[i] == '[')
			{
				const size_t close = text.find(']', i);
				if (close == std::string_view::npos || close == i + 1)
				{
					return std::nullopt;
				}
				size_t index = 0;
				const char* first = text.data() + i + 1;
				const char* last = text.data() + close;
				const auto [ptr, error] = std::from_chars(first, last, index);
				if (error != std::errc() || ptr != last)
				{
					return std::nullopt;
				}
				if (expectField)
				{
					return std::nullopt; // An index must follow a field or another index, e.g. not `a.[0]`
				}
				path.m_segments.emplace_back(index);
				i = close + 1;
				expectField = false;
				continue;
			}
			if (text[i] == '.')
			{
				if (expectField)
				{
					return std::nullopt; // Empty field name
				}
				expectField = true;
				i++;
				continue;
			}
			if (!expectField)
			{
				return std::nullopt; // e.g. `a[0]b`
			}
			const size_t start = i;
			while (i < text.size() && isFieldChar(text[i]))
			{
				i++;
			}
			if (i == start)
			{
				return std::nullopt;
			}
			path.m_segments.emplace_back(std::string(text.substr(start, i - start)));
			expectField = false;
		}
		if (expectField && !text.empty())
		{
			return std::nullopt; // Trailing '.'
		}
		return path;
	}

	std::optional<Path> Path::fromDecoderId(std::string_view id, std::span<const size_t> indices)
	{
		Path path;
		size_t used = 0;
		size_t i = 0;
		while (i < id.size())
		{
			if (id[i] == ':')
			{
				i++;
				continue;
			}
			if (id[i] == '^')
			{
				if (used == indices.size())
				{
					return std::nullopt;
				}
				path.m_segments.emplace_back(indices[used++]);
				i++;
				continue;
			}
			const size_t end = id.find_first_of(":^", i);
			const size_t stop = end == std::string_view::npos ? id.size() : end;
			path.m_segments.emplace_back(std::string(id.substr(i, stop - i)));
			i = stop;
		}
		return path;
	}

	Path Path::field(std::string_view name) const
	{
		Path result = *this;
		result.m_segments.emplace_back(std::string(name));
		return result;
	}

	Path Path::index(size_t index) const
	{
		Path result = *this;
		result.m_segments.emplace_back(index);
		return result;
	}

	Path Path::operator/(const Path& tail) const
	{
		Path result = *this;
		result.m_segments.insert(result.m_segments.end(), tail.m_segments.begin(), tail.m_segments.end());
		return result;
	}

	Path Path::parent() const
	{
		Path result = *this;
		if (!result.m_segments.empty())
		{
			result.m_segments.pop_back();
		}
		return result;
	}

	bool Path::startsWith(const Path& ancestor) const
	{
		return ancestor.m_segments.size() <= m_segments.size() &&
			   std::equal(ancestor.m_segments.begin(), ancestor.m_segments.end(), m_segments.begin());
	}

	std::string Path::str() const
	{
		std::string text;
		for (const auto& segment : m_segments)
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				if (!text.empty())
				{
					text += '.';
				}
				text += *name;
			}
			else
			{
				text += '[';
				text += std::to_string(std::get<size_t>(segment));
				text += ']';
			}
		}
		return text;
	}

	bool isAtOrBelow(std::string_view path, std::string_view ancestor)
	{
		if (ancestor.empty())
		{
			return true;
		}
		if (!path.starts_with(ancestor))
		{
			return false;
		}
		return path.size() == ancestor.size() || path[ancestor.size()] == '.' || path[ancestor.size()] == '[';
	}
} // namespace OM

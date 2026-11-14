/*
 * OmPath.cpp
 */

#include "OmPath.h"
#include <algorithm>
#include <charconv>

namespace UI::Xml
{
	namespace
	{
		/** Whether `c` may appear in a field name */
		bool isFieldChar(char c)
		{
			return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
		}

		/** Read all of `text` as a whole number */
		std::optional<int64_t> wholeNumber(std::string_view text)
		{
			int64_t value = 0;
			const char* last = text.data() + text.size();
			const auto [ptr, error] = std::from_chars(text.data(), last, value);
			if (text.empty() || error != std::errc() || ptr != last)
			{
				return std::nullopt;
			}
			return value;
		}

		/** The index a subject holds: its int, or its text as a whole number; nothing if it isn't one, or negative */
		std::optional<size_t> indexOf(lv_subject_t* subject)
		{
			std::optional<int64_t> value;
			switch (subject->type)
			{
			case LV_SUBJECT_TYPE_INT:
				value = lv_subject_get_int(subject);
				break;
			case LV_SUBJECT_TYPE_STRING:
			case LV_SUBJECT_TYPE_POINTER:
				value = wholeNumber(lv_subject_get_string(subject));
				break;
			default:
				break;
			}
			if (!value || *value < 0)
			{
				return std::nullopt;
			}
			return static_cast<size_t>(*value);
		}
	} // namespace

	std::optional<OmTemplate> OmTemplate::parse(std::string_view text)
	{
		OmTemplate result;
		size_t i = 0;
		bool expectField = true; // At the start and after '.', a field name must follow
		while (i < text.size())
		{
			const char c = text[i];
			if (c == '.')
			{
				if (expectField)
				{
					return std::nullopt;
				}
				expectField = true;
				i++;
			}
			else if (c == '[')
			{
				if (expectField)
				{
					return std::nullopt;
				}
				// Find the matching ']': a `{name}` may hold brackets of its own
				int depth = 0;
				size_t close = std::string_view::npos;
				for (size_t j = i; j < text.size(); j++)
				{
					if (text[j] == '[' || text[j] == '{')
					{
						depth++;
					}
					else if ((text[j] == ']' || text[j] == '}') && --depth == 0)
					{
						close = j;
						break;
					}
				}
				if (close == std::string_view::npos)
				{
					return std::nullopt;
				}
				const std::string_view inside = text.substr(i + 1, close - i - 1);
				if (inside.size() > 2 && inside.front() == '{' && inside.back() == '}')
				{
					result.m_segments.emplace_back(Ref{std::string(inside.substr(1, inside.size() - 2))});
				}
				else if (const auto number = wholeNumber(inside); number && *number >= 0)
				{
					result.m_segments.emplace_back(static_cast<size_t>(*number));
				}
				else
				{
					return std::nullopt;
				}
				i = close + 1;
			}
			else
			{
				if (!expectField)
				{
					return std::nullopt;
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
				result.m_segments.emplace_back(std::string(text.substr(start, i - start)));
				expectField = false;
			}
		}
		if (expectField)
		{
			return std::nullopt; // Empty, or a trailing '.'
		}
		return result;
	}

	OmBoundPath::OmBoundPath(const OM::Path& path)
	{
		for (const auto& segment : path.segments())
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				m_segments.emplace_back(*name);
			}
			else
			{
				m_segments.emplace_back(std::get<size_t>(segment));
			}
		}
	}

	std::optional<OmBoundPath> OmBoundPath::bind(const OmTemplate& path, const Lookup& lookup, std::string& missing)
	{
		OmBoundPath result;
		for (const auto& segment : path.segments())
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				result.m_segments.emplace_back(*name);
			}
			else if (const auto* index = std::get_if<size_t>(&segment))
			{
				result.m_segments.emplace_back(*index);
			}
			else
			{
				const auto& ref = std::get<OmTemplate::Ref>(segment);
				lv_subject_t* subject = lookup(ref.name);
				if (subject == nullptr)
				{
					missing = ref.name;
					return std::nullopt;
				}
				result.m_segments.emplace_back(subject);
			}
		}
		return result;
	}

	OmBoundPath OmBoundPath::operator/(const OmBoundPath& tail) const
	{
		OmBoundPath result = *this;
		result.m_segments.insert(result.m_segments.end(), tail.m_segments.begin(), tail.m_segments.end());
		return result;
	}

	bool OmBoundPath::indirect() const
	{
		return std::any_of(m_segments.begin(),
						   m_segments.end(),
						   [](const Segment& segment) { return std::holds_alternative<lv_subject_t*>(segment); });
	}

	std::vector<lv_subject_t*> OmBoundPath::subjects() const
	{
		std::vector<lv_subject_t*> result;
		for (const auto& segment : m_segments)
		{
			if (const auto* subject = std::get_if<lv_subject_t*>(&segment))
			{
				result.push_back(*subject);
			}
		}
		return result;
	}

	std::optional<OM::Path> OmBoundPath::resolve() const
	{
		OM::Path path;
		for (const auto& segment : m_segments)
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				path = path.field(*name);
			}
			else if (const auto* index = std::get_if<size_t>(&segment))
			{
				path = path.index(*index);
			}
			else
			{
				const auto value = indexOf(std::get<lv_subject_t*>(segment));
				if (!value)
				{
					return std::nullopt;
				}
				path = path.index(*value);
			}
		}
		return path;
	}

	std::optional<OmBoundText> OmBoundText::bind(std::string_view text,
												 const OmBoundPath::Lookup& lookup,
												 std::string& missing)
	{
		OmBoundText result;
		size_t from = 0;
		while (from < text.size())
		{
			const size_t open = text.find('{', from);
			if (open == std::string_view::npos)
			{
				result.m_parts.emplace_back(std::string(text.substr(from)));
				break;
			}
			const size_t close = text.find('}', open);
			if (close == std::string_view::npos)
			{
				missing = text.substr(open);
				return std::nullopt;
			}
			if (open > from)
			{
				result.m_parts.emplace_back(std::string(text.substr(from, open - from)));
			}
			const std::string_view name = text.substr(open + 1, close - open - 1);
			lv_subject_t* subject = lookup(name);
			if (subject == nullptr)
			{
				missing = name;
				return std::nullopt;
			}
			result.m_parts.emplace_back(subject);
			from = close + 1;
		}
		return result;
	}

	bool OmBoundText::indirect() const
	{
		return std::any_of(m_parts.begin(),
						   m_parts.end(),
						   [](const Part& part) { return std::holds_alternative<lv_subject_t*>(part); });
	}

	std::vector<lv_subject_t*> OmBoundText::subjects() const
	{
		std::vector<lv_subject_t*> result;
		for (const auto& part : m_parts)
		{
			if (const auto* subject = std::get_if<lv_subject_t*>(&part))
			{
				result.push_back(*subject);
			}
		}
		return result;
	}

	std::string OmBoundText::resolve() const
	{
		std::string text;
		for (const auto& part : m_parts)
		{
			if (const auto* literal = std::get_if<std::string>(&part))
			{
				text += *literal;
				continue;
			}
			lv_subject_t* subject = std::get<lv_subject_t*>(part);
			if (subject->type == LV_SUBJECT_TYPE_INT)
			{
				text += std::to_string(lv_subject_get_int(subject));
			}
			else if (subject->type == LV_SUBJECT_TYPE_STRING || subject->type == LV_SUBJECT_TYPE_POINTER)
			{
				text += lv_subject_get_string(subject);
			}
		}
		return text;
	}

	lv_subject_t* lookupSubject(const lv_xml_subject_ctx_t* origin, std::string_view name)
	{
		const std::string terminated(name);
		if (lv_subject_t* subject = lv_xml_subject_ctx_find(origin, terminated.c_str()))
		{
			return subject;
		}
		// The globals, the global resolver and the last chance, as the engine looks; it starts from the active
		// context, which is `origin` while a view is being built
		return lv_xml_find_subject(nullptr, terminated.c_str());
	}
} // namespace UI::Xml

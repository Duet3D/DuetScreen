/*
 * Patch.cpp
 */

#include "Patch.h"
#include <charconv>

namespace OM
{
	void Patch::reset()
	{
		m_root = Path();
		m_live = false;
		m_first = 0;
		m_count = 0;
		m_next = 0;
		m_hasResult = false;
		m_result.setValue(Node::Kind::null, "");
	}

	void Patch::setKey(std::string_view key)
	{
		m_root = Path::parse(key).value_or(Path());
	}

	void Patch::setFlags(std::string_view flags)
	{
		m_live = flags.find('f') != std::string_view::npos;
		m_first = 0;
		const size_t a = flags.find('a');
		if (a != std::string_view::npos)
		{
			const char* first = flags.data() + a + 1;
			std::from_chars(first, flags.data() + flags.size(), m_first);
		}
	}

	void Patch::value(const Path& path, Node::Kind kind, std::string_view text)
	{
		make(offset(path)).setValue(kind, text);
	}

	void Patch::objectStart(const Path& path)
	{
		make(offset(path)).makeObject();
	}

	void Patch::arrayEnd(const Path& path, size_t length)
	{
		if (path.empty())
		{
			// The result itself: its elements sit from m_first on
			m_count = length;
			make(path).resize(m_first + length);
			return;
		}
		make(offset(path)).resize(length);
	}

	Path Patch::offset(const Path& path) const
	{
		if (m_first == 0 || path.empty())
		{
			return path;
		}
		const auto* index = std::get_if<size_t>(&path.segments().front());
		if (index == nullptr)
		{
			return path;
		}
		Path shifted = Path().index(*index + m_first);
		for (size_t i = 1; i < path.size(); i++)
		{
			const auto& segment = path.segments()[i];
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				shifted = shifted.field(*name);
			}
			else
			{
				shifted = shifted.index(std::get<size_t>(segment));
			}
		}
		return shifted;
	}

	Node& Patch::make(const Path& path)
	{
		m_hasResult = true;
		Node* node = &m_result;
		for (const auto& segment : path.segments())
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				node = &node->field(*name);
			}
			else
			{
				node = &node->item(std::get<size_t>(segment));
			}
		}
		return *node;
	}
} // namespace OM

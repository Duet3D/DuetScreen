/*
 * Node.cpp
 */

#include "Node.h"
#include <charconv>
#include <cmath>

namespace OM
{
	namespace
	{
		/** Read `text` as a number if all of it is one */
		bool parseNumber(std::string_view text, double& value)
		{
			if (text.empty())
			{
				return false;
			}
			const char* last = text.data() + text.size();
			const auto [ptr, error] = std::from_chars(text.data(), last, value);
			return error == std::errc() && ptr == last;
		}

		/** A node of kind `missing` */
		struct MissingNode
		{
			/** Build it */
			MissingNode() { node.setValue(Node::Kind::missing, ""); }

			Node node; // The node
		};
	} // namespace

	Node::Node(Kind kind, std::string_view text)
	{
		setValue(kind, text);
	}

	Node::Node(Node&& other) noexcept
		: m_kind(other.m_kind)
		, m_text(std::move(other.m_text))
		, m_number(other.m_number)
		, m_fields(std::move(other.m_fields))
		, m_items(std::move(other.m_items))
	{
		other.m_kind = Kind::null;
		other.reset();
	}

	Node& Node::operator=(Node&& other) noexcept
	{
		if (this != &other)
		{
			m_kind = other.m_kind;
			m_text = std::move(other.m_text);
			m_number = other.m_number;
			m_fields = std::move(other.m_fields);
			m_items = std::move(other.m_items);
			other.m_kind = Kind::null;
			other.reset();
		}
		return *this;
	}

	const Node& Node::missing()
	{
		static const MissingNode s_missing;
		return s_missing.node;
	}

	double Node::number(double fallback) const
	{
		switch (m_kind)
		{
		case Kind::number:
			return m_number;
		case Kind::boolean:
			return m_text == "true" ? 1 : 0;
		case Kind::string:
		{
			double value = 0;
			return parseNumber(m_text, value) ? value : fallback;
		}
		default:
			return fallback;
		}
	}

	int64_t Node::integer(int64_t fallback) const
	{
		const double value = number(std::nan(""));
		return std::isnan(value) ? fallback : static_cast<int64_t>(std::llround(value));
	}

	bool Node::boolean(bool fallback) const
	{
		switch (m_kind)
		{
		case Kind::boolean:
			return m_text == "true";
		case Kind::number:
			return m_number != 0;
		default:
			return fallback;
		}
	}

	size_t Node::size() const
	{
		switch (m_kind)
		{
		case Kind::array:
			return m_items.size();
		case Kind::object:
			return m_fields.size();
		default:
			return 0;
		}
	}

	const Node& Node::operator[](size_t index) const
	{
		if (m_kind != Kind::array || index >= m_items.size())
		{
			return missing();
		}
		return *m_items[index];
	}

	const Node& Node::operator[](std::string_view name) const
	{
		if (m_kind != Kind::object)
		{
			return missing();
		}
		const auto it = m_fields.find(name);
		return it == m_fields.end() ? missing() : *it->second;
	}

	const Node& Node::at(const Path& path) const
	{
		const Node* node = this;
		for (const auto& segment : path.segments())
		{
			if (const auto* name = std::get_if<std::string>(&segment))
			{
				node = &(*node)[std::string_view(*name)];
			}
			else
			{
				node = &(*node)[std::get<size_t>(segment)];
			}
			if (node->m_kind == Kind::missing)
			{
				break;
			}
		}
		return *node;
	}

	const Node& Node::at(std::string_view path) const
	{
		const auto parsed = Path::parse(path);
		return parsed ? at(*parsed) : missing();
	}

	void Node::forEachField(const std::function<void(std::string_view, const Node&)>& visit) const
	{
		if (m_kind != Kind::object)
		{
			return;
		}
		for (const auto& [name, child] : m_fields)
		{
			visit(name, *child);
		}
	}

	std::string Node::toJson() const
	{
		const auto quoted = [](std::string_view text)
		{
			std::string out = "\"";
			for (const char c : text)
			{
				if (c == '"' || c == '\\')
				{
					out += '\\';
				}
				if (c == '\n')
				{
					out += "\\n";
					continue;
				}
				out += c;
			}
			return out + "\"";
		};
		switch (m_kind)
		{
		case Kind::object:
		{
			std::string out = "{";
			for (const auto& [name, child] : m_fields)
			{
				out += (out.size() > 1 ? "," : "") + quoted(name) + ":" + child->toJson();
			}
			return out + "}";
		}
		case Kind::array:
		{
			std::string out = "[";
			for (const auto& item : m_items)
			{
				out += (out.size() > 1 ? "," : "") + item->toJson();
			}
			return out + "]";
		}
		case Kind::string:
			return quoted(m_text);
		case Kind::boolean:
		case Kind::number:
			return m_text;
		default:
			return "null";
		}
	}

	bool Node::operator==(const Node& other) const
	{
		if (m_kind != other.m_kind || m_text != other.m_text)
		{
			return false;
		}
		if (m_kind == Kind::array)
		{
			if (m_items.size() != other.m_items.size())
			{
				return false;
			}
			for (size_t i = 0; i < m_items.size(); i++)
			{
				if (!(*m_items[i] == *other.m_items[i]))
				{
					return false;
				}
			}
		}
		if (m_kind == Kind::object)
		{
			if (m_fields.size() != other.m_fields.size())
			{
				return false;
			}
			for (auto a = m_fields.begin(), b = other.m_fields.begin(); a != m_fields.end(); ++a, ++b)
			{
				if (a->first != b->first || !(*a->second == *b->second))
				{
					return false;
				}
			}
		}
		return true;
	}

	void Node::setValue(Kind kind, std::string_view text)
	{
		reset();
		m_kind = kind;
		if (kind == Kind::boolean || kind == Kind::number || kind == Kind::string)
		{
			m_text = text;
		}
		if (kind == Kind::number && !parseNumber(m_text, m_number))
		{
			m_number = 0;
		}
	}

	void Node::makeObject()
	{
		if (m_kind != Kind::object)
		{
			reset();
			m_kind = Kind::object;
		}
	}

	void Node::makeArray()
	{
		if (m_kind != Kind::array)
		{
			reset();
			m_kind = Kind::array;
		}
	}

	Node& Node::field(std::string_view name)
	{
		makeObject();
		auto it = m_fields.find(name);
		if (it == m_fields.end())
		{
			it = m_fields.emplace(std::string(name), std::make_unique<Node>()).first;
		}
		return *it->second;
	}

	Node& Node::item(size_t index)
	{
		makeArray();
		if (index >= m_items.size())
		{
			resize(index + 1);
		}
		return *m_items[index];
	}

	void Node::resize(size_t size)
	{
		makeArray();
		const size_t old = m_items.size();
		m_items.resize(size);
		for (size_t i = old; i < size; i++)
		{
			m_items[i] = std::make_unique<Node>();
		}
	}

	void Node::removeField(std::string_view name)
	{
		if (m_kind != Kind::object)
		{
			return;
		}
		const auto it = m_fields.find(name);
		if (it != m_fields.end())
		{
			m_fields.erase(it);
		}
	}

	void Node::reset()
	{
		m_text.clear();
		m_number = 0;
		m_fields.clear();
		m_items.clear();
	}
} // namespace OM

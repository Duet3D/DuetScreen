/*
 * Json.cpp
 */

#include "Json.h"
#include <charconv>
#include <nlohmann/json.hpp>
#include <vector>

namespace OM
{
	namespace
	{
		/** Builds a Node as nlohmann's parser reads the text, without an nlohmann::json in between */
		class NodeBuilder
		{
		  public:
			using Json = nlohmann::json; // The parser's own types

			/** @param root Where the value read goes */
			explicit NodeBuilder(Node& root)
				: m_root(root)
			{
			}

			/** Why reading stopped, if it failed */
			const JsonError& error() const { return m_error; }

			/** A null */
			bool null() { return value(Node::Kind::null, ""); }

			/** true or false */
			bool boolean(bool value) { return this->value(Node::Kind::boolean, value ? "true" : "false"); }

			/** A whole number below 0 */
			bool number_integer(Json::number_integer_t value) { return integer(value); }

			/** A whole number of 0 or more */
			bool number_unsigned(Json::number_unsigned_t value) { return integer(value); }

			/** Any other number; `text` is the number as written, which the node keeps */
			bool number_float(Json::number_float_t, const Json::string_t& text)
			{
				return value(Node::Kind::number, text);
			}

			/** A string, escapes already turned into UTF-8 */
			bool string(Json::string_t& text) { return value(Node::Kind::string, text); }

			/** Binary data, which JSON text never has */
			bool binary(Json::binary_t&) { return fail("binary data"); }

			/** An object starts */
			bool start_object(std::size_t)
			{
				Node* node = place();
				if (node == nullptr)
				{
					return false;
				}
				node->makeObject();
				m_stack.push_back(node);
				return true;
			}

			/** The name of the object field whose value comes next */
			bool key(Json::string_t& name)
			{
				m_key = std::move(name);
				return true;
			}

			/** The innermost object ends */
			bool end_object()
			{
				m_stack.pop_back();
				return true;
			}

			/** An array starts */
			bool start_array(std::size_t)
			{
				Node* node = place();
				if (node == nullptr)
				{
					return false;
				}
				node->makeArray();
				m_stack.push_back(node);
				return true;
			}

			/** The innermost array ends */
			bool end_array()
			{
				m_stack.pop_back();
				return true;
			}

			/** The text isn't JSON */
			bool parse_error(std::size_t position, const std::string&, const nlohmann::detail::exception& error)
			{
				m_error = JsonError{.position = position, .message = error.what()};
				return false;
			}

		  private:
			/** A whole number, kept as its decimal text */
			template <typename T>
			bool integer(T number)
			{
				char text[24];
				const auto [end, error] = std::to_chars(std::begin(text), std::end(text), number);
				return value(Node::Kind::number, std::string_view(text, static_cast<size_t>(end - text)));
			}

			/** A value that isn't an object or array */
			bool value(Node::Kind kind, std::string_view text)
			{
				Node* node = place();
				if (node == nullptr)
				{
					return false;
				}
				node->setValue(kind, text);
				return true;
			}

			/**
			 * The node the next value goes in: the root, the next item of the array being read, or the field just named
			 * of the object being read
			 * @return the node, or nullptr if the nesting is too deep
			 */
			Node* place()
			{
				if (m_stack.empty())
				{
					return &m_root;
				}
				if (m_stack.size() >= maxJsonDepth)
				{
					fail("nesting deeper than " + std::to_string(maxJsonDepth));
					return nullptr;
				}
				Node& parent = *m_stack.back();
				return parent.isArray() ? &parent.item(parent.size()) : &parent.field(m_key);
			}

			/** Stop reading because of `what` */
			bool fail(std::string what)
			{
				m_error.message = std::move(what);
				return false;
			}

			Node& m_root;				// The value read
			std::vector<Node*> m_stack; // The objects and arrays being read, innermost last
			std::string m_key;			// The field name the next value of an object goes under
			JsonError m_error;			// Why reading stopped
		};
	} // namespace

	std::expected<Node, JsonError> parseJson(std::string_view text)
	{
		Node root;
		NodeBuilder builder(root);
		if (!nlohmann::json::sax_parse(text, &builder))
		{
			return std::unexpected(builder.error());
		}
		return root;
	}
} // namespace OM

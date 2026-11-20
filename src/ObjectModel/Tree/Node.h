/*
 * Node.h
 *
 * One node of the object model: an object (named children), an array (indexed children) or a value (null, a bool, a
 * number or a string), as the JSON from RepRapFirmware had it. Numbers keep the text they arrived as, so they can be
 * shown exactly as sent.
 *
 * Reading never fails: asking for a child that isn't there gives Node::missing(), a shared node that reads as null, so
 * `root["heat"]["heaters"][1]["current"].number()` is always safe and gives 0 if any step is missing.
 */

#pragma once

#include "Path.h"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace OM
{
	class Node
	{
	  public:
		/** What a node holds */
		enum class Kind : uint8_t
		{
			missing, // Not in the object model at all (only Node::missing() is this)
			null,	 // JSON null
			boolean, // true or false
			number,	 // A JSON number
			string,	 // A JSON string
			object,	 // Named children
			array,	 // Indexed children
		};

		/** A null node */
		Node() = default;

		/** A value node: `text` is the JSON text of a bool or number, or the string itself */
		Node(Kind kind, std::string_view text);

		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;

		/** Take what `other` holds, children included; `other` is left null */
		Node(Node&& other) noexcept;

		/** Take what `other` holds, children included, instead of what this held; `other` is left null */
		Node& operator=(Node&& other) noexcept;

		/** The node every failed lookup gives: it reads as null and has no children */
		static const Node& missing();

		// -- Reading -------------------------------------------------------------------------------------------------

		/** What the node holds */
		Kind kind() const { return m_kind; }

		/** Whether the node is null or missing */
		bool isNull() const { return m_kind == Kind::null || m_kind == Kind::missing; }

		/** Whether the node is there and not null */
		bool exists() const { return !isNull(); }

		/** Whether the node is an object */
		bool isObject() const { return m_kind == Kind::object; }

		/** Whether the node is an array */
		bool isArray() const { return m_kind == Kind::array; }

		/** Whether the node is a bool, number or string */
		bool isValue() const { return m_kind == Kind::boolean || m_kind == Kind::number || m_kind == Kind::string; }

		/** The value as text: a string itself, a number as received, "true" or "false"; empty for anything else */
		std::string_view text() const { return m_text; }

		/**
		 * The value as a number: a number, 1 or 0 for a bool, or a string that is wholly a number
		 * @param fallback What anything else gives
		 */
		double number(double fallback = 0) const;

		/** number(), rounded to the nearest whole number */
		int64_t integer(int64_t fallback = 0) const;

		/** The value as a bool: a bool, or whether a number is non-zero; `fallback` for anything else */
		bool boolean(bool fallback = false) const;

		/** Number of children: items of an array or fields of an object; 0 for anything else */
		size_t size() const;

		/** The array item at `index`, or missing() */
		const Node& operator[](size_t index) const;

		/** The object field `name`, or missing() */
		const Node& operator[](std::string_view name) const;

		/** The node at `path` below this one, or missing() */
		const Node& at(const Path& path) const;

		/** The node at the path written as `path` (see Path::parse()) below this one, or missing() */
		const Node& at(std::string_view path) const;

		/** Call `visit(name, child)` for each field of an object, in name order */
		void forEachField(const std::function<void(std::string_view, const Node&)>& visit) const;

		/** The node as JSON text, children included, e.g. for a log */
		std::string toJson() const;

		/** Whether two nodes hold the same thing, children included */
		bool operator==(const Node& other) const;

		// -- Building (used by Patch and Tree) ---------------------------------------------------------------------

		/** Hold a value instead of whatever the node held */
		void setValue(Kind kind, std::string_view text);

		/** Become an empty object, unless it already is an object */
		void makeObject();

		/** Become an empty array, unless it already is an array */
		void makeArray();

		/** The field `name`, made (null) if it isn't there; the node becomes an object first if it isn't one */
		Node& field(std::string_view name);

		/** The item at `index`, made (null, as are any before it) if the array is shorter; the node becomes an array
		 * first if it isn't one */
		Node& item(size_t index);

		/** Make an array `size` items long, adding null items or removing them from the end */
		void resize(size_t size);

		/** Remove the field `name` from an object; nothing happens if it isn't there */
		void removeField(std::string_view name);


	  private:
		/** Drop children and value */
		void reset();

		Kind m_kind = Kind::null;									 // What the node holds
		std::string m_text;											 // Value as text; empty for anything but a value
		double m_number = 0;										 // A number's value
		std::map<std::string, std::unique_ptr<Node>, std::less<>> m_fields; // An object's fields
		std::vector<std::unique_ptr<Node>> m_items;					 // An array's items
	};
} // namespace OM

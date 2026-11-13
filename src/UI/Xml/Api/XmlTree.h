/*
 * XmlTree.h
 *
 * A component file read into a tree of elements, for the tools that inspect components without building them (the
 * UI API reference and its lint).
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace UI::Xml::Api
{
	/** One element of an XML file */
	struct Element
	{
		std::string tag;										// Element name
		std::vector<std::pair<std::string, std::string>> attrs; // Attributes in order
		int line = 0;											// Line it starts on
		std::vector<Element> children;							// Child elements in order

		/** The value of attribute `name`, or nullptr */
		const std::string* attr(std::string_view name) const;

		/** The first child called `name`, or nullptr */
		const Element* child(std::string_view name) const;
	};

	/** The root element of the XML file at `path`, or nullopt (with `error` set) if it can't be read or parsed */
	std::optional<Element> parseXmlFile(const std::string& path, std::string& error);
} // namespace UI::Xml::Api

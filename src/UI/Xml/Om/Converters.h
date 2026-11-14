/*
 * Converters.h
 *
 * What an object model name's subject holds. A binding's subject is made when the view is built, usually before the
 * printer has sent anything, so its type can't come from the data: it comes from the name. Without a converter
 * (`om.heat.heaters[1].current`) the subject holds the value as text; with one (`…current|int`) it holds what the
 * converter makes of the value.
 *
 * A converter is given the field, and where it is (for one that needs the item's number). Converters are listed in the
 * UI API reference (docs/XML_UI_API.md). Adding one is the only C++ an object model field ever needs, and only for a
 * new kind of conversion.
 */

#pragma once

#include "ObjectModel/Tree/Node.h"
#include "ObjectModel/Tree/Path.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace UI::Xml
{
	struct Converter
	{
		/** What a converter's subject holds */
		enum class Type
		{
			text,	 // A string subject
			integer, // An int subject
		};

		std::string_view name;		  // As written after '|'; empty for the default (the value as text)
		Type type;					  // What its subject holds
		std::string_view description; // What it gives, for the UI API reference
		std::string_view argument;	  // What its argument is, for the UI API reference; empty if it takes none
		int32_t (*toInteger)(const OM::Node& node, const OM::Path& path, std::string_view argument); // For integer
		std::string (*toText)(const OM::Node& node, const OM::Path& path, std::string_view argument); // For text
	};

	/** The converter called `name` ("" for the default, the value as text), or nullptr if there is none */
	const Converter* findConverter(std::string_view name);

	/** Every converter, the default first */
	std::span<const Converter> allConverters();
} // namespace UI::Xml

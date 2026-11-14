/*
 * ItemAttributes.h
 *
 * `item.<attribute>` on a widget that builds a component for each row or item (<ds_list>, <ds_scope>) is given to
 * every item it builds, as if written where the item is used: a prop of the item's component, or an attribute of its
 * root. So one item component can be shown with different data or sizes, as a component used by name can be:
 *
 *     <ds_list list="om.fans" item="gauge_bar" item.title="row.name|default:tr.fine_tune.fan"
 *              item.value="row.actualValue|percent" item.flex_grow="1"/>
 *
 * The values are taken as written where the widget is, `$props` already replaced, and their names are looked up in
 * each item's own scope, so `row.*` is the item's row.
 */

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace UI::Xml
{
	class ItemAttributes
	{
	  public:
		/** What an item attribute is written after */
		static constexpr std::string_view PREFIX = "item.";

		/** The `item.*` attributes of `attrs`, an XML attribute list (name, value, …, nullptr) */
		static ItemAttributes from(const char** attrs);

		/**
		 * An XML attribute list for building an item: `name` as its name, then each item attribute, ending in
		 * nullptr. The pointers are this object's and `name`'s, so both must outlive the list.
		 */
		std::vector<const char*> attrsFor(const std::string& name) const;

	  private:
		std::vector<std::pair<std::string, std::string>> m_attrs; // Each attribute without `item.`, and its value
	};
} // namespace UI::Xml

/*
 * Widgets.h
 *
 * The DuetScreen widgets XML can use in addition to LVGL's `lv_*` widgets (`<ds_button>`, `<ds_list>`, …). Each
 * registers itself with a WidgetSpec: its element name, what it does and the attributes it takes. The specs are the
 * widgets' documentation: they are listed, with every attribute, in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "helix_xml.h"
#include <string>
#include <vector>

namespace UI::Xml
{
	/** One attribute a widget takes */
	struct WidgetAttribute
	{
		std::string name;		 // As written, e.g. "bind_value"
		std::string description; // What it does and the values it takes
	};

	/** What a widget is, for the UI API reference */
	struct WidgetSpec
	{
		std::string name; // Registered element name, e.g. "ds_bar", or "ds_tabs-tab" for an element of a ds_tabs
		std::string extends = {}; // The widget whose attributes it also takes, e.g. "lv_obj"; empty for none
		std::string description;  // What it is and how it behaves
		std::string example = {}; // A short XML example; empty for none
		std::vector<WidgetAttribute> attributes = {}; // Its own attributes, in the order worth reading them
	};

	namespace Widgets
	{
		/** Register every DuetScreen widget with the XML engine. Call after lv_xml_init(). */
		void registerAll();

		/** Register an element with the XML engine as `spec.name`, and remember its spec */
		void add(WidgetSpec spec, lv_xml_widget_create_cb_t create, lv_xml_widget_apply_cb_t apply);

		/** The specs of every element registered with add(), in name order */
		std::vector<const WidgetSpec*> specs();
	} // namespace Widgets
} // namespace UI::Xml

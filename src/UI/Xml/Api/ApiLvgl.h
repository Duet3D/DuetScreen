/*
 * ApiLvgl.h
 *
 * What the XML engine's own widgets and elements take, read from the descriptions it ships (helix-xml's xmls/, kept
 * true to the engine by its test_widget_descriptions): each element's attributes with their types and help, the enum
 * types those name, and the properties a `<style>` takes. The schema (ApiXsd.h) declares them, so an editor completes
 * and documents `lv_label`, `bind_flag_if_eq` and the rest as it does DuetScreen's own elements.
 */

#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Xml::Api
{
	/** An attribute an LVGL element takes */
	struct LvglAttribute
	{
		std::string name; // As written, e.g. "long_mode" or "value-animated"
		std::string type; // As described, e.g. "int", "enum:lv_align", "subject"; empty if not given
		std::string help; // What it does
	};

	/** An LVGL element: a widget, or a child element of one */
	struct LvglElement
	{
		std::string name;					   // As registered: "lv_label", or "lv_obj-bind_flag_if_eq" for a child
		std::string help = {};				   // What it is; empty for a widget, which the descriptions don't say
		bool isObject = false;				   // It is a widget, so it also takes lv_obj's attributes
		std::vector<LvglAttribute> attributes = {}; // Its own attributes, in described order
	};

	/** An enum type attributes name, e.g. lv_align */
	struct LvglEnum
	{
		std::string name;											// e.g. "lv_align"
		std::vector<std::pair<std::string, std::string>> values = {}; // Each value and its help, in described order
	};

	/** Everything the descriptions say */
	struct LvglApi
	{
		std::vector<LvglElement> elements;			// Every described element, in name order
		std::vector<LvglEnum> enums;				// Every enum type, in name order
		std::vector<LvglAttribute> styleProperties; // What a `<style>` in `<styles>` takes, e.g. bg_color
	};

	/** Read the descriptions in directory `dir`, or nullopt (with `error` set) if one can't be read */
	std::optional<LvglApi> readLvglApi(const std::string& dir, std::string& error);
} // namespace UI::Xml::Api

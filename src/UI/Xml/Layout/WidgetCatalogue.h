/*
 * WidgetCatalogue.h
 *
 * The widgets a user can place in a layout (see Layout.h), each described by a widget file beside its component:
 * `components/dashboard/fan.widget.json` describes the widget `fan`.
 *
 *     {
 *       "component": "dashboard_fan",              // The XML component shown; default "widget_<id>"
 *       "title_tag": "widget.fan.title",           // Its name in the widget pool (or "title": "Fan")
 *       "description_tag": "widget.fan.description",
 *       "icon": "fan.png",
 *       "slots": ["dashboard"],                    // The layout slots it can be placed in
 *       "size": {"default": [3, 2], "min": [2, 1], "max": [5, 3]},   // In grid cells: [columns, rows]
 *       "props": {"style": "card"},                // Props always given to the component
 *       "settings": [                              // Props the user chooses; see SettingDef
 *         {"name": "fan", "kind": "row", "list": "fans", "key": "number", "label": "name", "default": 0,
 *          "title_tag": "widget.fan.which"}
 *       ]
 *     }
 *
 * A placed widget is its component, built with `props` and the values of its settings as the component's props, so
 * the component uses them as `$fan` (or `${fan}` inside a longer value). One component can be several widgets: each
 * sidebar page button is the same component with different `props`.
 *
 * Widget ids are global like component names: a widget in a later bundle replaces the one with the same id.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** How a setting's value is chosen on the screen */
	enum class SettingKind
	{
		Int,	// A whole number between min and max, typed on the number pad
		Bool,	// On or off, with a switch
		Text,	// Text, typed on the keyboard
		Choice, // One of a fixed list of options
		Row		// One row of a list the UI publishes (a fan, a tool, …), by the value of one of its fields
	};

	/** One of a Choice setting's options */
	struct SettingOption
	{
		std::string value{};	  // The value, as XML writes it
		std::string title{};	  // Shown name, if there is no translation
		std::string titleTag{}; // Translation key of the shown name; empty for none
	};

	/** A widget setting: a prop of the widget's component that the user chooses */
	struct SettingDef
	{
		std::string name{};		   // The prop it sets
		SettingKind kind{};		   // How it is chosen
		std::string title{};		   // Shown name, if there is no translation
		std::string titleTag{};	   // Translation key of the shown name; empty for none
		nlohmann::json fallback{};   // The value until the user chooses one ("default" in the widget file)
		int32_t min = std::numeric_limits<int32_t>::min(); // Int: smallest value
		int32_t max = std::numeric_limits<int32_t>::max(); // Int: largest value
		std::string unit{};							// Int: shown after the value, e.g. "s"
		std::vector<SettingOption> options{};			// Choice: the values to choose from
		std::string list{};							// Row: the global list whose rows are chosen from, e.g. "fans"
		std::string key{};							// Row: the rows' field whose value is stored, e.g. "number"
		std::string label{};							// Row: the rows' field shown, e.g. "name"
		size_t maxLength = 64;						// Text: longest value, in bytes

		/**
		 * `value` as a value of this setting, or nullopt if it isn't one. Numbers and text are accepted for each
		 * other where they mean the same, e.g. "5" for an Int; a Row value isn't checked against the list, whose
		 * rows come and go.
		 */
		std::optional<nlohmann::json> accept(const nlohmann::json& value) const;

		/** `value` (an accepted one) as XML writes a prop value: "5", "true", "Bed" */
		static std::string toProp(const nlohmann::json& value);
	};

	/** A size in grid cells */
	struct CellSize
	{
		int32_t columns = 1; // Width in grid columns
		int32_t rows = 1;	 // Height in grid rows

		/** Equal if both dimensions are */
		bool operator==(const CellSize&) const = default;
	};

	/** A widget of the pool: a component that can be placed in layout slots, and its settings */
	struct WidgetDef
	{
		std::string id{};					   // Name layouts use, from the widget file's name
		std::string component{};			   // XML component shown
		std::string bundle{};				   // Id of the bundle it came from
		std::filesystem::path source{};	   // The widget file
		std::string title{};				   // Shown name, if there is no translation
		std::string titleTag{};			   // Translation key of the shown name; empty for none
		std::string description{};		   // What it shows, if there is no translation
		std::string descriptionTag{};		   // Translation key of the description; empty for none
		std::string icon{};				   // Image shown in the widget pool; empty for none
		std::vector<std::string> slots{};	   // Layout slots it can be placed in
		CellSize defaultSize{2, 2};		   // Size when added to a grid
		CellSize minSize{1, 1};			   // Smallest size in a grid
		CellSize maxSize{100, 100};		   // Largest size in a grid
		std::map<std::string, std::string> props{}; // Props always given to the component
		std::vector<SettingDef> settings{};		  // Props the user chooses

		/** The setting called `name`, or nullptr */
		const SettingDef* setting(std::string_view name) const;

		/** Whether the widget can be placed in `slot` */
		bool fits(std::string_view slot) const;

		/** The component's props for these setting values: `props`, then each setting's value or its default */
		std::map<std::string, std::string> propsFor(const nlohmann::json& settings) const;
	};

	namespace WidgetCatalogue
	{
		/** The file name ending that marks a widget file */
		inline constexpr std::string_view FILE_SUFFIX = ".widget.json";

		/**
		 * Read a widget file
		 * @param json The file's contents
		 * @param id The widget's id (the file name without FILE_SUFFIX)
		 * @param error Set to what is wrong if nullopt is returned
		 */
		std::optional<WidgetDef> parse(const nlohmann::json& json, std::string id, std::string& error);

		/** Make `widgets` the pool, replacing what was there; a later widget replaces an earlier one of its id */
		void set(std::vector<WidgetDef> widgets);

		/** The widget with this id, or nullptr */
		const WidgetDef* find(std::string_view id);

		/** Every widget in the pool, by id */
		const std::map<std::string, WidgetDef, std::less<>>& all();

		/** The name of `kind` in a widget file, e.g. "int" */
		std::string_view kindName(SettingKind kind);
	} // namespace WidgetCatalogue
} // namespace UI::Xml

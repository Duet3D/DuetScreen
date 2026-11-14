/*
 * ApiReference.h
 *
 * What the XML UI provides to XML, gathered from the running UI: the names a view can bind to (from every
 * `fields()`), the actions it can raise, the presenters, widgets and components it can use, and the theme's styles,
 * colours, consts and fonts. Nothing here is written down separately, so the reference can't fall behind the code.
 *
 * ApiMarkdown.h renders it as docs/XML_UI_API.md, which a test keeps up to date.
 */

#pragma once

#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include <string>
#include <vector>

namespace UI::Xml::Api
{
	/** A subject or list a view can bind to */
	struct Field
	{
		std::string name;				   // As written in XML, e.g. "machine.status"
		std::string type;				   // "int", "text", "colour", "surface", "list" or "history"
		std::string description;		   // What it holds
		std::vector<Field> rowFields = {}; // For a list, the fields every row has (including `index`); else empty
	};

	/** An action a view can raise with `<action name="presenter.…">` */
	struct Action
	{
		std::string name;		 // As XML writes it, e.g. "presenter.fans.set_speed" or "actions.heater.toggle"
		std::string description; // What it does and what it takes
		std::string file = {};	 // For a global action, the source file that adds it, e.g. "src/…/HeaterActions.cpp"
	};

	/** A data source: printer data published as global names */
	struct Source
	{
		std::string name;		   // The source's name, e.g. "fans"
		std::vector<Field> fields; // What it publishes
	};

	/** A converter for object model names, written after '|' (see Om/Converters.h) */
	struct OmConverter
	{
		std::string name;		 // e.g. "fixed"; empty for the default
		std::string type;		 // What its subject holds: "int" or "text"
		std::string argument;	 // What its argument is; empty if it takes none
		std::string description; // What it gives
	};

	/** A presenter a component can name with `<component presenter="…">` */
	struct Presenter
	{
		std::string name;					 // Registered name
		std::vector<Field> fields;			 // Names it adds to its view's scope
		std::vector<Action> actions;		 // Actions it handles
		std::vector<std::string> components; // Loaded components that name it
	};

	/** A theme style, for `<style name="…"/>` */
	struct Style
	{
		std::string name;		// e.g. "card"
		std::string definition; // What the root theme sets in it, as written there; empty if it sets nothing
	};

	/** A theme const, for `#name` in attribute values */
	struct Const
	{
		std::string name;  // e.g. "radius"
		std::string value; // As the root theme writes it, e.g. "5" or "#border_width"
	};

	/** A prop a component takes, declared in its `<api>`: `<my_dialog title="…"/>` sets `$title` inside it */
	struct Prop
	{
		std::string name;		  // As written
		std::string type;		  // As declared, e.g. "string"
		std::string defaultValue; // Its value when not given; empty for none
	};

	/** One place a component is used by another */
	struct Use
	{
		/** How it is used */
		enum class As
		{
			Element, // As an element: `<fan_card/>`
			Item,	 // As a list's item: `<ds_list item="fan_card">`
			View,	 // As a view: `<ds_view view="fans">`
			Base,	 // As the base of a view: `<view extends="fan_card">`
			Widget,	 // As a widget the user places: its widget file names it (see Layout/WidgetCatalogue.h)
		};

		std::string by;		   // The component that uses it; for a widget, the widget's id
		As as = As::Element;   // How
		std::string presenter; // For a view, the presenter `<ds_view presenter="…">` gives it; else empty
	};

	/** A theme, as its file sets it: what it changes from the theme it extends */
	struct Theme
	{
		std::string name;	   // e.g. "duetscreen"
		std::string extends;   // The theme it extends, or empty
		bool abstract = false; // Only a base for other themes, so not offered to users
		std::string source;	   // File it was loaded from
		std::vector<std::pair<std::string, std::string>> palette; // Its `<palette>` attributes, as written
		std::vector<std::string> colours;						  // Its colour adjustments, as written
		std::vector<std::string> fonts;							  // Its fonts, e.g. "header: 18 px, weight 700, bold"
		std::vector<Const> consts;								  // The consts it sets
	};

	/** A loaded component */
	struct Component
	{
		std::string name;		   // Component name, used as an element or a `<ds_list item="…">`
		std::string presenter;	   // Presenter it names, or empty
		std::string bundle;		   // Id of the bundle it came from
		std::string source;		   // File it was loaded from
		std::string extends{};	   // What its view extends: a widget, or another component
		std::vector<Prop> props{}; // Props it takes, in order
		std::vector<Use> uses{};   // Where other components use it, in name order

		/** Whether it is shown as a view of its own: by a `<ds_view>`, or from C++ as nothing else uses it */
		bool isView() const;
	};

	/** Everything the XML UI provides to XML */
	struct Reference
	{
		std::string apiVersion;					  // UI API version, "major.minor"
		std::vector<OmConverter> omConverters;	  // Converters for object model names, the default first
		std::vector<Field> omRowFields;			  // Fields every row of an object model list has
		std::vector<Source> sources;			  // Global data, by data source
		std::vector<std::string> triggers;		  // What `<action trigger="…">` accepts
		std::vector<Action> globalActions;		  // Actions every view can raise
		std::vector<Presenter> presenters;		  // In name order
		std::vector<WidgetSpec> widgets;		  // DuetScreen widgets and elements, in name order
		std::vector<std::string> lvglElements;	  // The XML engine's own widgets and elements, in name order
		std::vector<Component> components;		  // In name order
		std::vector<WidgetDef> placeable;		  // The widgets a user can place in a layout, in id order
		std::vector<Field> placedFields;		  // Names every placed widget has besides its presenter's (its size)
		std::vector<Layout> layouts;			  // The layout each slot shows by default, in slot order
		std::vector<Theme> themes;				  // Every loaded theme, the root first, then in name order
		std::string rootTheme;					  // The theme that styles, consts and fonts are described from
		std::vector<Style> styles;				  // Theme styles
		std::vector<std::string> colours;		  // Palette colour names, for `#name`
		std::vector<Const> consts;				  // Theme consts
		std::vector<std::string> fonts;			  // Font names
		std::vector<std::string> images;		  // Image names
		std::vector<std::string> translationKeys; // Keys of the current language, sorted; not in the Markdown
	};

	/**
	 * Describe the running XML UI. Requires Xml::init(), with the presenters registered. Each presenter is created
	 * (but not activated) to ask for its fields and actions.
	 */
	Reference build();
} // namespace UI::Xml::Api

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

#include "UI/Xml/Widgets/Widgets.h"
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

	/** An action a view can raise with `<action name="…">` */
	struct Action
	{
		std::string name;		 // e.g. "fans.set_speed"
		std::string description; // What it does and what it takes
	};

	/** A data source: printer data published as global names */
	struct Source
	{
		std::string name;		   // The source's name, e.g. "fans"
		std::vector<Field> fields; // What it publishes
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

	/** A loaded component */
	struct Component
	{
		std::string name;	   // Component name, used as an element or a `<ds_list item="…">`
		std::string presenter; // Presenter it names, or empty
		std::string bundle;	   // Id of the bundle it came from
		std::string source;	   // File it was loaded from
	};

	/** Everything the XML UI provides to XML */
	struct Reference
	{
		std::string apiVersion;			   // UI API version, "major.minor"
		std::vector<Source> sources;	   // Global data, by data source
		std::vector<Action> globalActions; // Actions every view can raise
		std::vector<Presenter> presenters; // In name order
		std::vector<WidgetSpec> widgets;   // DuetScreen widgets and elements, in name order
		std::vector<Component> components; // In name order
		std::string rootTheme;			   // The theme that styles, consts and fonts are described from
		std::vector<Style> styles;		   // Theme styles
		std::vector<std::string> colours;  // Palette colour names, for `#name`
		std::vector<Const> consts;		   // Theme consts
		std::vector<std::string> fonts;	   // Font names
		std::vector<std::string> images;   // Image names
	};

	/**
	 * Describe the running XML UI. Requires Xml::init(), with the presenters registered. Each presenter is created
	 * (but not activated) to ask for its fields and actions.
	 */
	Reference build();
} // namespace UI::Xml::Api

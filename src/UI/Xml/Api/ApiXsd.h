/*
 * ApiXsd.h
 *
 * Renders the UI API reference (see ApiReference.h) as an XML Schema, docs/XML_UI.xsd, so an editor can complete and
 * document component XML: VS Code's XML extension (redhat.vscode-xml) is pointed at it in .devcontainer.
 *
 * The schema declares DuetScreen's own elements (`ds_*`, `<action>`, `<style>`, `<component>`, `<view>`, …) and every
 * component, each with its attributes and their documentation. An attribute that names something (a subject, style,
 * action, …) offers every such name, but accepts any value: which names are valid depends on where a component is
 * used, and a `$prop` is only known there, so checking them is the lint's job (ApiLint.h), not the schema's.
 *
 * The XML engine's own widgets and elements (`lv_label`, `bind_flag_if_eq`, …) are declared too, from its registry,
 * with the attributes its descriptions give them (ApiLvgl.h): their help, the values an enum takes, and the names a
 * subject, style or translation attribute takes. An element also takes what it extends takes - an LVGL widget lv_obj's,
 * a DuetScreen widget its base's, a component what its view extends - by referring to each one's attribute group.
 * Further attributes are still accepted, so nothing the schema doesn't know is reported.
 */

#pragma once

#include "ApiLvgl.h"
#include "ApiReference.h"
#include <string>

namespace UI::Xml::Api
{
	/** `reference` as an XML Schema document, with what `lvgl` describes of the engine's own elements */
	std::string toXsd(const Reference& reference, const LvglApi& lvgl);
} // namespace UI::Xml::Api

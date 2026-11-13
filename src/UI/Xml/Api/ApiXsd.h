/*
 * ApiXsd.h
 *
 * Renders the UI API reference (see ApiReference.h) as an XML Schema, docs/XML_UI.xsd, so an editor can complete and
 * document component XML: VS Code's XML extension (redhat.vscode-xml) is pointed at it in .devcontainer.
 *
 * The schema declares DuetScreen's own elements (`ds_*`, `<action>`, `<style>`, `<component>`, `<view>`, …) and every
 * component, each with its attributes and their documentation. An attribute that names something (a subject, style,
 * action, …) offers every such name, but accepts any value: which names are valid depends on where a component is
 * used, and a `$prop` is only known there, so checking them is the lint's job (ApiLint.h), not the schema's. LVGL's
 * own widgets and bindings (`lv_label`, `bind_flag_if_eq`, …) and the attributes every widget takes from its base are
 * left open, as the XML engine has no list of them to generate from.
 */

#pragma once

#include "ApiReference.h"
#include <string>

namespace UI::Xml::Api
{
	/** `reference` as an XML Schema document */
	std::string toXsd(const Reference& reference);
} // namespace UI::Xml::Api

/*
 * DsBar.h
 *
 * <ds_bar>: DuetScreen's Bar (a progress bar that draws its value as text, inside the filled part when it fits) in
 * XML.
 *
 *     <ds_bar min="0" max="100" label_format="{}%" bind_value="progress"/>
 *
 * `label_format` is an fmt format for the value (default "{}"). Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/Bar/Bar.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsBar : public Bar
	{
	  public:
		/** Create a bar inside `parent` */
		DsBar(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_bar>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to Bar, which only uses it during construction */
		DsBar(const std::string& name, LvObjRef&& parent);

		/** `<ds_bar>` create callback: create the C++ bar, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_bar>` apply callback: lv_obj attributes, then the bar's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

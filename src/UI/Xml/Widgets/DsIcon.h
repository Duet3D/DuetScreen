/*
 * DsIcon.h
 *
 * <ds_icon>: an icon from the icon set chosen in the settings, coloured by the theme. Its attributes are in
 * registerXml().
 */

#pragma once

#include "UI/Components/Icon/Icon.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsIcon : public Icon
	{
	  public:
		/** Create an empty icon inside `parent` */
		DsIcon(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_icon>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to Icon, which only uses it during construction */
		DsIcon(const std::string& name, LvObjRef&& parent);

		/** `<ds_icon>` create callback: create the C++ icon, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_icon>` apply callback: lv_image attributes, then `icon` */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

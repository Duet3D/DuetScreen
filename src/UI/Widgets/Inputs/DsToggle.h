/*
 * DsToggle.h
 *
 * <ds_toggle>: an on/off setting, drawn as the build's toggle (a check box, or a switch if built with
 * DUETSCREEN_UI_SETTINGS_TOGGLE_WIDGET=LvSwitch). Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvCheckbox.h"
#include "UI/Lvgl/LvSwitch.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsToggle : public UI_SETTINGS_TOGGLE_WIDGET
	{
	  public:
		/** Create a toggle inside `parent` */
		DsToggle(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_toggle>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to the toggle, which only uses it during construction */
		DsToggle(const std::string& name, LvObjRef&& parent);

		/** `<ds_toggle>` create callback: create the C++ toggle, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_toggle>` apply callback: lv_obj attributes */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

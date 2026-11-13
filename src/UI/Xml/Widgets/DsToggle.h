/*
 * DsToggle.h
 *
 * <ds_toggle>: an on/off setting, drawn as the build's toggle (a check box, or a switch if built with
 * DUETSCREEN_UI_SETTINGS_TOGGLE_WIDGET=LvSwitch), so every setting looks the same.
 *
 *     <ds_toggle>
 *         <bind_state_if_eq subject="confirmations" state="checked" ref_value="1"/>
 *         <action trigger="value_changed" name="settings.confirmations"/>
 *     </ds_toggle>
 *
 * A `value_changed` action gets 1 when it is turned on and 0 when off (ActionArgs::value()). Takes every lv_obj
 * attribute.
 */

#pragma once

#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvSwitch.h"
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

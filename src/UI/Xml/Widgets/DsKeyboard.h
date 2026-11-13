/*
 * DsKeyboard.h
 *
 * <ds_keyboard>: DuetScreen's on-screen keyboard (LVGL's, with its own key layouts) in XML. A <ds_textarea> brings it
 * up and types into it.
 *
 *     <ds_keyboard name="console_keyboard" mode="text_upper" width="100%" height="40%" hidden="true"/>
 *
 * `mode` is `text` (default), `text_upper` or `number`. Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/LVGL/LvKeyboard.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsKeyboard : public LvKeyboard
	{
	  public:
		/** Create a keyboard inside `parent` */
		DsKeyboard(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_keyboard>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to LvKeyboard, which only uses it during construction */
		DsKeyboard(const std::string& name, LvObjRef&& parent);

		/** `<ds_keyboard>` create callback: create the C++ keyboard, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_keyboard>` apply callback: lv_obj attributes, then `mode` */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

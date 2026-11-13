/*
 * DsPasswordToggle.h
 *
 * <ds_password_toggle>: an eye button that shows or hides what is typed in a password box (a `ds_textarea` with
 * `password_mode="true"`), placed beside it. Its attributes are in registerXml().
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsPasswordToggle : public Button
	{
	  public:
		/** Create the button inside `parent` */
		DsPasswordToggle(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_password_toggle>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_password_toggle>` create callback: create the C++ button, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_password_toggle>` apply callback: lv_obj attributes, then which box it shows */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Checked: show the password; unchecked: hide it */
		static void toggledCb(lv_event_t* e);

		/** The text box called m_textarea nearest the button, or nullptr */
		lv_obj_t* findTextarea() const;

		std::string m_textarea; // Name of the password box it shows
	};
} // namespace UI::Xml

/*
 * DsTextarea.h
 *
 * <ds_textarea>: a text box that brings up the on-screen keyboard (the view called `keyboard`) while it is being
 * edited, and hides it again when the edit is done.
 *
 *     <ds_textarea name="input" one_line="true" bind_text="alert.input" bind_accepted_chars="alert.accepted_chars"
 *                  bind_keyboard_mode="alert.keyboard_mode">
 *         <action trigger="value_changed" name="alert.input"/>
 *     </ds_textarea>
 *
 *   bind_text            a string subject: the box shows its value whenever it changes (edits don't change it; a
 *                        handler reads them with ActionArgs::text())
 *   bind_accepted_chars  a string subject: the characters that may be typed; empty for any
 *   bind_keyboard_mode   a string subject: `number` or `text` (default), the keyboard to show
 *   one_line             a single line of text
 *
 * Raises `focused` when the keyboard comes up and `defocused` / `ready` when it goes, plus `value_changed` on every
 * edit. Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/LVGL/LvTextarea.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsTextarea : public LvTextarea
	{
	  public:
		/** Create an empty text box inside `parent` */
		DsTextarea(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_textarea>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to LvTextarea, which only uses it during construction */
		DsTextarea(const std::string& name, LvObjRef&& parent);

		/** `<ds_textarea>` create callback: create the C++ text box, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_textarea>` apply callback: lv_obj attributes, then the text box's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_text` observer */
		static void textObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_accepted_chars` observer */
		static void acceptedObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_keyboard_mode` observer */
		static void keyboardModeObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Focus and editing events: bring up or put away the keyboard */
		static void keyboardEventCb(lv_event_t* e);

		std::string m_accepted;			 // Characters that may be typed; LVGL keeps a pointer to them
		bool m_numberKeyboard = false;	 // Show the number keyboard rather than the text one
	};
} // namespace UI::Xml

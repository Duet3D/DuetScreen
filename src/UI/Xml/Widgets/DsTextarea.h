/*
 * DsTextarea.h
 *
 * <ds_textarea>: a text box that brings up an on-screen keyboard while it is being edited.
 *
 *     <ds_textarea name="input" one_line="true" bind_text="alert.input" bind_accepted_chars="alert.accepted_chars"
 *                  bind_keyboard_mode="alert.keyboard_mode">
 *         <action trigger="value_changed" name="alert.input"/>
 *     </ds_textarea>
 *
 *   bind_text                 a string subject: the box shows its value whenever it changes to something other than
 *                             what is in the box. Handlers read edits with ActionArgs::text(); a presenter that keeps
 *                             the subject equal to the edits can replace the text at any time.
 *   bind_accepted_chars       a string subject: the characters that may be typed; empty for any
 *   keyboard_mode             `number`, `text` (default) or `text_upper`: the keyboard layout to show
 *   bind_keyboard_mode        a string subject, as keyboard_mode
 *   keyboard                  the keyboard to use: the nearest lv_keyboard of that name around the text box (e.g. one
 *                             in the same view), else the view of that name (default `keyboard`, the shared one);
 *                             `none` for a box edited another way, such as on a number pad when it is tapped
 *   hide_keyboard_on_ready    put the keyboard away when its OK key is pressed (default true)
 *   hide_keyboard_on_defocus  put the keyboard away when the box loses focus (default true)
 *   one_line                  a single line of text
 *   password_mode             show dots for the characters, e.g. for a password
 *   placeholder               shown while the box is empty; or placeholder_tag, translated
 *
 * The keyboard's own close key always puts it away. Raises `focused`, `defocused`, `ready` and `cancel` as the edit
 * starts and ends, and `value_changed` on every edit. Takes every lv_obj attribute.
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

		/** The keyboard to use, or nullptr if there is none */
		lv_obj_t* findKeyboard() const;

		/** Show `keyboard` for this box, or put it away */
		void showKeyboard(lv_obj_t* keyboard, bool show);

		std::string m_accepted;							  // Characters that may be typed; LVGL keeps a pointer to them
		lv_keyboard_mode_t m_mode = LV_KEYBOARD_MODE_TEXT_LOWER; // Keyboard layout to show
		std::string m_keyboard = "keyboard";			  // Name of the keyboard to use
		bool m_hideOnReady = true;						  // Put the keyboard away on its OK key
		bool m_hideOnDefocus = true;					  // Put the keyboard away when focus leaves
	};
} // namespace UI::Xml

/*
 * DsTextarea.h
 *
 * <ds_textarea>: a text box that brings up an on-screen keyboard while it is being edited. Its attributes are in
 * registerXml().
 */

#pragma once

#include "UI/Lvgl/LvTextarea.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsTextarea : public LvTextarea
	{
	  public:
		/** Create an empty text box inside `parent` */
		DsTextarea(const std::string& name, lv_obj_t* parent);

		/** Stop watching a press that left the box */
		~DsTextarea() override;

		/** Register `<ds_textarea>` with the XML engine */
		static void registerXml();

	  private:
		/**
		 * The user pressed outside the box while editing: watch the presses from now on, and end the edit on the first
		 * that is a tap outside it (see leftReleasedCb). LVGL tells the box only of the first press elsewhere, so a scroll
		 * that keeps the edit going is followed by presses it would not hear of. Without a press to watch, end it now.
		 */
		void leftByPress();

		/**
		 * A press watched since the box was left was let go. One that scrolled (the dialog or page the box is in), or
		 * that was on something that takes no focus (the keyboard's keys), keeps the edit; one back on the box keeps it
		 * and stops the watch; any other ends it.
		 */
		static void leftReleasedCb(lv_event_t* e);

		/** Stop watching presses */
		void stopWatchingLeave();

		/** Pass a temporary C++ wrapper for `parent` to LvTextarea, which only uses it during construction */
		DsTextarea(const std::string& name, LvObjRef&& parent);

		/** `<ds_textarea>` create callback: create the C++ text box, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_textarea>` apply callback: lv_obj attributes, then the text box's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Stop `value_changed` reaching the actions while the text is written by `bind_text` */
		static void quietChangeCb(lv_event_t* e);

		/** `bind_text` observer */
		static void textObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_accepted_chars` observer */
		static void acceptedObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_keyboard_mode` observer */
		static void keyboardModeObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/**
		 * Focus and editing events: bring up or put away the keyboard. Tapping outside the box (LEAVE) ends the edit
		 * like losing focus, and tapping the box while the keyboard is away starts it again.
		 */
		static void keyboardEventCb(lv_event_t* e);

		/** The keyboard to use, or nullptr if there is none */
		lv_obj_t* findKeyboard() const;

		/** Show `keyboard` for this box, or put it away */
		void showKeyboard(lv_obj_t* keyboard, bool show);

		std::string m_accepted; // Characters that may be typed; LVGL keeps a pointer to them
		lv_keyboard_mode_t m_mode = LV_KEYBOARD_MODE_TEXT_LOWER; // Keyboard layout to show
		std::string m_keyboard = "keyboard";					 // Name of the keyboard to use
		bool m_hideOnReady = true;								 // Put the keyboard away on its OK key
		bool m_hideOnDefocus = true;							 // Put the keyboard away when focus leaves
		bool m_settingQuietly = false;							 // True while `bind_text` writes the text
		lv_indev_t* m_leavingIndev = nullptr;					 // The input device watched since the box was left, if any
	};
} // namespace UI::Xml

/*
 * DsButton.h
 *
 * <ds_button>: DuetScreen's Button (text and/or icon) in XML.
 *
 *     <ds_button text="…" translation_tag="…" bind_text="subject" icon="file.png" checkable="true"
 *                long_mode="clip" flash_on="subject"/>
 *
 * `translation_tag` sets the text from the translation table and follows language changes; `bind_text` follows a
 * string or integer subject, formatted with `bind_text-fmt` if given (e.g. "%d°C"). `long_mode` is the label's (wrap, dots, scroll, scroll_circular, clip). `flash_on` briefly
 * flashes the button, in its `user_1` state, whenever a subject changes, e.g. on every response from the printer:
 *
 *     <ds_button flash_on="responses.count">
 *         <style name="console_btn_anim" selector="user_1"/>
 *     </ds_button>
 *
 * Every lv_obj attribute (width, flex_grow, style_*, hidden, …) works too.
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsButton : public Button
	{
	  public:
		/** Create a button inside `parent` showing `text` */
		DsButton(const std::string& name, lv_obj_t* parent, std::string_view text);

		/** Register `<ds_button>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_button>` create callback: create the C++ button, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_button>` apply callback: apply lv_obj attributes, then the button's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Show the translation of the tag after a language change */
		static void languageChangedCb(lv_event_t* e);

		/** `bind_text` observer: show the subject's value */
		static void textObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `flash_on` observer: flash on every change after the first notification */
		static void flashObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Fade a recolour in and out once, in the `user_1` state */
		void flash();

		std::string m_translationTag; // `translation_tag`, or empty if the text isn't translated
		std::string m_textFormat;	  // `bind_text-fmt`, or empty for the plain value
		bool m_flashArmed = false;	  // False until `flash_on` has reported its starting value
	};
} // namespace UI::Xml

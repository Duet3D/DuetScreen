/*
 * DsButton.h
 *
 * <ds_button>: DuetScreen's Button (text and/or icon) in XML.
 *
 *     <ds_button text="…" translation_tag="…" bind_text="subject" icon="file.png" checkable="true"/>
 *
 * `translation_tag` sets the text from the translation table and follows language changes; `bind_text` follows a
 * string subject. Every lv_obj attribute (width, flex_grow, style_*, hidden, …) works too.
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

		std::string m_translationTag; // `translation_tag`, or empty if the text isn't translated
	};
} // namespace UI::Xml

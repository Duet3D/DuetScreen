/*
 * DsChoice.h
 *
 * <ds_choice>: a button showing the option chosen, with a chevron, that opens the shared choice dialog
 * (Prompts::askChoice) when tapped. The option picked reaches a `value_confirmed` action as its index
 * (ActionArgs::value()). Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvLabel.h"
#include "UI/Lvgl/LvObj.h"
#include "helix_xml.h"
#include <string>
#include <vector>

namespace UI::Xml
{
	class DsChoice : public LvObj
	{
	  public:
		/** Create a choice inside `parent`, with no options yet */
		DsChoice(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_choice>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_choice>` create callback: create the C++ choice, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_choice>` apply callback: lv_obj attributes, then the choice's own and its bindings */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_options` observer: take the subject's options */
		static void optionsObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_selected` observer: show the subject's option chosen */
		static void selectedObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_text` observer: show the subject's text instead of the option chosen */
		static void textObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Tapped: ask for one of the options */
		static void clickedCb(lv_event_t* e);

		/** Show the text to show: `bind_text`'s if bound, else the option chosen, else nothing */
		void showChosen();

		/** Open the choice dialog, marking the option chosen; the one picked is confirmed with `value_confirmed` */
		void ask();

		/** The user picked option `index`: show it, unless a subject says what is chosen, and confirm it */
		void pick(size_t index);

		LvLabel m_label;						 // The option chosen, or `bind_text`'s text
		LvLabel m_chevron;						 // Says that tapping opens a choice
		std::vector<std::string> m_options;		 // The options, in order
		int32_t m_chosen = -1;					 // Index of the option shown chosen, or -1 for none
		bool m_chosenBound = false;				 // True if `bind_selected` says which is chosen
		bool m_textBound = false;				 // True if `bind_text` says what is shown
		std::string m_text;						 // `bind_text`'s text
		std::string m_title;					 // `title`: the dialog's title, if there is no `title_tag`
		std::string m_titleTag;					 // `title_tag`: the dialog title's translation key, or empty
	};
} // namespace UI::Xml

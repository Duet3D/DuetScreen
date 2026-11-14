/*
 * DsSetting.h
 *
 * <ds_setting>: one setting, a row: its title, and under it a hint if it has one, then whatever changes it (its
 * children, at the end of the row). Tapping the row anywhere works its control, as tapping the control would, if that
 * is a switch, a button or a choice. Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsSetting : public LvContainer
	{
	  public:
		/** Create a setting, with no title or control yet, inside `parent` */
		DsSetting(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_setting>` with the XML engine */
		static void registerXml();

		/**
		 * The control that tapping the row works: the first child shown after the title that is a switch (or anything
		 * else checkable), a button or a choice; nullptr if there is none
		 */
		lv_obj_t* control() const;

	  private:
		/** `<ds_setting>` create callback: create the C++ setting, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_setting>` apply callback: lv_obj attributes, then the title and hint */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_title` observer: show the subject's text as the title */
		static void titleObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Translate the title and hint again after a language change */
		static void languageChangedCb(lv_event_t* e);

		/** Pressed: look pressed only if tapping works a control */
		static void pressedCb(lv_event_t* e);

		/** Tapped (not on its control): work the control, as tapping it would */
		static void clickedCb(lv_event_t* e);

		/** Show `hint` under the title, or no hint if it is empty */
		void setHint(std::string_view hint);

		LvContainer m_text{"titles", getRoot()}; // The title over the hint, taking the room the control leaves
		LvLabel m_title{"title", m_text};	   // What the setting is
		LvLabel m_hint{"hint", m_text};		   // More about it, if it needs saying
		std::string m_titleTag;				   // `title_tag`, or empty if the title isn't translated
		std::string m_hintTag;				   // `hint_tag`, or empty if the hint isn't translated
	};
} // namespace UI::Xml

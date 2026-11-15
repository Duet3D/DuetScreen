/*
 * DsSettingGroup.h
 *
 * <ds_setting_group>: a group of settings, its title (in capitals) over a box of the rows written inside it
 * (<ds_setting>, or anything that should sit in the box, such as a <ds_list> of rows). The rows are written straight
 * inside the group; each child the XML builds is moved into the box as it is created, so hiding the group (`hidden`,
 * or a `bind_flag_if_eq` on it) hides its title too. Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "helix_xml.h"
#include <string>
#include <string_view>

namespace UI::Xml
{
	class DsSettingGroup : public LvContainer
	{
	  public:
		/** Create a group, untitled and empty, inside `parent` */
		DsSettingGroup(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_setting_group>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_setting_group>` create callback: create the C++ group, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_setting_group>` apply callback: lv_obj attributes, then the title, the note and `danger` */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A child was created: if it is the group's own (not its title, box or what is in them), move it into the box */
		static void childCreatedCb(lv_event_t* e);

		/** Translate the title again after a language change */
		static void languageChangedCb(lv_event_t* e);

		/** Show `title`, in capitals, over the box; no title if it is empty */
		void setTitle(std::string_view title);

		/** Show `note` at the end of the title's line, e.g. who sets the settings; none if it is empty */
		void setNote(std::string_view note);

		/** Show the title's line only while it has a title or a note */
		void showHeader();

		LvContainer m_header{"header", getRoot()}; // The title and the note, on a line over the box
		LvLabel m_title{"title", m_header};		   // What the settings in the box are for
		LvLabel m_note{"note", m_header};		   // More about them, muted, at the end of the line
		LvContainer m_box{"rows", getRoot()};	   // The rows
		std::string m_titleTag;					   // `title_tag`, or empty if the title isn't translated
		std::string m_noteTag;					   // `note_tag`, or empty if the note isn't translated
	};
} // namespace UI::Xml

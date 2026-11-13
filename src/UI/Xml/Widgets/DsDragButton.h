/*
 * DsDragButton.h
 *
 * <ds_drag_button>: a button that must be dragged, not just tapped, to act, such as the E-stop. Its attributes are in
 * registerXml().
 */

#pragma once

#include "UI/Components/Button/DraggableButton.h"
#include "UI/Components/LVGL/LvArclabel.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsDragButton : public DraggableButton
	{
	  public:
		/** Create a button inside `parent` showing `text` */
		DsDragButton(const std::string& name, lv_obj_t* parent, std::string_view text);

		/** Register `<ds_drag_button>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_drag_button>` create callback: create the C++ button, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_drag_button>` apply callback: lv_obj attributes, then the button's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Hide the hint while the button is held, and show it again when let go */
		static void pressCb(lv_event_t* e);

		LvArclabel m_hint{"hint", getRoot()}; // Curved around the edge: how to use it
		bool m_hasHint = false;				  // Whether there is a hint to show
	};
} // namespace UI::Xml

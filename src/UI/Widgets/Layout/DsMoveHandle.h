/*
 * DsMoveHandle.h
 *
 * <ds_move_handle>: a grip that moves the widget it is in (its parent) around that widget's parent when dragged, e.g.
 * the customise toolbar's. Its attributes are in registerXml(); what it looks like is up to the XML inside it.
 *
 * The first drag takes the moved widget out of its parent's layout (it floats), and it is kept inside its parent.
 */

#pragma once

#include "helix_xml.h"
#include <optional>

namespace UI::Xml
{
	class DsMoveHandle
	{
	  public:
		/** Move the parent of `handle` when `handle` is dragged */
		explicit DsMoveHandle(lv_obj_t* handle);

		/** Register `<ds_move_handle>` with the XML engine */
		static void registerXml();

	  private:
		/** Where a drag started */
		struct Start
		{
			lv_point_t touch{};	 // The touch, on the screen
			lv_point_t moved{};	 // The moved widget's position in its parent
		};

		/** `<ds_move_handle>` create callback: a plain container, to hold the grip's looks */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_move_handle>` apply callback: lv_obj attributes, then start following drags */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A touch on the handle: start, follow or end a drag */
		static void eventCb(lv_event_t* e);

		/** The touch moved to `touch`: move the widget as far, kept inside its parent */
		void dragTo(const lv_point_t& touch) const;

		lv_obj_t* m_handle;			// The ds_move_handle widget
		std::optional<Start> m_start; // While dragging, where it started
	};
} // namespace UI::Xml

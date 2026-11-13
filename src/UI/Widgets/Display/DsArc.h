/*
 * DsArc.h
 *
 * <ds_arc>: an arc that shows a value, such as a gauge's: LVGL's lv_arc, read-only, with a value and range that follow
 * subjects. Its attributes are in registerXml(); it also takes every lv_arc attribute (angles, rotation, mode)
 * except `bind_value`, which in lv_arc writes back to the subject when the arc is dragged.
 *
 * It can't be touched and has no knob. What it looks like is its styles: the track is the main part's arc, the value
 * the indicator's.
 */

#pragma once

#include "helix_xml.h"

namespace UI::Xml
{
	class DsArc
	{
	  public:
		/** Register `<ds_arc>` with the XML engine */
		static void registerXml();

	  private:
		/** `<ds_arc>` create callback: an lv_arc that can't be touched, without its knob */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_arc>` apply callback: lv_arc attributes, then the arc's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_value` observer: show the subject's value */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_min` observer: make the subject's value the bottom of the range */
		static void minObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_max` observer: make the subject's value the top of the range */
		static void maxObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Show `value`'s value again after the range changed, which clamps it; nothing if `value` is nullptr */
		static void showValue(lv_obj_t* arc, lv_subject_t* value);
	};
} // namespace UI::Xml

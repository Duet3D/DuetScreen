/*
 * DsSlider.h
 *
 * <ds_slider>: DuetScreen's Slider (label, value, -/+ buttons, optional number pad) in XML. `numberpad="true"` uses
 * the view registered as `numberpad` (see Navigation.h).
 *
 *     <ds_slider min="0" max="100" increment="1" decimals="0" label="…" label_tag="…"
 *                bind_label="subject" bind_value="subject" numberpad="true">
 *         <action trigger="value_confirmed" name="…"/>
 *     </ds_slider>
 *
 * `label_shown="false"` hides the label, which still heads the number pad. `out_of_range` ("upper", "lower" or "both")
 * lets a typed value go past the slider's end, e.g. a timeout longer than the slider reaches.
 *
 * The slider raises `value_confirmed` when the user finishes an edit (releases it, presses -/+ or types a value), or
 * with `send="change"` on every movement, for a setting seen as it changes, such as brightness.
 * Values written by `bind_value` are not edits and raise nothing, so a view can show live data and send changes
 * without echoing the data back to the printer.
 */

#pragma once

#include "UI/Components/Input/Slider.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsSlider : public Slider
	{
	  public:
		/** Create a slider inside `parent` */
		DsSlider(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_slider>` with the XML engine */
		static void registerXml();

		/** Set the value without raising `value_confirmed` */
		void setValueQuietly(float value);

		/** Set the range without raising `value_confirmed` */
		void setRangeQuietly(float min, float max);

	  private:
		/** `<ds_slider>` create callback: create the C++ slider, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_slider>` apply callback: apply lv_obj attributes, then the slider's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_label` observer */
		static void labelObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_value` observer; ignores updates while the user is editing */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		bool m_settingQuietly = false; // True while a value is written by code rather than edited
	};
} // namespace UI::Xml

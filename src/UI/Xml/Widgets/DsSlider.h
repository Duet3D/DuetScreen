/*
 * DsSlider.h
 *
 * <ds_slider>: DuetScreen's Slider (label, value, -/+ buttons, optional number pad) in XML. Its attributes are in
 * registerXml().
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

	  protected:
		/** `<ds_slider>` apply callback: apply lv_obj attributes, then the slider's own. Also for widgets extending it.
		 */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

	  private:
		/** `<ds_slider>` create callback: create the C++ slider, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_label` observer */
		static void labelObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_value` observer; ignores updates while the user is editing */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		bool m_settingQuietly = false; // True while a value is written by code rather than edited
		float m_valueScale = 1.0f;	   // `bind_value`'s subject holds the value times this
	};
} // namespace UI::Xml

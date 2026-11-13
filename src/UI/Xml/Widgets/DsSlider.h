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
		/** What tapping the value does (`numberpad`) */
		enum class NumberPad
		{
			NONE,	// Type it in place
			SHARED, // Type it on the shared number pad
			OWN,	// Raise `value_tapped`, for the view's own number pad
		};

		/** Create a slider inside `parent` */
		DsSlider(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_slider>` with the XML engine */
		static void registerXml();

		/** Set the value without raising `value_confirmed` */
		void setValueQuietly(float value);

		/** Set the range without raising `value_confirmed` */
		void setRangeQuietly(float min, float max);

	  protected:
		/** With `numberpad="true"`, ask for the value on the shared number pad */
		bool askForValue() override;

		/** `<ds_slider>` apply callback: apply lv_obj attributes, then the slider's own. Also for widgets extending it.
		 */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

	  private:
		/** `<ds_slider>` create callback: create the C++ slider, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_label` observer */
		static void labelObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_min` observer */
		static void minObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_max` observer */
		static void maxObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_value` observer; ignores updates while the user is editing */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		bool m_settingQuietly = false;	// True while a value is written by code rather than edited
		NumberPad m_numberPad = NumberPad::NONE; // What tapping the value does
		float m_valueScale = 1.0f;	   // `bind_value`'s subject holds the value times this
	};
} // namespace UI::Xml

/*
 * DsHueSlider.h
 *
 * <ds_hue_slider>: a <ds_slider> for a hue, 0 to 360, whose track shows every hue at a lightness and chroma, so the
 * slider is a colour picker, e.g. for a theme's primary colour.
 *
 *     <ds_hue_slider bind_value="primary_hue" bind_lightness="primary.lightness" bind_chroma="primary.chroma"/>
 *
 *   bind_lightness, bind_chroma   integer subjects: the OKLCH lightness and chroma the hues are shown at, in
 *                                 thousandths (a lightness of 0.65 is 650)
 *
 * Takes every <ds_slider> attribute; min and max default to 0 and 360.
 */

#pragma once

#include "DsSlider.h"
#include "UI/Components/Canvas/Canvas.h"
#include <memory>

namespace UI::Xml
{
	class DsHueSlider : public DsSlider
	{
	  public:
		/** Create a hue slider inside `parent` */
		DsHueSlider(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_hue_slider>` with the XML engine */
		static void registerXml();

		/** Show the hues at `lightness` and `chroma` */
		void setColour(float lightness, float chroma);

	  private:
		/** `<ds_hue_slider>` create callback: create the C++ slider, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_hue_slider>` apply callback: a slider's attributes, then the colour's */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_lightness` observer */
		static void lightnessObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_chroma` observer */
		static void chromaObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Keep the hues under the slider's bar, the same size and shape */
		static void barEventCb(lv_event_t* e);

		/** Draw every hue */
		void render();

		std::unique_ptr<Canvas> m_hues; // The hues, under the bar
		float m_lightness = 0.5f;		// OKLCH lightness they are shown at
		float m_chroma = 0.1f;			// OKLCH chroma they are shown at
	};
} // namespace UI::Xml

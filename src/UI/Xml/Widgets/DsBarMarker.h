/*
 * DsBarMarker.h
 *
 * <ds_bar_marker>: a value marked on a bar, such as a heater's target temperature above its current-temperature bar.
 * It is a button showing the value, placed along the bar where the value falls and pointing at that place with a
 * triangle. Drag it along the bar to choose a new value, or tap it.
 *
 *     <ds_bar name="current" bind_value="current_value" bind_min="scale_min" bind_max="scale_max" align="center"/>
 *     <ds_bar_marker name="target" bar="current" bind_value="active" bind_min="scale_min" bind_max="scale_max"
 *                    label_format="{} °C" align="left_mid" y="-30">
 *         <style name="temperature_bar_indic" selector="indicator"/>
 *         <action trigger="clicked" name="heaters.set_active"/>
 *         <action trigger="value_confirmed" name="heaters.set_active"/>
 *     </ds_bar_marker>
 *
 *   bar                the sibling bar it marks, by name; written before the marker. The bar spans its parent's width
 *                      (e.g. width="100%"), as the marker is placed by percentages of that width
 *   min, max           the scale the value is on (default 0 to 100), or bind_min and bind_max to follow subjects
 *   bind_value         an integer subject: the value marked
 *   label_format       fmt format for the value (default "{}")
 *
 * Its x position is set from the value; set its y (above or below the bar) with `align` and `y`. The triangle is the
 * indicator part's background colour.
 *
 * A drag raises `value_confirmed` with the chosen value when released; until the subject changes, the marker shows
 * the value dragged to. A tap raises `clicked`; a drag doesn't. Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsBarMarker : public Button
	{
	  public:
		/** Create a marker inside `parent` */
		DsBarMarker(const std::string& name, lv_obj_t* parent);

		/** Stop following the bar */
		~DsBarMarker() override;

		/** Register `<ds_bar_marker>` with the XML engine */
		static void registerXml();

		/** Mark `value`, unless it is being dragged */
		void setValue(int32_t value);

		/** Put the value on the scale `min` to `max` */
		void setRange(int32_t min, int32_t max);

		/** Mark positions along `bar`, which must be a sibling */
		void setBar(lv_obj_t* bar);

		/** Show the value with fmt format `format` */
		void setLabelFormat(std::string format);

	  private:
		/** `<ds_bar_marker>` create callback: create the C++ marker, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_bar_marker>` apply callback: lv_obj attributes, then the marker's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_value` observer */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_min` observer */
		static void minObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_max` observer */
		static void maxObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** The marker's own events: dragging, the triangle, and following its size */
		static void eventCb(lv_event_t* e);

		/** The bar's events: follow its size, and forget it when it is deleted */
		static void barEventCb(lv_event_t* e);

		/** Start a possible drag */
		void pressed();

		/** Follow a drag once it has gone far enough to be one */
		void pressing();

		/** Finish a drag, raising `value_confirmed` */
		void released();

		/** Draw the triangle from the bar to the marker */
		void drawTriangle(lv_event_t* e);

		/** How far outside the marker the triangle reaches */
		int32_t triangleReach() const;

		/** Show `value` as the text and the position */
		void show(int32_t value);

		/** Move along the bar to point at the value shown */
		void place();

		/** Where `value` falls on the scale, 0-100 */
		int32_t percentOf(int32_t value) const;

		/** The value under screen x position `x` on the bar, allowing for where the marker was taken hold of */
		int32_t valueAt(int32_t x) const;

		/** The point the active input device is at, in the marker's untransformed coordinates */
		lv_point_t pointerPoint() const;

		lv_obj_t* m_bar = nullptr;	   // The bar marked, or nullptr if there is none (yet, or any more)
		std::string m_format{"{}"};	   // Text format for the value
		int32_t m_min = 0;			   // Bottom of the scale
		int32_t m_max = 100;		   // Top of the scale
		int32_t m_value = 0;		   // The value from the subject
		int32_t m_shown = 0;		   // The value shown: m_value, or the one dragged to
		lv_point_t m_pressedAt{};	   // Where it was pressed
		int32_t m_holdOffset = 0;	   // How far right of the marked position it was taken hold of, px
		bool m_dragging = false;	   // Being dragged
		bool m_dragged = false;		   // The last press was a drag, so the click that follows isn't a tap
	};
} // namespace UI::Xml

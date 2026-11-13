/*
 * DsPlot.h
 *
 * <ds_plot>: an X/Y area with scales and a grid, such as the bed with its objects, whose content is widgets placed by
 * X/Y position. Layers (<ds_plot_layer>) written inside it are kept over its drawing area, and rectangles
 * (<ds_plot_rect>) inside a layer are kept at their X/Y bounds:
 *
 *     <ds_plot title_tag="object_cancel.current_object_prefix" title_style="text_emphasis" bind_detail="current"
 *              bind_x_min="x_min" bind_x_max="x_max" bind_y_min="y_min" bind_y_max="y_max">
 *         <ds_plot_layer name="layer">
 *             <ds_list list="objects" item="object_rect" width="100%" height="100%"/>
 *         </ds_plot_layer>
 *     </ds_plot>
 *
 *   title, title_tag, bind_title   the title above the area
 *   title_style                    a theme style for the title, e.g. text_emphasis
 *   bind_detail                    a string subject shown after the title
 *   x_min, x_max, y_min, y_max     the area's span (default 0 to 100), or bind_x_min … to follow integer subjects
 *   resolution                     pixels the grid is drawn with along each side (default 200), scaled to fit
 *
 * Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/Canvas/Canvas.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "helix_xml.h"
#include <memory>
#include <string>

namespace UI::Xml
{
	class DsPlot : public Canvas
	{
	  public:
		/** Create a plot inside `parent` */
		DsPlot(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_plot>` with the XML engine */
		static void registerXml();

		/**
		 * Where (x, y) is drawn, relative to the top left of the drawing area's content (inside its border)
		 * @return false if it is outside the span
		 */
		bool toWidget(float x, float y, lv_point_t& point) const;

		/** The drawing area's widget */
		lv_obj_t* area() { return getCanvas().getRootPtr(); }

		/** The plot `obj` is in, or nullptr */
		static DsPlot* of(lv_obj_t* obj);

	  private:
		/** Pass a temporary C++ wrapper for `parent` to Canvas, which only uses it during construction */
		DsPlot(const std::string& name, LvObjRef&& parent);

		/** `<ds_plot>` create callback: create the C++ plot, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_plot>` apply callback: lv_obj attributes, then the plot's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Observer of one of the span's subjects; its user data is the value it sets */
		static void rangeObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_title` observer */
		static void titleObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_detail` observer */
		static void detailObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Redraw the grid when the theme changes, and keep the layers over the drawing area */
		static void eventCb(lv_event_t* e);

		/** Use the span in m_range, redrawing the grid and moving the content */
		void applyRange();

		/** Put every layer over the drawing area and every rectangle at its bounds */
		void placeContent();

		/** The label after the title, made when first needed */
		LvLabel& detail();

		/** The span, as set */
		struct Span
		{
			int32_t xMin = 0;	// Lowest X
			int32_t xMax = 100; // Highest X
			int32_t yMin = 0;	// Lowest Y
			int32_t yMax = 100; // Highest Y
		} m_range;

		std::unique_ptr<LvLabel> m_detail; // Shown after the title, or nullptr if there is no bind_detail
	};
} // namespace UI::Xml

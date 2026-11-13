/*
 * DsHeatmap.h
 *
 * <ds_heatmap>: a colour map of a Surface (see Binding/Surface.h), such as a bed's height map, with X and Y scales,
 * a grid, a title, and the colour scale beside it.
 *
 *     <ds_heatmap bind_surface="surface" bind_title="title" resolution="200"/>
 *
 *   bind_surface   the Surface to show; redrawn whenever it changes
 *   bind_title     a string subject shown above the map
 *   resolution     pixels the map is drawn with along each side (default 200), scaled to fit
 *
 * Takes every lv_obj attribute.
 */

#pragma once

#include "UI/Components/Heatmap/Heatmap.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class Surface;

	class DsHeatmap : public Heatmap
	{
	  public:
		/** Create a heat map inside `parent` */
		DsHeatmap(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_heatmap>` with the XML engine */
		static void registerXml();

		/** Draw `surface` */
		void show(const Surface& surface);

	  private:
		/** Pass a temporary C++ wrapper for `parent` to Heatmap, which only uses it during construction */
		DsHeatmap(const std::string& name, LvObjRef&& parent);

		/** `<ds_heatmap>` create callback: create the C++ heat map, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_heatmap>` apply callback: lv_obj attributes, then the heat map's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_surface` observer: draw the surface */
		static void surfaceObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_title` observer: show the title */
		static void titleObserverCb(lv_observer_t* observer, lv_subject_t* subject);
	};
} // namespace UI::Xml

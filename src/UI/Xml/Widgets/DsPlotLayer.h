/*
 * DsPlotLayer.h
 *
 * <ds_plot_layer>: a layer written inside a <ds_plot>, kept exactly over its drawing area. See DsPlot.h. Its attributes
 * are in registerXml().
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsPlotLayer : public LvObj
	{
	  public:
		/** Create a layer inside `parent`, a plot */
		DsPlotLayer(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_plot_layer>` with the XML engine */
		static void registerXml();

		/** Cover `area`, a sibling */
		void cover(lv_obj_t* area);

	  private:
		/** `<ds_plot_layer>` create callback: create the C++ layer, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_plot_layer>` apply callback: lv_obj attributes */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

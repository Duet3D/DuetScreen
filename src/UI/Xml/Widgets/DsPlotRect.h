/*
 * DsPlotRect.h
 *
 * <ds_plot_rect>: a rectangle kept at its X/Y bounds over a <ds_plot>. See DsPlot.h. Its attributes are in
 * registerXml().
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsPlotRect : public LvObj
	{
	  public:
		/** Create a rectangle inside `parent` */
		DsPlotRect(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_plot_rect>` with the XML engine */
		static void registerXml();

		/** Move to the bounds, over the plot it is in; hide if they are outside its span */
		void place();

	  private:
		/** `<ds_plot_rect>` create callback: create the C++ rectangle, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_plot_rect>` apply callback: lv_obj attributes, then the bounds */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Observer of one corner's subject; its user data is the coordinate it sets */
		static void cornerObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		int32_t m_x1 = 0; // A corner's X
		int32_t m_y1 = 0; // A corner's Y
		int32_t m_x2 = 0; // The opposite corner's X
		int32_t m_y2 = 0; // The opposite corner's Y
	};
} // namespace UI::Xml

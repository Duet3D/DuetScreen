/*
 * Widgets.h
 *
 * The DuetScreen widgets XML can use in addition to LVGL's `lv_*` widgets:
 *
 *   <ds_bar>          see DsBar.h
 *   <ds_bar_marker>   see DsBarMarker.h
 *   <ds_button>       see DsButton.h
 *   <ds_dropdown>     see DsDropdown.h
 *   <ds_graph>        see DsGraph.h
 *   <ds_heatmap>      see DsHeatmap.h
 *   <ds_icon>         see DsIcon.h
 *   <ds_keyboard>     see DsKeyboard.h
 *   <ds_list>         see DsList.h
 *   <ds_modal>        see DsModal.h
 *   <ds_plot>         see DsPlot.h
 *   <ds_plot_layer>   see DsPlotLayer.h
 *   <ds_plot_rect>    see DsPlotRect.h
 *   <ds_slider>       see DsSlider.h
 *   <ds_tabs>         see DsTabs.h
 *   <ds_textarea>     see DsTextarea.h
 *   <ds_thumbnail>    see DsThumbnail.h
 *   <ds_view>         see DsView.h
 */

#pragma once

namespace UI::Xml::Widgets
{
	/** Register every DuetScreen widget with the XML engine. Call after lv_xml_init(). */
	void registerAll();
} // namespace UI::Xml::Widgets

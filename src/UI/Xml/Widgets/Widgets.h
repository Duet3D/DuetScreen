/*
 * Widgets.h
 *
 * The DuetScreen widgets XML can use in addition to LVGL's `lv_*` widgets:
 *
 *   <ds_button>       see DsButton.h
 *   <ds_slider>       see DsSlider.h
 *   <ds_list>         see DsList.h
 *   <ds_view>         see DsView.h
 */

#pragma once

namespace UI::Xml::Widgets
{
	/** Register every DuetScreen widget with the XML engine. Call after lv_xml_init(). */
	void registerAll();
} // namespace UI::Xml::Widgets

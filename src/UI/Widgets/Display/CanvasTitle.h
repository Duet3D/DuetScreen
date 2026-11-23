/*
 * CanvasTitle.h
 *
 * The title attributes of the widgets drawn on a Canvas (<ds_plot>, <ds_heatmap>), written the same way on each: a
 * title, fixed, translated or followed, in a theme style, and a detail after it, e.g. "Printing now: gear".
 */

#pragma once

#include "UI/Widgets/Display/Canvas.h"
#include "UI/Widgets/Widgets.h"
#include "helix_xml.h"
#include <string_view>
#include <vector>

namespace UI::Xml::CanvasTitle
{
	/** The attributes, for a widget's spec: title, title_tag, bind_title, title_style, bind_detail */
	std::vector<WidgetAttribute> attributes();

	/** Apply the attributes in `attrs` to `canvas`, the widget `widget` (e.g. "ds_plot", for warnings) built by `state` */
	void apply(lv_xml_parser_state_t* state, const char** attrs, Canvas& canvas, std::string_view widget);
} // namespace UI::Xml::CanvasTitle

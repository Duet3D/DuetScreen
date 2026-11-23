/*
 * DsCross.cpp
 */

#include "DsCross.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsCross::DsCross(const std::string& name, lv_obj_t* parent)
		: LvObj(lv_obj_create, name, parent)
	{
		lv_obj_t* root = getRootPtr();
		// Taps go to what it crosses out
		lv_obj_remove_flag(root, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(root, drawCb, LV_EVENT_DRAW_MAIN_END, nullptr);
	}

	void DsCross::registerXml()
	{
		Widgets::add({.name = "ds_cross",
					  .extends = "lv_obj",
					  .description = "An X drawn corner to corner across its content area (inside its padding), in "
									 "its line style (`line_color`, `line_width`, `line_opa`, `line_rounded`), to "
									 "cross something out, such as a cancelled object on the bed. Taps pass through "
									 "it to its parent.",
					  .example = R"(<lv_obj width="120" height="80">
	<ds_cross width="100%" height="100%" style_line_width="2" style_line_color="0x9E9E9E"/>
</lv_obj>)"},
					 xmlCreate,
					 xmlApply);
	}

	void DsCross::drawCb(lv_event_t* e)
	{
		lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		lv_area_t area;
		lv_obj_get_content_coords(obj, &area);
		if (lv_area_get_width(&area) <= 0 || lv_area_get_height(&area) <= 0)
		{
			return;
		}
		lv_draw_line_dsc_t line;
		lv_draw_line_dsc_init(&line);
		lv_obj_init_draw_line_dsc(obj, LV_PART_MAIN, &line);
		if (line.width <= 0 || line.opa <= LV_OPA_MIN)
		{
			return;
		}
		lv_layer_t* layer = lv_event_get_layer(e);
		line.p1 = {static_cast<lv_value_precise_t>(area.x1), static_cast<lv_value_precise_t>(area.y1)};
		line.p2 = {static_cast<lv_value_precise_t>(area.x2), static_cast<lv_value_precise_t>(area.y2)};
		lv_draw_line(layer, &line);
		line.p1 = {static_cast<lv_value_precise_t>(area.x2), static_cast<lv_value_precise_t>(area.y1)};
		line.p2 = {static_cast<lv_value_precise_t>(area.x1), static_cast<lv_value_precise_t>(area.y2)};
		lv_draw_line(layer, &line);
	}

	void* DsCross::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* cross = new DsCross(attrOr(attrs, "name", "cross"), parentOf(state));
		deleteWith(cross->getRootPtr(), cross);
		return cross->getRootPtr();
	}

	void DsCross::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}
} // namespace UI::Xml

/*
 * DsPlotLayer.cpp
 */

#include "DsPlotLayer.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsPlotLayer::DsPlotLayer(const std::string& name, lv_obj_t* parent)
		: LvObj(lv_obj_create, name, parent)
	{
		lv_obj_t* root = getRootPtr();
		lv_obj_add_flag(root, LV_OBJ_FLAG_FLOATING); // Placed by the plot, not its layout
		lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
	}

	void DsPlotLayer::registerXml()
	{
		lv_xml_register_widget("ds_plot_layer", xmlCreate, xmlApply);
	}

	void DsPlotLayer::cover(lv_obj_t* area)
	{
		lv_obj_t* root = getRootPtr();
		lv_area_t target;
		lv_obj_get_coords(area, &target);
		lv_area_t content;
		lv_obj_get_content_coords(lv_obj_get_parent(root), &content);
		lv_obj_set_pos(root, target.x1 - content.x1, target.y1 - content.y1);
		lv_obj_set_size(root, lv_area_get_width(&target), lv_area_get_height(&target));
	}

	void* DsPlotLayer::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* layer = new DsPlotLayer(attrOr(attrs, "name", "layer"), parentOf(state));
		deleteWith(layer->getRootPtr(), layer);
		return layer->getRootPtr();
	}

	void DsPlotLayer::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}
} // namespace UI::Xml

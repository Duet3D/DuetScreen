/*
 * DsPlotRect.cpp
 */

#include "DsPlotRect.h"
#include "DsPlot.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>
#include <cstdlib>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsPlotRect::DsPlotRect(const std::string& name, lv_obj_t* parent)
		: LvObj(lv_obj_create, name, parent)
	{
		lv_obj_t* root = getRootPtr();
		lv_obj_add_flag(root, LV_OBJ_FLAG_FLOATING); // Placed by its bounds, not its parent's layout
		lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
	}

	void DsPlotRect::registerXml()
	{
		lv_xml_register_widget("ds_plot_rect", xmlCreate, xmlApply);
	}

	void DsPlotRect::place()
	{
		lv_obj_t* root = getRootPtr();
		DsPlot* plot = DsPlot::of(root);
		lv_point_t corner1;
		lv_point_t corner2;
		if (plot == nullptr || !plot->toWidget(static_cast<float>(m_x1), static_cast<float>(m_y1), corner1) ||
			!plot->toWidget(static_cast<float>(m_x2), static_cast<float>(m_y2), corner2))
		{
			lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
			return;
		}

		// From the drawing area's content (inside its border) to the parent's content area
		lv_area_t area;
		lv_obj_get_content_coords(plot->area(), &area);
		lv_area_t content;
		lv_obj_get_content_coords(lv_obj_get_parent(root), &content);
		lv_obj_set_pos(root,
					   area.x1 - content.x1 + std::min(corner1.x, corner2.x),
					   area.y1 - content.y1 + std::min(corner1.y, corner2.y));
		lv_obj_set_size(
			root, std::max(1, std::abs(corner2.x - corner1.x)), std::max(1, std::abs(corner2.y - corner1.y)));
		lv_obj_remove_flag(root, LV_OBJ_FLAG_HIDDEN);
	}

	void* DsPlotRect::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* rect = new DsPlotRect(attrOr(attrs, "name", "rect"), parentOf(state));
		deleteWith(rect->getRootPtr(), rect);
		return rect->getRootPtr();
	}

	void DsPlotRect::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& rect = static_cast<DsPlotRect&>(*LvObj::fromPtr(root));
		const struct
		{
			const char* name; // Attribute naming the value
			int32_t* value;	  // The coordinate it sets
		} corners[] = {{"x1", &rect.m_x1}, {"y1", &rect.m_y1}, {"x2", &rect.m_x2}, {"y2", &rect.m_y2}};
		for (const auto& corner : corners)
		{
			if (const char* value = lv_xml_get_value_of(attrs, corner.name))
			{
				*corner.value = lv_xml_atoi(value);
			}
			const std::string bind = std::string("bind_") + corner.name;
			if (lv_subject_t* subject = findSubject(state, attrs, bind.c_str()))
			{
				lv_subject_add_observer_obj(subject, cornerObserverCb, root, corner.value);
			}
		}
		rect.place();
	}

	void DsPlotRect::cornerObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		*static_cast<int32_t*>(lv_observer_get_user_data(observer)) = lv_subject_get_int(subject);
		static_cast<DsPlotRect&>(*LvObj::fromPtr(lv_observer_get_target_obj(observer))).place();
	}
} // namespace UI::Xml

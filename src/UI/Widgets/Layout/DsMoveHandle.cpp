/*
 * DsMoveHandle.cpp
 */

#include "DsMoveHandle.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** Where the touch now is, on the screen */
		lv_point_t touchPoint()
		{
			lv_point_t point{};
			if (lv_indev_t* indev = lv_indev_active())
			{
				lv_indev_get_point(indev, &point);
			}
			return point;
		}

		/** `obj`'s position in its parent's content area, as lv_obj_set_pos() takes it */
		lv_point_t positionIn(lv_obj_t* obj)
		{
			lv_area_t coords;
			lv_area_t content;
			lv_obj_get_coords(obj, &coords);
			lv_obj_get_content_coords(lv_obj_get_parent(obj), &content);
			return {coords.x1 - content.x1, coords.y1 - content.y1};
		}
	} // namespace

	DsMoveHandle::DsMoveHandle(lv_obj_t* handle)
		: m_handle(handle)
	{
		lv_obj_add_flag(m_handle, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(m_handle, LV_OBJ_FLAG_PRESS_LOCK);
		lv_obj_remove_flag(m_handle, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(m_handle, LV_OBJ_FLAG_SCROLL_CHAIN);
		lv_obj_add_event_cb(m_handle, eventCb, LV_EVENT_ALL, this);
	}

	void DsMoveHandle::registerXml()
	{
		Widgets::add(
			{.name = "ds_move_handle",
			 .extends = "lv_obj",
			 .description =
				 "A grip that moves the widget it is in around that widget's parent when dragged, e.g. a floating "
				 "toolbar's. The first drag takes the widget out of its parent's layout; it is kept inside its parent. "
				 "What the grip looks like is the XML inside it.",
			 .example = R"(<lv_obj width="200" height="content" flex_flow="row" style_flex_cross_place="center">
	<ds_move_handle width="content" height="content" style_pad_all="0">
		<ds_icon icon="drag.png" width="32" height="48"/>
	</ds_move_handle>
	<lv_label text="Drag me"/>
</lv_obj>)"},
			xmlCreate,
			xmlApply);
	}

	void* DsMoveHandle::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		return lv_obj_create(parentOf(state));
	}

	void DsMoveHandle::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
		lv_obj_t* handle = itemOf(state);
		deleteWith(handle, new DsMoveHandle(handle));
	}

	void DsMoveHandle::eventCb(lv_event_t* e)
	{
		auto* self = static_cast<DsMoveHandle*>(lv_event_get_user_data(e));
		lv_obj_t* moved = lv_obj_get_parent(self->m_handle);
		if (moved == nullptr || lv_obj_get_parent(moved) == nullptr)
		{
			return;
		}
		switch (lv_event_get_code(e))
		{
		case LV_EVENT_PRESSED:
		{
			// Out of its parent's layout, where it is now, so it can go anywhere
			const lv_point_t position = positionIn(moved);
			if (!lv_obj_has_flag(moved, LV_OBJ_FLAG_FLOATING))
			{
				lv_obj_add_flag(moved, LV_OBJ_FLAG_FLOATING);
				lv_obj_set_align(moved, LV_ALIGN_TOP_LEFT);
				lv_obj_set_pos(moved, position.x, position.y);
			}
			self->m_start = Start{.touch = touchPoint(), .moved = position};
			break;
		}
		case LV_EVENT_PRESSING:
			if (self->m_start)
			{
				self->dragTo(touchPoint());
			}
			break;
		case LV_EVENT_RELEASED:
		case LV_EVENT_PRESS_LOST:
			self->m_start.reset();
			break;
		default:
			break;
		}
	}

	void DsMoveHandle::dragTo(const lv_point_t& touch) const
	{
		lv_obj_t* moved = lv_obj_get_parent(m_handle);
		lv_obj_t* parent = lv_obj_get_parent(moved);
		const int32_t maxX = std::max(0, lv_obj_get_content_width(parent) - lv_obj_get_width(moved));
		const int32_t maxY = std::max(0, lv_obj_get_content_height(parent) - lv_obj_get_height(moved));
		const int32_t x = std::clamp(m_start->moved.x + touch.x - m_start->touch.x, 0, maxX);
		const int32_t y = std::clamp(m_start->moved.y + touch.y - m_start->touch.y, 0, maxY);
		lv_obj_set_pos(moved, x, y);
	}
} // namespace UI::Xml

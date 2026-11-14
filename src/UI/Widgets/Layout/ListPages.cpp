/*
 * ListPages.cpp
 */

#include "ListPages.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include <algorithm>
#include <cmath>

namespace UI::Xml
{
	namespace
	{
		/** How far beyond a dot a tap still hits it, pixels: dots are small */
		constexpr int32_t DOT_TAP_MARGIN = 8;
	} // namespace

	ListPages::ListPages(lv_obj_t* container)
		: m_container(container)
	{
		lv_obj_set_layout(m_container, LV_LAYOUT_FLEX);
		lv_obj_set_flex_flow(m_container, LV_FLEX_FLOW_ROW);
		lv_obj_set_scroll_dir(m_container, LV_DIR_HOR);
		lv_obj_set_scroll_snap_x(m_container, LV_SCROLL_SNAP_CENTER);
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_SCROLL_ONE);
		lv_obj_set_scrollbar_mode(m_container, LV_SCROLLBAR_MODE_OFF);

		m_dots = lv_obj_create(m_container);
		lv_obj_remove_style_all(m_dots);
		lv_obj_add_style(m_dots, Themes::getComponentStyles().list_pages, LV_PART_MAIN);
		lv_obj_add_flag(m_dots, LV_OBJ_FLAG_FLOATING);
		lv_obj_remove_flag(m_dots, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(m_dots, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_size(m_dots, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(m_dots, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(m_dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_align(m_dots, LV_ALIGN_BOTTOM_MID, 0, 0);
		lv_obj_add_flag(m_dots, LV_OBJ_FLAG_HIDDEN);

		lv_obj_add_event_cb(m_container, sizeChangedCb, LV_EVENT_SIZE_CHANGED, this);
		lv_obj_add_event_cb(m_container, scrollEndCb, LV_EVENT_SCROLL_END, this);
	}

	ListPages::~ListPages()
	{
		lv_obj_remove_event_cb_with_user_data(m_container, sizeChangedCb, this);
		lv_obj_remove_event_cb_with_user_data(m_container, scrollEndCb, this);
		// The dots call back here, so they go now rather than with the rest of the container's children
		lv_obj_delete(m_dots);
	}

	void ListPages::refresh()
	{
		const auto shown = items();
		m_shown = shown.empty() ? 0 : std::min(m_shown, shown.size() - 1);
		lv_obj_move_foreground(m_dots);
		updateDots(shown.size());
		sizeItems(shown);
	}

	std::vector<lv_obj_t*> ListPages::items() const
	{
		std::vector<lv_obj_t*> result;
		const uint32_t count = lv_obj_get_child_count(m_container);
		for (uint32_t i = 0; i < count; i++)
		{
			lv_obj_t* child = lv_obj_get_child(m_container, static_cast<int32_t>(i));
			if (child != m_dots)
			{
				result.push_back(child);
			}
		}
		return result;
	}

	void ListPages::sizeItems(const std::vector<lv_obj_t*>& items)
	{
		const int32_t width = lv_obj_get_content_width(m_container);
		const int32_t height = std::max<int32_t>(0, lv_obj_get_content_height(m_container) - dotsHeight());
		for (lv_obj_t* item : items)
		{
			lv_obj_set_flex_grow(item, 0);
			lv_obj_set_size(item, width, height);
		}
	}

	void ListPages::updateDots(size_t count)
	{
		const size_t dots = count > 1 ? count : 0;
		while (lv_obj_get_child_count(m_dots) > dots)
		{
			lv_obj_delete(lv_obj_get_child(m_dots, -1));
		}
		const auto& styles = Themes::getComponentStyles();
		while (lv_obj_get_child_count(m_dots) < dots)
		{
			lv_obj_t* dot = lv_obj_create(m_dots);
			lv_obj_remove_style_all(dot);
			lv_obj_add_style(dot, styles.list_page_dot, LV_PART_MAIN);
			lv_obj_add_style(dot, styles.list_page_dot_current, LV_STATE_CHECKED);
			lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_set_ext_click_area(dot, DOT_TAP_MARGIN);
			lv_obj_add_event_cb(dot, dotClickedCb, LV_EVENT_CLICKED, this);
		}
		for (uint32_t i = 0; i < dots; i++)
		{
			lv_obj_set_state(lv_obj_get_child(m_dots, static_cast<int32_t>(i)), LV_STATE_CHECKED, i == m_shown);
		}
		lv_obj_set_flag(m_dots, LV_OBJ_FLAG_HIDDEN, dots == 0);
	}

	size_t ListPages::shownIndex(const std::vector<lv_obj_t*>& items) const
	{
		if (items.empty())
		{
			return 0;
		}
		const int32_t step = lv_obj_get_width(items.front()) + lv_obj_get_style_pad_column(m_container, LV_PART_MAIN);
		if (step <= 0)
		{
			return 0;
		}
		const auto index = static_cast<size_t>(
			std::max<long>(0, std::lround(static_cast<double>(lv_obj_get_scroll_x(m_container)) / step)));
		return std::min(index, items.size() - 1);
	}

	int32_t ListPages::dotsHeight() const
	{
		if (lv_obj_has_flag(m_dots, LV_OBJ_FLAG_HIDDEN) || lv_obj_get_child_count(m_dots) == 0)
		{
			return 0;
		}
		// The dots' own height is only known once laid out, so it is worked out from their styles
		const int32_t dot = lv_obj_get_style_height(lv_obj_get_child(m_dots, 0), LV_PART_MAIN);
		return std::max<int32_t>(0, dot) + lv_obj_get_style_pad_top(m_dots, LV_PART_MAIN) +
			   lv_obj_get_style_pad_bottom(m_dots, LV_PART_MAIN);
	}

	void ListPages::sizeChangedCb(lv_event_t* e)
	{
		auto* pages = static_cast<ListPages*>(lv_event_get_user_data(e));
		pages->sizeItems(pages->items());
		// Keep the same item shown. The items' positions are only worked out by the next layout, so the scroll is
		// set from their new width rather than by scrolling one into view.
		const int32_t step = lv_obj_get_content_width(pages->m_container) +
							 lv_obj_get_style_pad_column(pages->m_container, LV_PART_MAIN);
		lv_obj_scroll_to_x(pages->m_container, static_cast<int32_t>(pages->m_shown) * step, LV_ANIM_OFF);
	}

	void ListPages::scrollEndCb(lv_event_t* e)
	{
		auto* pages = static_cast<ListPages*>(lv_event_get_user_data(e));
		const auto shown = pages->items();
		pages->m_shown = pages->shownIndex(shown);
		pages->updateDots(shown.size());
	}

	void ListPages::dotClickedCb(lv_event_t* e)
	{
		auto* pages = static_cast<ListPages*>(lv_event_get_user_data(e));
		const auto* dot = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		const auto index = static_cast<size_t>(lv_obj_get_index(dot));
		const auto shown = pages->items();
		if (index < shown.size())
		{
			lv_obj_scroll_to_view(shown[index], LV_ANIM_ON);
		}
	}
} // namespace UI::Xml

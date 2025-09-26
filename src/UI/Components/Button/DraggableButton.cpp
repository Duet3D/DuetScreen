/*
 * DraggableButton.cpp
 *
 *  Created on: 2025-06-05
 *      Author: Andy Everitt
 */

#include "DraggableButton.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	DraggableButton::DraggableButton(const std::string& name, LvObj& parent, const std::string& text)
		: Button(name, parent, text)
	{
		// lv_obj_add_flag(getCont(), LV_OBJ_FLAG_OVERFLOW_VISIBLE);
		setExtDrawSize(200);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		// Add styles
		addStyle(Themes::getLvglStyles().draggable, 0);
		addStyle(Themes::getLvglStyles().dragging, LV_STATE_PRESSED);
		addStyle(Themes::getLvglStyles().drag_complete, LV_STATE_USER_1);

		// Add event callback
		lv_obj_add_event_cb(getButton(), dragEventCallback, LV_EVENT_ALL, this);
	}

	void DraggableButton::setDragCallback(DraggableButton::drag_cb_t drag_cb, void* user_data)
	{
		UI_LOCK();
		m_dragCallback = drag_cb;
		m_dragUserData = drag_cb ? user_data : nullptr;
	}

	void DraggableButton::setDragThreshold(lv_coord_t threshold)
	{
		UI_LOCK();
		if (threshold < 0)
		{
			LOG_ERROR("Drag threshold cannot be negative");
			return;
		}
		LOG_DBG("Setting drag threshold to {}", threshold);
		m_dragThreshold = threshold;
		m_dragThresholdSqr = m_dragThreshold * m_dragThreshold;
	}

	float DraggableButton::getDragPct(const lv_point_t& start_pos, const lv_event_t* e) const
	{
		lv_point_t curr_pos;
		lv_indev_get_point(lv_indev_get_act(), &curr_pos);

		lv_coord_t dx = curr_pos.x - start_pos.x;
		lv_coord_t dy = curr_pos.y - start_pos.y;

		lv_coord_t distSqr = dx * dx + dy * dy;

		float pct = (float)distSqr / (m_dragThresholdSqr);

		pct = pct > 1.0f ? 1.0f : (pct < 0.0f ? 0.0f : pct);
		return pct;
	}

	void DraggableButton::dragEventCallback(lv_event_t* e)
	{
		// Get the button instance from the user data
		DraggableButton* button = static_cast<DraggableButton*>(lv_event_get_user_data(e));
		if (!button)
		{
			LOG_ERROR("DraggableButton: No user data found in event");
			return;
		}

		lv_event_code_t code = lv_event_get_code(e);
		static lv_point_t start_pos;

		if (code == LV_EVENT_PRESSED)
		{
			// lv_obj_add_flag(button->getButton(), LV_OBJ_FLAG_OVERFLOW_VISIBLE);
			// lv_obj_refresh_ext_draw_size(button->getButton());
			lv_obj_refresh_ext_draw_size(button->getRoot());
			lv_obj_invalidate(button->getRoot());
			lv_indev_get_point(lv_indev_get_act(), &start_pos);
		}
		else if (code == LV_EVENT_PRESSING)
		{
			// lv_obj_refresh_ext_draw_size(button->getButton());
			// lv_obj_refresh_ext_draw_size(button->getCont());
			float pct = button->getDragPct(start_pos, e);

			lv_style_value_t borderWidth;
			lv_style_get_prop(Themes::getLvglStyles().dragging, LV_STYLE_BORDER_WIDTH, &borderWidth);

			lv_obj_set_style_border_width(button->getButton(), borderWidth.num * (1 - pct), LV_STATE_PRESSED);

			if (!lv_obj_has_state(button->getButton(), LV_STATE_USER_1) && pct == 1.0f)
			{
				lv_obj_set_state(button->getButton(), LV_STATE_USER_1, true);
				lv_obj_invalidate(button->getButton());
			}
			else if (lv_obj_has_state(button->getButton(), LV_STATE_USER_1) && pct < 1.0f)
			{
				lv_obj_set_state(button->getButton(), LV_STATE_USER_1, false);
				lv_obj_invalidate(button->getButton());
			}
		}
		else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
		{
			lv_obj_remove_local_style_prop(button->getButton(), LV_STYLE_BORDER_WIDTH, LV_STATE_PRESSED);

			// lv_obj_remove_flag(button->getButton(), LV_OBJ_FLAG_OVERFLOW_VISIBLE);
			float pct = button->getDragPct(start_pos, e);
			lv_obj_set_state(button->getButton(), LV_STATE_USER_1, false);

			if (button->m_dragCallback)
			{
				button->m_dragCallback(pct, button->m_dragUserData);
			}
		}
	}
} // namespace UI

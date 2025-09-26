/*
 * DraggableButton.h
 *
 *  Created on: 2025-06-05
 *      Author: Andy Everitt
 */

#pragma once

#include "Button.h"

namespace UI
{
	class DraggableButton : public Button
	{
		using drag_cb_t = std::function<void(float pct, void* user_data)>;

	  public:
		DraggableButton(const std::string& name, LvObj& parent, const std::string& text);

		void setDragCallback(drag_cb_t drag_cb, void* user_data);
		void setDragThreshold(lv_coord_t threshold);

	  private:
		static void dragEventCallback(lv_event_t* e);

		float getDragPct(const lv_point_t& start_pos, const lv_event_t* e) const;

		drag_cb_t m_dragCallback;
		void* m_dragUserData = nullptr;
		lv_coord_t m_dragThreshold = 60; // Default drag threshold in pixels
		lv_coord_t m_dragThresholdSqr = m_dragThreshold * m_dragThreshold;
	};
} // namespace UI

/*
 * EStop.cpp
 *
 *  Created on: 2025-08-27
 *      Author: Andy Everitt
 */

#include "EStop.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	EStop::EStop(const std::string& name, LvObj& parent)
		: View(name, parent, _("estop"))
	{
		UI_LOCK();

		setIcon(IMAGE_ASSET("estop.png"));
		addStyle(Themes::getComponentStyles().estop, LV_PART_MAIN, true);

		m_label.setSize(LV_PCT(100), LV_PCT(100));
		m_label.setRadius(LV_PCT(70));
		m_label.setText(_("estop_prompt"));
		m_label.setAngleStart(200);
		m_label.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_label.setAlign(LV_ALIGN_CENTER);

		addEventCallback(
			[](lv_event_t* e)
			{
				lv_event_code_t code = lv_event_get_code(e);
				EStop& estop = *static_cast<EStop*>(lv_event_get_user_data(e));
				switch (code)
				{
				case LV_EVENT_PRESSED:
					estop.m_label.hide();
					break;
				case LV_EVENT_RELEASED:
					estop.m_label.show();
					break;
				}
			},
			LV_EVENT_ALL,
			this);

		setDragCallback(eStopDraggedEvent, this);
	}

	void EStop::eStopDraggedEvent(float pct, void* user_data)
	{
		LOG_INFO("E-Stop button dragged");
		EStop* estop = static_cast<EStop*>(user_data);
		if (pct < 0.5f)
		{
			Model::get().post<EventType::Response>(std::string(_("estop_prompt")));
		}

		if (pct == 1.0f)
		{
			estop->m_presenter->eStop();
		}
	}
} // namespace UI

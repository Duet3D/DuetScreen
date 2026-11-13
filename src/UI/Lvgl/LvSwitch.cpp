/*
 * LvSwitch.cpp
 *
 *  Created on: 2026-01-06
 *      Author: Andy Everitt
 */

#include "LvSwitch.h"
#include "Debug.h"

namespace UI
{
	LvSwitch::LvSwitch(const std::string& name, LvObj& parent)
		: LvSwitchGen(name, parent)
	{
		UI_LOCK();

		addEventCallback(
			[this](lv_event_t*)
			{
				if (m_checkedCallback)
				{
					m_checkedCallback(getChecked());
				}
			},
			LV_EVENT_VALUE_CHANGED);
	}

	void LvSwitch::setChecked(bool checked)
	{
		UI_LOCK();

		const bool prev = hasState(LV_STATE_CHECKED);

		setState(LV_STATE_CHECKED, checked);

		if (!m_checkedInitialised || (prev != checked))
		{
			m_checkedInitialised = true;
			sendEvent(LV_EVENT_VALUE_CHANGED);
		}
	}
} // namespace UI

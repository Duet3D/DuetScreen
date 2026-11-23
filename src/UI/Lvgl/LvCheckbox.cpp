/*
 * LvCheckbox.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "LvCheckbox.h"
#include "Debug.h"

namespace UI
{
	/**
	 * Construct a checkbox with the given name and parent.
	 * - Creates the underlying LVGL object via lv_checkbox_create
	 * - Locks UI mutex during creation
	 */
	LvCheckbox::LvCheckbox(const std::string& name, LvObj& parent)
		: LvCheckboxGen(name, parent)
	{
		UI_LOCK();

		setText("");

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

	void LvCheckbox::setChecked(bool checked)
	{
		const bool prev = hasState(LV_STATE_CHECKED);

		setState(LV_STATE_CHECKED, checked);

		if (!m_checkedInitialised || (prev != checked))
		{
			m_checkedInitialised = true;
			sendEvent(LV_EVENT_VALUE_CHANGED);
		}
	}

	void LvCheckbox::setCheckedCallback(checked_callback_t cb)
	{
		UI_LOCK();
		m_checkedCallback = std::move(cb);
	}

	bool LvCheckbox::getChecked() const
	{
		return hasState(LV_STATE_CHECKED);
	}
} // namespace UI

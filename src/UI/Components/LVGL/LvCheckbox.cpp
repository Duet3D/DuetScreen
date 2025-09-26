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
		: LvObj(lv_checkbox_create, name, parent)
	{
		UI_LOCK();

		addEventCallback(
			[](lv_event_t* e)
			{
				auto checkbox = static_cast<LvCheckbox*>(lv_event_get_user_data(e));
				if (checkbox->m_checkedCallback)
				{
					checkbox->m_checkedCallback(checkbox->getChecked());
				}
			},
			LV_EVENT_VALUE_CHANGED,
			this);
	}

	/**
	 * Set the text of the checkbox. The text is copied by LVGL and can be freed after the call.
	 * Equivalent to lv_checkbox_set_text.
	 * @param txt The text to display
	 */
	void LvCheckbox::setText(std::string_view txt)
	{
		UI_LOCK();
		lv_checkbox_set_text(getRoot(), txt.data());
	}

	/**
	 * Set the text of the checkbox using a static string. The pointer must remain valid
	 * for the lifetime of the checkbox. Equivalent to lv_checkbox_set_text_static.
	 * @param txt The static string pointer to use as label text.
	 */
	void LvCheckbox::setTextStatic(const char* txt)
	{
		UI_LOCK();
		lv_checkbox_set_text_static(getRoot(), txt);
	}

	void LvCheckbox::setChecked(bool checked)
	{
		setState(LV_STATE_CHECKED, checked);
	}

	void LvCheckbox::setCheckedCallback(checked_callback_t cb)
	{
		UI_LOCK();
		m_checkedCallback = cb;
	}

	/**
	 * Get the text associated with the checkbox.
	 * Equivalent to lv_checkbox_get_text.
	 * @return const char* Pointer to the internal text buffer managed by LVGL.
	 */
	std::string_view LvCheckbox::getText() const
	{
		UI_LOCK();
		return lv_checkbox_get_text(getRoot());
	}

	bool LvCheckbox::getChecked() const
	{
		return hasState(LV_STATE_CHECKED);
	}
} // namespace UI

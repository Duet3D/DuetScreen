/*
 * ToolControl.cpp
 *
 *  Created on: 2025-07-14
 *      Author: Andy Everitt
 */

#include "ToolControl.h"
#include "Debug.h"

namespace UI
{
	ToolControl::ToolControl(const std::string& name, LvObj& parent)
		: View(name, parent)
		, m_toolInfoCont("tool_info", getRoot())
		, m_name("tool_name", m_toolInfoCont)
		, m_state("tool_state", m_toolInfoCont)
		, m_heaters("heaters", getRoot())
	{
		UI_LOCK();
		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_toolInfoCont.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_toolInfoCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_toolInfoCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_name.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_state.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_heaters.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_heaters.setListFlow(LV_FLEX_FLOW_COLUMN);
		m_heaters.setListSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_name.addClickedCallback(
			[](lv_event_t* e)
			{
				auto& control = *(ToolControl*)lv_event_get_user_data(e);
				control.m_presenter->toggleToolState();
			},
			this);

		addStyle(Themes::getComponentStyles().tool_selected, LV_STATE_CHECKED);
	}

	void ToolControl::setToolName(std::string_view name)
	{
		m_name.setText(name);
	}

	void ToolControl::setToolState(ToolControlPresenter::tool_state_t state, std::string_view str)
	{
		bool active = state == ToolControlPresenter::tool_state_t::active;
		setState(LV_STATE_CHECKED, active);
		m_name.setChecked(active);
		m_state.setText(str);
	}

	void ToolControl::setNumberPad(NumberPad* numberPad)
	{
		UI_LOCK();
		m_numberPad = numberPad;
		for (auto heater : m_heaters)
		{
			heater->setNumberPad(numberPad);
		}
	}
} // namespace UI

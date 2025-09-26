/*
 * VerticalButtonPanel.cpp
 *
 *  Created on: 2025-02-28
 *      Author: Andy Everitt
 */

#include "VerticalButtonPanel.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	VerticalButtonPanel::VerticalButtonPanel(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_reset("reset", getRoot(), "")
		, m_increment("increment", getRoot(), "")
		, m_decrement("decrement", getRoot(), "")
		, m_valueCont("value_cont", getRoot())
		, m_values{Button("value1", m_valueCont, ""), Button("value2", m_valueCont, "")}
	{
		UI_LOCK();
		lv_obj_set_layout(getRoot(), LV_LAYOUT_FLEX);
		lv_obj_set_flex_flow(getRoot(), LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(getRoot(), LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		for (size_t i = 0; i < lv_obj_get_child_cnt(getRoot()); i++)
		{
			lv_obj_t* child = lv_obj_get_child(getRoot(), i);
			lv_obj_set_width(child, LV_PCT(100));
		}

		m_reset.setFlexGrow(1);
		m_increment.setFlexGrow(3);
		m_decrement.setFlexGrow(3);
		m_valueCont.setFlexGrow(2);

		m_valueCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_valueCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		for (auto& v : m_values)
		{
			v.setHeight(LV_PCT(100));
			v.setFlexGrow(1);
			v.setCheckable(true);
			v.setChecked(false);
			v.setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(&v - m_values)));
			v.addClickedCallback(
				[](lv_event_t* e)
				{
					UI_LOCK();
					VerticalButtonPanel* panel = static_cast<VerticalButtonPanel*>(lv_event_get_user_data(e));
					lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
					panel->setSelectedValueIndex(
						static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn))));
				},
				this);
		}
		m_values[m_selectedValueIndex].setChecked(true);

		m_decrement.addClickedCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				VerticalButtonPanel* panel = static_cast<VerticalButtonPanel*>(lv_event_get_user_data(e));
				if (panel->m_valueChangeCallback)
				{
					panel->m_valueChangeCallback(-panel->getSelectedValue());
				}
			},
			this);

		m_increment.addClickedCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				VerticalButtonPanel* panel = static_cast<VerticalButtonPanel*>(lv_event_get_user_data(e));
				if (panel->m_valueChangeCallback)
				{
					panel->m_valueChangeCallback(panel->getSelectedValue());
				}
			},
			this);

		m_reset.addClickedCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				VerticalButtonPanel* panel = static_cast<VerticalButtonPanel*>(lv_event_get_user_data(e));
				if (panel->m_resetCallback)
				{
					panel->m_resetCallback();
				}
			},
			this);
	}

	void VerticalButtonPanel::setIncrementLabel(std::string_view label)
	{
		m_increment.setText(label);
	}

	void VerticalButtonPanel::setDecrementLabel(std::string_view label)
	{
		m_decrement.setText(label);
	}

	void VerticalButtonPanel::setResetLabel(std::string_view label)
	{
		m_reset.setText(label);
	}

	void VerticalButtonPanel::setValueLabelFmt(std::string_view fmt)
	{
		UI_LOCK();
		m_fmt = fmt;

		updateValueLabels();
	}

	void VerticalButtonPanel::setIncrementValues(const std::array<float, 2>& values)
	{
		UI_LOCK();
		m_incrementValues[0] = values[0];
		m_incrementValues[1] = values[1];

		updateValueLabels();
	}

	void VerticalButtonPanel::updateValueLabels()
	{
		m_values[0].setText(fmt::format(fmt::runtime(m_fmt), m_incrementValues[0]).c_str());
		m_values[1].setText(fmt::format(fmt::runtime(m_fmt), m_incrementValues[1]).c_str());
	}

	float VerticalButtonPanel::getSelectedValue() const
	{
		return m_incrementValues[m_selectedValueIndex];
	}

	void VerticalButtonPanel::setSelectedValueIndex(uint8_t index)
	{
		UI_LOCK();
		if (index >= 2)
		{
			return;
		}
		m_values[m_selectedValueIndex].setChecked(false); // uncheck the current value
		m_selectedValueIndex = index;
		m_values[m_selectedValueIndex].setChecked(true); // check the new value
	}

	void VerticalButtonPanel::setValueChangeCallback(std::function<void(float)> callback)
	{
		m_valueChangeCallback = callback;
	}

	void VerticalButtonPanel::setResetCallback(std::function<void()> callback)
	{
		m_resetCallback = callback;
	}
} // namespace UI

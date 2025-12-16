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
		, m_reset("reset", getRoot())
		, m_increment("increment", getRoot())
		, m_decrement("decrement", getRoot())
		, m_values{"value_list", getRoot()}
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		iterateChildren([](size_t /* i */, LvObj& child) { child.setWidth(LV_PCT(100)); });

		m_reset.setFlexGrow(1);
		m_reset.setMinHeight(LV_SIZE_CONTENT);
		m_increment.setFlexGrow(3);
		m_decrement.setFlexGrow(3);
		m_increment.setMinHeight(LV_SIZE_CONTENT);
		m_decrement.setMinHeight(LV_SIZE_CONTENT);

		m_values.setFlexGrow(2);
		m_values.setMinHeight(LV_SIZE_CONTENT);
		m_values.setListFlow(LV_FLEX_FLOW_ROW);
		m_values.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_values.getListContainer().setFlexGrow(1);
		// m_values.getListContainer().setHeight(LV_PCT(100));
		m_values.getListContainer().setMinHeight(LV_SIZE_CONTENT);
		m_values.setStylePad(0);
		m_values.getListContainer().setStylePad(0);
		m_values.setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
		m_values.setStyleBgOpa(LV_OPA_COVER);

		m_increment.setIcon("increment.png");
		m_decrement.setIcon("decrement.png");

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

	void VerticalButtonPanel::setIncrementIcon(std::string_view icon)
	{
		m_increment.setIcon(icon);
	}

	void VerticalButtonPanel::setDecrementIcon(std::string_view icon)
	{
		m_decrement.setIcon(icon);
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

	void VerticalButtonPanel::setIncrementValues(const std::vector<float>& values)
	{
		UI_LOCK();
		m_values.setItemCount(values.size(), this, &VerticalButtonPanel::createValueButton);
		m_incrementValues = values;

		updateValueLabels();
	}

	std::unique_ptr<Button> VerticalButtonPanel::createValueButton(size_t index, LvObj& parent)
	{
		auto btn = std::make_unique<Button>(fmt::format("value_btn_{}", index), parent);
		btn->getLabel().setLongMode(LV_LABEL_LONG_MODE_WRAP);
		btn->setHeight(LV_PCT(100));
		btn->setMinHeight(LV_SIZE_CONTENT);
		btn->setFlexGrow(1);
		btn->setCheckable(true);
		btn->setChecked(index == m_selectedValueIndex);
		btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(index)));
		btn->addClickedCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				VerticalButtonPanel* panel = static_cast<VerticalButtonPanel*>(lv_event_get_user_data(e));
				LvObj* btn = LvObj::fromPtr(lv_event_get_target_obj(e));
				panel->setSelectedValueIndex(static_cast<uint8_t>(reinterpret_cast<uintptr_t>(btn->getUserData())));
			},
			this);
		return btn;
	}

	void VerticalButtonPanel::updateValueLabels()
	{
		m_values.iterateListItems([this](size_t index, Button& btn)
								  { btn.setText(fmt::format(fmt::runtime(m_fmt), m_incrementValues.at(index))); });
	}

	float VerticalButtonPanel::getSelectedValue() const
	{
		return m_incrementValues[m_selectedValueIndex];
	}

	void VerticalButtonPanel::setSelectedValueIndex(uint8_t index)
	{
		UI_LOCK();
		m_values.iterateListItems([index](size_t i, Button& btn) { btn.setChecked(i == index); });
		m_selectedValueIndex = index;
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

/*
 * MultiValueSelector.cpp
 *
 *  Created on: 2025-11-16
 *      Author: Andy Everitt
 */

#include "MultiValueSelector.h"
#include "Debug.h"

namespace UI
{
	MultiValueSelector::MultiValueSelector(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_topRow.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_topRow.setSize(LV_PCT(100), LV_PCT(50));
		m_topRow.setMinHeight(LV_SIZE_CONTENT);

		m_decrementBtn.setIcon("decrement.png");
		m_incrementBtn.setIcon("increment.png");

		m_valueDisplay.setHeight(LV_PCT(100));
		// m_valueDisplay.setMinHeight(LV_SIZE_CONTENT);
		m_valueDisplay.setFlexGrow(1);
		m_valueDisplay.setOneLine(true);
		m_valueDisplay.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);

		m_valueBtns.setWidth(LV_PCT(100));
		m_valueBtns.setFlexGrow(1);
		m_valueBtns.setMinHeight(LV_SIZE_CONTENT);
		m_valueBtns.setListFlow(LV_FLEX_FLOW_ROW);
		m_valueBtns.getListContainer().setWidth(LV_PCT(100));
		m_valueBtns.getListContainer().setFlexGrow(1);
		m_valueBtns.getListContainer().setMinHeight(LV_SIZE_CONTENT);

		setValue(0.0f);
	}

	void MultiValueSelector::setValue(float value)
	{
		m_value = value;
		m_valueDisplay.setText(std::to_string(value));
	}

	float MultiValueSelector::getValue() const
	{
		return m_value;
	}

	void MultiValueSelector::setIncrement(float increment)
	{
		m_incrementValue = increment;
	}

	void MultiValueSelector::setValues(const std::vector<float>& values)
	{
		UI_LOCK();
		m_currentValues = values;

		m_valueBtns.setItemCount(values.size(),
								 [](size_t index, LvObj& parent)
								 {
									 auto btn = std::make_unique<Button>(std::to_string(index), parent);
									 btn->setHeight(LV_PCT(100));
									 btn->setMinHeight(LV_SIZE_CONTENT);
									 btn->setFlexGrow(1);
									 btn->setMinWidth(LV_SIZE_CONTENT);
									 return btn;
								 });

		m_valueBtns.iterateListItems([&values](size_t index, Button& btn)
									 { btn.setText(std::to_string(values.at(index))); });
	}
} // namespace UI

/*
 * MultiValueSelector.h
 *
 *  Created on: 2025-11-16
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/TextBox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/List/List.h"

namespace UI
{
	class MultiValueSelector : public LvContainer
	{
	  public:
		MultiValueSelector(const std::string& name, LvObj& parent);

		void setValue(float value);
		float getValue() const;

		void setIncrement(float increment);
		void setValues(const std::vector<float>& values);

	  private:
		LvContainer m_topRow{"topRow", getRoot()};
		Button m_decrementBtn{"decrement", m_topRow};
		TextBox m_valueDisplay{"valueDisplay", m_topRow};
		Button m_incrementBtn{"increment", m_topRow};
		List<Button> m_valueBtns{"values", getRoot()};

		float m_incrementValue = 1.0f;
		float m_value = 0.0f;
		std::vector<float> m_currentValues;
	};
} // namespace UI

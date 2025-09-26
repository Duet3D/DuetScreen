/*
 * VerticalButtonPanel.h
 *
 *  Created on: 2025-02-28
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Core/View.h"
#include "UI/Components/Button/Button.h"

namespace UI
{
	class VerticalButtonPanel : public LvObj
	{
      public:
		VerticalButtonPanel(const std::string& name, LvObj& parent);
		void setIncrementLabel(std::string_view label);
		void setDecrementLabel(std::string_view label);
		void setResetLabel(std::string_view label);
		void setValueLabelFmt(std::string_view fmt);
		void setIncrementValues(const std::array<float, 2>& values);
        
        float getSelectedValue() const;
        void setSelectedValueIndex(uint8_t index);

        void setValueChangeCallback(std::function<void(float)> callback);
        void setResetCallback(std::function<void()> callback);

	  private:
		void updateValueLabels();

		Button m_reset;
		Button m_increment;
		Button m_decrement;

		LvContainer m_valueCont;
		Button m_values[2];

        uint8_t m_selectedValueIndex = 0;

        std::string m_fmt;
        float m_incrementValues[2];

        std::function<void(float)> m_valueChangeCallback;
        std::function<void()> m_resetCallback;
	};
} // namespace UI

/*
 * ToolControl.h
 *
 *  Created on: 2025-07-14
 *      Author: Andy Everitt
 */

#pragma once

#include "HeaterSlider.h"
#include "ToolControlPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"

namespace UI
{
	class ToolControl : public View<ToolControlPresenter>
	{
	  public:
		ToolControl(const std::string& name, LvObj& parent);

		void setToolName(std::string_view name);
        void setToolState(ToolControlPresenter::tool_state_t state, std::string_view str);
		List<HeaterSlider>& getHeaters() { return m_heaters; }

		void setNumberPad(NumberPad* numberPad);
		auto getNumberPad() { return m_numberPad; }

	  private:
		LvContainer m_toolInfoCont;
		Button m_name;
		LvLabel m_state;
		List<HeaterSlider> m_heaters;

		NumberPad* m_numberPad = nullptr;
	};
} // namespace UI

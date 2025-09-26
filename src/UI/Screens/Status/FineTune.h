/*
 * FineTune.h
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#pragma once

#include "FineTunePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"
#include "UI/Widgets/BabyStep/BabyStep.h"

namespace UI
{
	class FineTune : public View<FineTunePresenter>
	{
	  public:
		FineTune(LvObj& parent, lv_obj_t* numberpad_parent = nullptr);

		void setSpeedValue(uint32_t value);

		void setExtruderCount(size_t count);
		void setFanCount(size_t count);
		size_t getExtruderCount() const { return m_extruders.getItemCount(); }
		size_t getFanCount() const { return m_fans.getItemCount(); }

		void setExtruderLabel(size_t index, std::string_view label);
		void setExtruderValue(size_t index, uint32_t value);

		void setFanLabel(size_t index, std::string_view label);
		void setFanValue(size_t index, uint32_t value);

		void showKeyboard(bool show);

	  protected:
		void onShow() override;

		BabyStep m_babystep;
		LvContainer m_sliderCont;

		// Speed Factor
		Slider m_speed;

		// Extruders
		List<Slider> m_extruders;

		// Fans
		List<Slider> m_fans;

		LvKeyboard m_keyboard;
		ModalNumberPad m_numberPad;
	};
} // namespace UI

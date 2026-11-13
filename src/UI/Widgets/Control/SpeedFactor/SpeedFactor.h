/*
 * SpeedFactor.h
 *
 *  Created on: 2025-11-10
 *      Author: Andy Everitt
 */

#pragma once

#include "SpeedFactorPresenter.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/List/List.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"

namespace UI
{
	class SpeedFactor : public View<SpeedFactorPresenter>
	{
	  public:
		SpeedFactor(const std::string& name, LvObj& parent);

		void setSpeedValue(float value);
		void setNumberPad(NumberPad* numberPad) { m_numberPad = numberPad; }

		Slider& getSlider() { return *m_speed.getItem(0); }

	  private:
		/* List because it is plausible that we might want speed control for both motion systems */
		List<Slider> m_speed{"slider", getRoot()};
		NumberPad* m_numberPad = nullptr;
	};

	class ModalSpeedFactor : public Modal<LvContainer>
	{
	  public:
		ModalSpeedFactor(const std::string& name, LvObj& parent);

		auto& getSpeedFactor() { return m_speedFactor; }
		NumberPad& getNumberPad() { return m_numberPad; }

	  private:
		void onShow() override;

		SpeedFactor m_speedFactor{"speed_factor", getRoot()};
		NumberPad m_numberPad{"number_pad", getRoot(), layout_t(0, 0, 50, 100)};
	};
} // namespace UI

/*
 * GenericAxisControl.h
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Core/View.h"

namespace UI
{
	class GenericAxisControl : public LvObj
	{
		using jog_cb_t = std::function<void(char axis_letter, bool forward)>;
		using home_cb_t = std::function<void(char axis_letter)>;
		using label_cb_t = std::function<void(char axis_letter, float position)>;

	  public:
		GenericAxisControl(const std::string& name, LvObj& parent);

		char getAxisLetter() const { return m_axisLetter; }
		void setAxisLetter(const char letter);
		void setAxisPosition(float value);
		void setAxisHomed(bool homed) { m_homeButton.setChecked(!homed); }

		void setDisabled(bool disabled);
		void setJogDisabled(bool disabled);
		void setHomeDisabled(bool disabled);

		void setJogCallback(jog_cb_t cb);
		void setHomeCallback(home_cb_t cb);
		void setLabelCallback(label_cb_t cb);

	  private:
		static void onJogBtn(lv_event_t* event);
		static void onHomeBtn(lv_event_t* event);
		static void onLabelClick(lv_event_t* event);

		void updateLabel();

		LvLabel m_label;
		Button m_incrementButton;
		Button m_homeButton;
		Button m_decrementButton;

		char m_axisLetter = '\0';
		float m_axisPosition = 0.0f;

		jog_cb_t m_positionCallback;
		home_cb_t m_homeCallback;
		label_cb_t m_labelCallback;
	};
} // namespace UI

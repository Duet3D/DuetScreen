/*
 * Slider.h
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvSlider.h"
#include "UI/Components/LVGL/LvTextArea.h"
#include "UI/Core/View.h"
#include <functional>

namespace UI
{
	class Slider : public LvObj
	{
	  public:
		enum class SendMode
		{
			VALUE_CONFIRMED, // Only run callback once the slider has been released or the buttons are pressed
			VALUE_CHANGED,	 // Run callback as soon as the slider is moved or the buttons are pressed
		};

		enum class OutOfRange
		{
			NONE,  // Do not allow the value to go out of range
			BOTH,  // Allow the value to go out of range in both directions
			UPPER, // Allow the value to go out of range in the +ve direction
			LOWER, // Allow the value to go out of range in the -ve direction
		};

		Slider(const std::string& name, LvObj& parent);

		float getValue() const { return m_value; }
		float getMin() const { return m_min; }
		float getMax() const { return m_max; }
		lv_obj_t* getInput() const { return m_input; }

		bool isFocused() const { return m_focused; }

		void setOutOfRangeMode(OutOfRange mode);
		void setLabel(std::string_view text);
		void setIncrementValue(float value);
		void setRange(float min, float max);
		void setValue(float value);
		void setSendMode(SendMode mode) { m_sendMode = mode; }
		void setLongPressedEnabled(bool enabled) { m_longPressEnabled = enabled; }
		void setKeyboard(lv_obj_t* keyboard) { m_keyboard = keyboard; }
		void setValueChangedCallback(std::function<void(int32_t)> callback) { m_valueChangedCallback = callback; }
		void setFocusedCallback(std::function<void(bool)> callback) { m_focusedCallback = callback; }

	  protected:
		static void onValueChanged(lv_event_t* e);
		static void onInputEvent(lv_event_t* e);

		bool boundValue(float& value);
		int32_t normaliseValue(float value) const;
		void updateText();

		LvLabel m_label;
		LvContainer m_sliderCont;

		Button m_decrement;
		LvSlider m_slider;
		Button m_increment;
		LvTextArea m_input;

		float m_incrementValue;
		lv_obj_t* m_keyboard;

		float m_min;
		float m_max;
		float m_value;
		bool m_focused = false;
		bool m_longPressEnabled = true;
		SendMode m_sendMode = SendMode::VALUE_CONFIRMED;
		OutOfRange m_outOfRangeMode = OutOfRange::NONE;
		std::function<void(int32_t)> m_valueChangedCallback;
		std::function<void(int32_t)> m_focusedCallback;
	};
} // namespace UI

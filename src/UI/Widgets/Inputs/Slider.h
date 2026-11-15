/*
 * Slider.h
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Widgets/Buttons/Button.h"
#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvKeyboard.h"
#include "UI/Lvgl/LvLabel.h"
#include "UI/Lvgl/LvSlider.h"
#include "UI/Lvgl/LvTextarea.h"
#include "UI/Core/View.h"
#include <functional>
#include <optional>

namespace UI
{
	class Slider : public LvContainer
	{
	  public:
		using value_changed_callback_t = std::function<void(float)>;
		using focused_callback_t = std::function<void(bool)>;

		enum class SendMode
		{
			VALUE_CONFIRMED, // Only run callback once the slider has been released or the buttons are pressed
			VALUE_CHANGED,	 // Run callback as soon as the slider is moved or the buttons are pressed
			DISABLED		 // Do not run the callback
		};

		Slider(const std::string& name, LvObj& parent);
		/** For widgets built by the XML engine, whose parent has no C++ wrapper */
		Slider(const std::string& name, lv_obj_t* parent);

		LvLabel& getLabel() { return m_label; }
		std::string_view getLabelText() const { return m_label.getText(); }
		float getValue() const { return m_value; }
		/** What the - and + buttons take off or add */
		float getIncrementValue() const { return m_incrementValue; }
		float getMin() const { return m_min; }
		float getMax() const { return m_max; }
		LvSlider& getLvSlider() { return m_slider; }
		LvTextarea& getInput() { return m_input; }

		bool isFocused() const { return m_focused; }

		/**
		 * The least and the most the value may be, dragged, typed or reset, where they differ from the ends of the
		 * slider's range (setRange()): a value past an end shows the slider at that end. Unset, a limit is that end;
		 * an infinite one is none, e.g. a timeout typed longer than the slider reaches. A range that follows the value
		 * (a window either side of it) keeps the value's true limits here.
		 */
		void setLimits(std::optional<float> lowest, std::optional<float> highest);
		/** Show `text` above the slider (unless the label is kept hidden), and as the number pad's header */
		void setLabel(std::string_view text);

		/** Keep the label hidden, whatever it says (`false`), or show it while it says something (`true`) */
		void setLabelShown(bool shown);
		void setIncrementValue(float value);
		void setRange(float min, float max);
		void setValue(float value);
		void setDefaultValue(float value);
		void setDisplayDecimals(int decimals);

		/** Show `unit` after the value, e.g. "%" or "s"; empty for none */
		void setUnit(std::string_view unit);

		/** The value's unit, e.g. "%"; empty if it has none */
		std::string_view getUnit() const { return m_unit.getText(); }
		void setSendMode(SendMode mode) { m_sendMode = mode; }
		void setLongPressedEnabled(bool enabled) { m_longPressEnabled = enabled; }
		void setValueChangedCallback(value_changed_callback_t callback) { m_valueChangedCallback = callback; }

		/**
		 * @brief Set a callback to run when the focus state of the text input changes
		 */
		void setFocusedCallback(focused_callback_t callback) { m_focusedCallback = callback; }

	  protected:
		static void onValueChanged(lv_event_t* e);
		static void onInputEvent(lv_event_t* e);

		bool boundValue(float& value);
		int32_t normaliseValue(float value) const;
		void updateText();
		bool hasDefaultValue() const { return !std::isnan(m_defaultValue); }

		/**
		 * The value was tapped: ask for a new one somewhere else, e.g. on a number pad, and set it with setValue().
		 * @return false to type it in place instead (the default)
		 */
		virtual bool askForValue() { return false; }

		/** The least the value may be: its lower limit, else the bottom of the range */
		float lowest() const { return m_lowest.value_or(m_min); }

		/** The most the value may be: its upper limit, else the top of the range */
		float highest() const { return m_highest.value_or(m_max); }

	  private:
		void init();
		void onShow() override;

		LvContainer m_header{"header", getRoot()};
		LvContainer m_sliderCont{"slider_cont", getRoot()};

		LvLabel m_label{"label", m_header};
		Button m_reset{"reset", m_header};

		Button m_decrement{"decrement", m_sliderCont};
		LvSlider m_slider{"slider", m_sliderCont};
		Button m_increment{"increment", m_sliderCont};
		LvContainer m_valueCont{"value_cont", m_sliderCont}; // The value, and its unit after it if it has one
		LvTextarea m_input{"input", m_valueCont};
		LvLabel m_unit{"unit", m_valueCont}; // The value's unit, e.g. "%"; hidden if it has none

		float m_incrementValue = 1;

		float m_min;
		float m_max;
		float m_value;
		float m_defaultValue = std::numeric_limits<float>::quiet_NaN();
		int m_displayDecimals = 2;
		bool m_focused = false;
		bool m_labelShown = true; // The label shows while it says something; false keeps it hidden
		bool m_longPressEnabled = true;
		SendMode m_sendMode = SendMode::VALUE_CONFIRMED;
		std::optional<float> m_lowest;	// The least the value may be, if not the bottom of the range
		std::optional<float> m_highest; // The most the value may be, if not the top of the range
		value_changed_callback_t m_valueChangedCallback;
		focused_callback_t m_focusedCallback;
	};
} // namespace UI

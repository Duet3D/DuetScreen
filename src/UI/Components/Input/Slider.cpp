/*
 * Slider.cpp
 *
 *  Created on: 2025-07-10
 *      Author: Andy Everitt
 */

#include "Slider.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Slider::Slider(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_label("label", getRoot())
		, m_sliderCont("slider_cont", getRoot())
		, m_decrement("slider_decrement", m_sliderCont, LV_SYMBOL_MINUS)
		, m_slider("slider", m_sliderCont)
		, m_increment("slider_increment", m_sliderCont, LV_SYMBOL_PLUS)
		, m_input("slider_input", m_sliderCont)
		, m_incrementValue(1)
		, m_keyboard(nullptr)
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

		m_label.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_sliderCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_sliderCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_sliderCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		for (size_t i = 0; i < m_sliderCont.getChildCnt(); i++)
		{
			lv_obj_t* child = m_sliderCont.getChild(i);
			lv_obj_set_height(child, LV_SIZE_CONTENT);
			lv_obj_set_style_pad_all(child, 2, 0);
		}

		m_decrement.setWidth(LV_SIZE_CONTENT);
		m_increment.setWidth(LV_SIZE_CONTENT);
		m_input.setWidth(50);
		m_slider.setFlexGrow(1);

		m_decrement.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_event_code_t code = lv_event_get_code(e);
				Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));

				if (code == LV_EVENT_PRESSED || (code == LV_EVENT_LONG_PRESSED_REPEAT && slider->m_longPressEnabled))
				{
					slider->setValue(slider->getValue() - slider->m_incrementValue);
				}
			},
			LV_EVENT_ALL,
			this);

		m_increment.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_event_code_t code = lv_event_get_code(e);
				Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));

				if (code == LV_EVENT_PRESSED || (code == LV_EVENT_LONG_PRESSED_REPEAT && slider->m_longPressEnabled))
				{
					slider->setValue(slider->getValue() + slider->m_incrementValue);
				}
			},
			LV_EVENT_ALL,
			this);

		setRange(0, 100);

		m_input.setOneLine(true);
		m_input.setAcceptedChars("0123456789-.");
		m_input.setMaxLength(4);
		m_input.setCursorClickPos(false);
		m_input.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		updateText();

		m_slider.addEventCallback(onValueChanged, LV_EVENT_ALL, this);
		m_input.addEventCallback(onInputEvent, LV_EVENT_ALL, this);

		m_input.addStyle(Themes::getLvglStyles().input);
	}

	void Slider::setOutOfRangeMode(OutOfRange mode)
	{
		UI_LOCK();
		switch (mode)
		{
		case OutOfRange::NONE:
		case OutOfRange::UPPER:
			m_input.setAcceptedChars(getMin() < 0 ? "-0123456789" : "0123456789");
			break;
		case OutOfRange::LOWER:
		case OutOfRange::BOTH:
			m_input.setAcceptedChars("-0123456789");
			break;
		}
		m_outOfRangeMode = mode;
	}

	void Slider::setLabel(std::string_view text)
	{
		UI_LOCK();
		m_label.setFlag(LV_OBJ_FLAG_HIDDEN, text.empty());
		m_label.setText(text);
	}

	void Slider::setIncrementValue(float value)
	{
		UI_LOCK();
		m_incrementValue = value;
		m_slider.setMaxValue((m_max - m_min) / m_incrementValue);
	}

	void Slider::setRange(float min, float max)
	{
		UI_LOCK();
		m_min = min;
		m_max = max;
		boundValue(m_value);
		m_slider.setMaxValue((m_max - m_min) / m_incrementValue);
		m_slider.setValue(m_value);
	}

	void Slider::setValue(float value)
	{
		UI_LOCK();
		boundValue(value);
		m_value = value;
		m_slider.setValue(normaliseValue(value));

		if (!m_input.hasState(LV_STATE_FOCUSED))
		{
			updateText();
		}

		if (m_valueChangedCallback)
		{
			m_valueChangedCallback(getValue());
		}
	}

	void Slider::onValueChanged(lv_event_t* e)
	{
		UI_LOCK();
		lv_event_code_t code = lv_event_get_code(e);
		Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));

		switch (code)
		{
		case LV_EVENT_FOCUSED:
			slider->m_focused = true;
			break;
		case LV_EVENT_VALUE_CHANGED:
		{
			slider->m_value = (slider->m_slider.getValue() - slider->m_slider.getMinValue()) /
								  static_cast<float>(slider->m_slider.getMaxValue() - slider->m_slider.getMinValue()) *
								  (slider->getMax() - slider->getMin()) +
							  slider->getMin();
			if (slider->boundValue(slider->m_value))
			{
				slider->setValue(slider->m_value);
			}
			if (slider->m_sendMode == SendMode::VALUE_CHANGED && slider->m_valueChangedCallback)
			{
				slider->m_valueChangedCallback(slider->getValue());
			}
			if (!slider->m_input.hasState(LV_STATE_FOCUSED))
			{
				slider->updateText();
			}
			break;
		}
		case LV_EVENT_RELEASED:
			slider->m_focused = false;
			if (slider->m_valueChangedCallback)
			{
				slider->m_valueChangedCallback(slider->getValue());
			}
			slider->updateText();
			break;
		}
	}

	void Slider::onInputEvent(lv_event_t* e)
	{
		UI_LOCK();
		lv_event_code_t code = lv_event_get_code(e);
		Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));
		switch (code)
		{
		case LV_EVENT_PRESSED:
		case LV_EVENT_FOCUSED:
		{
			slider->m_focused = true;
			if (slider->m_keyboard)
			{
				lv_keyboard_set_textarea(slider->m_keyboard, slider->m_input);
				if (slider->m_focusedCallback)
				{
					slider->m_focusedCallback(true);
				}
			}
			break;
		}
		case LV_EVENT_DEFOCUSED:
		{
			slider->m_focused = false;
			if (slider->m_keyboard)
			{
				lv_keyboard_set_textarea(slider->m_keyboard, nullptr);
				if (slider->m_focusedCallback)
				{
					slider->m_focusedCallback(false);
				}
				slider->updateText();
			}
			break;
		}
		case LV_EVENT_READY:
		{
			float value = atof(slider->m_input.getText().data());
			slider->setValue(value);
			break;
		}
		}
	}

	bool Slider::boundValue(float& value)
	{
		UI_LOCK();
		bool outOfRange = false;
		switch (m_outOfRangeMode)
		{
		case OutOfRange::NONE:
			outOfRange = value < getMin() || value > getMax();
			value = std::clamp(value, getMin(), getMax());
			m_decrement.setDisabled(value == getMin());
			m_increment.setDisabled(value == getMax());
			break;
		case OutOfRange::BOTH:
			m_decrement.setDisabled(false);
			m_increment.setDisabled(false);
			break;
		case OutOfRange::UPPER:
			outOfRange = value < getMin();
			value = std::max(value, getMin());
			m_decrement.setDisabled(value == getMin());
			m_increment.setDisabled(false);
			break;
		case OutOfRange::LOWER:
			outOfRange = value > getMax();
			value = std::min(value, getMax());
			m_decrement.setDisabled(false);
			m_increment.setDisabled(value == getMax());
			break;
		}
		return outOfRange;
	}

	int32_t Slider::normaliseValue(float value) const
	{
		if (value < getMin())
		{
			return m_slider.getMinValue();
		}
		else if (value > getMax())
		{
			return m_slider.getMaxValue();
		}
		else
		{
			const int32_t slider_min = m_slider.getMinValue();
			const int32_t slider_max = m_slider.getMaxValue();
			const int32_t slider_range = slider_max - slider_min;
			return static_cast<int32_t>((slider_range * (value - getMin())) / (getMax() - getMin())) + slider_min;
		}
	}

	void Slider::updateText()
	{
		UI_LOCK();
		m_input.setText(fmt::format("{:g}", getValue()));
	}
} // namespace UI

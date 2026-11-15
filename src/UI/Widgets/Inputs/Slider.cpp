/*
 * Slider.cpp
 *
 *  Created on: 2025-07-10
 *      Author: Andy Everitt
 */

#include "Slider.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"

#define BUTTON_SIZE 50

namespace UI
{
	namespace
	{
		/** The gap between the value and its unit */
		constexpr lv_coord_t UNIT_GAP = 4;
	} // namespace

	Slider::Slider(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		init();
	}

	Slider::Slider(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		init();
	}

	void Slider::init()
	{
		ZoneScoped;
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

		setStylePad(0, LV_PART_MAIN, Padding::ROW);

		m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_header.setStylePad(0);

		m_label.hide(); // will be shown if `setLabel()` is called
		m_label.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_reset.setText(_("slider.reset"));
		m_reset.setAlign(LV_ALIGN_RIGHT_MID, 0, 0);
		m_reset.setVisible(false);

		m_sliderCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_sliderCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_sliderCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_decrement.addStyle(Themes::getComponentStyles().slider_buttons); // sets the size
		m_increment.addStyle(Themes::getComponentStyles().slider_buttons); // sets the size
		m_increment.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_decrement.setIcon("decrement.png");
		m_increment.setIcon("increment.png");

		m_valueCont.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_valueCont.setStylePad(0);
		m_valueCont.setStylePad(UNIT_GAP, 0, Padding::COLUMN);
		m_valueCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_valueCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_valueCont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_input.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_input.setMinWidth(50);

		m_unit.addStyle(Themes::getLvglStyles().text_muted);
		m_unit.setText(""); // A label starts as "Text"
		m_unit.hide();

		m_slider.setMinHeight(LV_SIZE_CONTENT);
		m_slider.setFlexGrow(1);
		// m_slider.setStylePad(2);

		m_sliderCont.addEventCallback([this](lv_event_t*) { m_slider.setExtClickArea(m_sliderCont.getHeight() / 4); },
									  LV_EVENT_SIZE_CHANGED);
		m_sliderCont.setStylePad(20, 0, Padding::COLUMN);

		m_decrement.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_event_code_t code = lv_event_get_code(e);
				Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));

				if (code == LV_EVENT_PRESSED || (code == LV_EVENT_LONG_PRESSED_REPEAT && slider->m_longPressEnabled))
				{
					slider->setValue(slider->getValue() - slider->m_incrementValue);
					slider->sendEvent(LV_EVENT_VALUE_CHANGED, &slider->m_value);
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
					slider->sendEvent(LV_EVENT_VALUE_CHANGED, &slider->m_value);
				}
			},
			LV_EVENT_ALL,
			this);

		m_reset.addClickedCallback(
			[](lv_event_t* e)
			{
				Slider& slider = *static_cast<Slider*>(lv_event_get_user_data(e));
				slider.setValue(slider.m_defaultValue);
				slider.sendEvent(LV_EVENT_VALUE_CHANGED, &slider.m_value);
			},
			this);

		setRange(0, 100);
		setLimits(std::nullopt, std::nullopt);

		m_input.setOneLine(true);
		m_input.setCursorClickPos(false);
		m_input.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		updateText();

		m_slider.addEventCallback(onValueChanged, LV_EVENT_ALL, this);
		m_input.addEventCallback(onInputEvent, LV_EVENT_ALL, this);
	}

	void Slider::onShow() {}

	void Slider::setLimits(std::optional<float> lowest, std::optional<float> highest)
	{
		ZoneScoped;
		UI_LOCK();
		m_lowest = lowest;
		m_highest = highest;
		m_input.setAcceptedChars(this->lowest() < 0 ? "-0123456789." : "0123456789.");
		setValue(m_value);
	}

	void Slider::setLabel(std::string_view text)
	{
		ZoneScoped;
		UI_LOCK();
		m_label.setFlag(LV_OBJ_FLAG_HIDDEN, text.empty() || !m_labelShown);
		m_label.setText(text);
	}

	void Slider::setLabelShown(bool shown)
	{
		m_labelShown = shown;
		m_label.setFlag(LV_OBJ_FLAG_HIDDEN, getLabelText().empty() || !m_labelShown);
	}

	void Slider::setIncrementValue(float value)
	{
		ZoneScoped;
		UI_LOCK();
		m_incrementValue = value;
		m_slider.setMaxValue(static_cast<int32_t>((m_max - m_min) / m_incrementValue));
	}

	void Slider::setRange(float min, float max)
	{
		ZoneScoped;
		UI_LOCK();
		m_min = min;
		m_max = max;
		boundValue(m_value);
		m_slider.setMaxValue(static_cast<int32_t>((m_max - m_min) / m_incrementValue));
		setValue(m_value);
	}

	void Slider::setValue(float value)
	{
		ZoneScoped;
		UI_LOCK();
		boundValue(value);
		m_value = value;
		const int32_t normalised = normaliseValue(value);
		m_slider.setValue(normalised);

		if (!m_input.hasState(LV_STATE_FOCUSED))
		{
			updateText();
		}

		if (m_valueChangedCallback && m_sendMode != SendMode::DISABLED)
		{
			m_valueChangedCallback(getValue());
		}

		if (hasDefaultValue())
		{
			m_reset.setVisible(getValue() != m_defaultValue);
		}
	}

	void Slider::setDefaultValue(float value)
	{
		ZoneScoped;
		UI_LOCK();
		m_defaultValue = value;
		m_reset.setVisible(hasDefaultValue() && getValue() != m_defaultValue);
	}

	void Slider::setDisplayDecimals(int decimals)
	{
		ZoneScoped;
		UI_LOCK();
		m_displayDecimals = decimals;
		updateText();
	}

	void Slider::onValueChanged(lv_event_t* e)
	{
		ZoneScoped;
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
			slider->m_value = static_cast<float>(slider->m_slider.getValue() - slider->m_slider.getMinValue()) /
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
			if (slider->hasDefaultValue())
			{
				slider->m_reset.setVisible(slider->getValue() != slider->m_defaultValue);
			}
			slider->sendEvent(LV_EVENT_VALUE_CHANGED, &slider->m_value);
			break;
		}
		case LV_EVENT_RELEASED:
			slider->m_focused = false;
			if (slider->m_valueChangedCallback && slider->m_sendMode != SendMode::DISABLED)
			{
				slider->m_valueChangedCallback(slider->getValue());
			}
			slider->updateText();
			break;
		default:
			break;
		}
	}

	void Slider::onInputEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		lv_event_code_t code = lv_event_get_code(e);
		Slider* slider = static_cast<Slider*>(lv_event_get_user_data(e));
		switch (code)
		{
		case LV_EVENT_CLICKED:
		{
			slider->m_focused = true;
			if (slider->askForValue())
			{
				slider->getInput().sendEvent(LV_EVENT_DEFOCUSED, nullptr); // stop cursor blinking
			}
			if (slider->m_focusedCallback)
			{
				slider->m_focusedCallback(true);
			}
			break;
		}
		case LV_EVENT_DEFOCUSED:
		{
			slider->m_focused = false;
			if (slider->m_focusedCallback)
			{
				slider->m_focusedCallback(false);
			}
			slider->updateText();
			break;
		}
		case LV_EVENT_READY:
		{
			float value = static_cast<float>(atof(slider->m_input.getText().data()));
			slider->setValue(value);
			break;
		}
		case LV_EVENT_VALUE_CHANGED:
		{
			if (!slider->m_input.hasState(LV_STATE_FOCUSED))
			{
				/*
				Ignore changes while input is not focused (ie slider being dragged) to prevent flicker.

				Because the input box has accepted chars set, a `LV_EVENT_VALUE_CHANGED` event is generated for every
				char. This means that when the `Slider::setValue()` udpates the input box text, it generates multiple
				events which should be ignored to prevent flicker.

				ie: If the user types "123", events are generated for '1', '12', and '123'. We only actually want to
				update the slider bar if the input is focused by the user and not when we update the text
				programmatically.
				*/
				break;
			}
			float value = static_cast<float>(atof(slider->m_input.getText().data()));
			if (value > slider->highest())
			{
				slider->m_input.setText(fmt::format("{:g}", slider->highest()));
			}
			slider->boundValue(value);
			slider->m_value = value;
			slider->m_slider.setValue(slider->normaliseValue(value));
			break;
		}
		default:
			break;
		}
	}

	bool Slider::boundValue(float& value)
	{
		ZoneScoped;
		UI_LOCK();
		const float original = value;
		value = std::clamp(value, lowest(), highest());
		m_decrement.setDisabled(value <= lowest());
		m_increment.setDisabled(value >= highest());
		return original != value;
	}

	int32_t Slider::normaliseValue(float value) const
	{
		ZoneScoped;
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
			return static_cast<int32_t>((static_cast<float>(slider_range) * (value - getMin())) /
										(getMax() - getMin())) +
				   slider_min;
		}
	}

	void Slider::setUnit(std::string_view unit)
	{
		ZoneScoped;
		UI_LOCK();
		m_unit.setText(unit);
		m_unit.setVisible(!unit.empty());
	}

	void Slider::updateText()
	{
		ZoneScoped;
		UI_LOCK();
		m_input.setText(utils::formatFloat(getValue(), m_displayDecimals));
	}
} // namespace UI

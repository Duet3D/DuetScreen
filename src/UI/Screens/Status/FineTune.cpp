/*
 * FineTune.cpp
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#include "FineTune.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include <algorithm>

namespace UI
{
	FineTune::FineTune(LvObj& parent, LvObj* numberpad_parent)
		: View("fine_tune", parent, layout_t(0, 0, 100, 100))
		, m_babystep("babystep", getRoot())
		, m_sliderCont("sliders", getRoot())
		, m_speed("speed", m_sliderCont)
		, m_extruders("extruders", m_sliderCont)
		, m_fans("fans", m_sliderCont)
		, m_keyboard("kb", getRoot())
		, m_numberPad("numberpad", numberpad_parent ? *numberpad_parent : parent, layout_t(0, 0, 50, 70))
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);
		m_babystep.addStyle(Themes::getLvglStyles().card);
		m_speed.addStyle(Themes::getLvglStyles().card);
		m_extruders.addStyle(Themes::getLvglStyles().card);
		m_fans.addStyle(Themes::getLvglStyles().card);

		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		for (size_t i = 0; i < lv_obj_get_child_cnt(getRoot()); i++)
		{
			lv_obj_t* child = lv_obj_get_child(getRoot(), i);
			lv_obj_set_height(child, LV_PCT(100));
		}
		m_babystep.setFlexGrow(2);
		m_sliderCont.setFlexGrow(5);
		m_keyboard.setFlexGrow(6);

		m_babystep.setMaxWidth(200);

		m_sliderCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_sliderCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_speed.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_extruders.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_fans.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_extruders.setTitle(_("fine_tune_extruder_header"));
		m_fans.setTitle(_("fine_tune_fan_header"));

		m_keyboard.setMode(LV_KEYBOARD_MODE_NUMBER);
		m_keyboard.hide();
		m_keyboard.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				FineTune* view = static_cast<FineTune*>(lv_event_get_user_data(e));
				view->showKeyboard(false);
			},
			LV_EVENT_CANCEL,
			this);

		m_numberPad.hide();

		m_speed.setLabel(_("fine_tune_speed_factor"));
		m_speed.setKeyboard(m_keyboard);
		m_speed.setFocusedCallback([this](bool focused) { showKeyboard(focused); });
		m_speed.setOutOfRangeMode(Slider::OutOfRange::UPPER);
		m_speed.setRange(1, 200);
		m_speed.setValueChangedCallback([this](int32_t value) { m_presenter->setSpeedFactor(value); });

		m_sliderCont.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				FineTune* view = static_cast<FineTune*>(lv_event_get_user_data(e));
				view->showKeyboard(false);
			},
			LV_EVENT_SCROLL,
			this);
	}

	void FineTune::setSpeedValue(uint32_t value)
	{
		m_speed.setValue(value);
	}

	/**
	 * @brief
	 * @param count
	 */
	void FineTune::setExtruderCount(size_t count)
	{
		m_extruders.setItemCount(count,
								 [this](size_t index, LvObj& parent)
								 {
									 auto slider = std::make_shared<Slider>(fmt::format("{:d}", index), parent);
									 slider->setSize(LV_PCT(100), LV_SIZE_CONTENT);
									 slider->setKeyboard(m_keyboard);
									 slider->setFocusedCallback([this](bool focused) { showKeyboard(focused); });
									 slider->setRange(0, 200);
									 slider->setOutOfRangeMode(Slider::OutOfRange::UPPER);
									 slider->setValueChangedCallback([this, index](int32_t value)
																	 { m_presenter->setExtruderFactor(index, value); });
									 return slider;
								 });
	}

	void FineTune::setFanCount(size_t count)
	{
		m_fans.setItemCount(count,
							[this](size_t index, LvObj& parent)
							{
								auto slider = std::make_shared<Slider>(fmt::format("{:d}", index), parent);

								slider->setSize(LV_PCT(100), LV_SIZE_CONTENT);
								slider->setKeyboard(m_keyboard);
								slider->setFocusedCallback([this](bool focused) { showKeyboard(focused); });
								slider->setValueChangedCallback([this, index](int32_t value)
																{ m_presenter->setFanValue(index, value); });
								return slider;
							});
	}

	void FineTune::setExtruderLabel(size_t index, std::string_view label)
	{
		UI_LOCK();
		auto extruder = m_extruders.getItem(index);
		if (!extruder || extruder->isFocused())
		{
			return;
		}
		extruder->setLabel(label);
	}

	void FineTune::setExtruderValue(size_t index, uint32_t value)
	{
		UI_LOCK();
		auto extruder = m_extruders.getItem(index);
		if (!extruder || extruder->isFocused())
		{
			return;
		}
		extruder->setValue(value);
	}

	void FineTune::setFanLabel(size_t index, std::string_view label)
	{
		UI_LOCK();
		auto fan = m_fans.getItem(index);
		if (!fan || fan->isFocused())
		{
			return;
		}
		fan->setLabel(label);
	}

	void FineTune::setFanValue(size_t index, uint32_t value)
	{
		UI_LOCK();
		auto fan = m_fans.getItem(index);
		if (!fan || fan->isFocused())
		{
			return;
		}

		fan->setValue(value);
	}

	void FineTune::showKeyboard(bool show)
	{
		UI_LOCK();
		lv_obj_set_flag(m_keyboard, LV_OBJ_FLAG_HIDDEN, !show);
	}

	void FineTune::onShow()
	{
		m_babystep.activate();
	}
} // namespace UI

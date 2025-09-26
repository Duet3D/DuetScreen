#include "HeaterSlider.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include <math.h>

namespace UI
{
	HeaterSlider::HeaterSlider(const std::string& name, LvObj& parent)
		: View(name, parent)
		, m_heaterInfoCont("heater_info_cont", getRoot())
		, m_heaterName("heater_name", m_heaterInfoCont)
		, m_heaterState("heater_state", m_heaterInfoCont)
		, m_temperatureCont("temperature_cont", getRoot())
		, m_currentTemperature("current_temperature", m_temperatureCont)
		, m_activeTemperature("active_temperature", m_temperatureCont)
		, m_standbyTemperature("standby_temperature", m_temperatureCont)
	{
		UI_LOCK();

		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		// Set up the heater slider view
		m_heaterName.setText("Heater");
		m_heaterState.setText("State");
		setActiveTemperature(-2000);
		setStandbyTemperature(-2000);
		m_currentTemperature.setRange(m_minTempValue, m_maxTempValue);

		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_heaterInfoCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_heaterInfoCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		m_heaterInfoCont.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_heaterInfoCont.setMinHeight(LV_SIZE_CONTENT);
		m_temperatureCont.setHeight(LV_PCT(100));
		m_temperatureCont.setMinHeight(100);
		m_temperatureCont.setFlexGrow(1);
		m_temperatureCont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_currentTemperature.setSize(LV_PCT(100), 15);
		// lv_coord_t
		m_currentTemperature.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_activeTemperature.setAlign(LV_ALIGN_LEFT_MID, 0, -30);
		m_standbyTemperature.setAlign(LV_ALIGN_LEFT_MID, 0, 31);

		m_activeTemperature.setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		m_standbyTemperature.setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		m_activeTemperature.setExtClickArea(20);
		m_standbyTemperature.setExtClickArea(20);
		lv_obj_move_foreground(m_activeTemperature);
		lv_obj_move_foreground(m_standbyTemperature);

		m_heaterName.addClickedCallback(onToggleStateEvent, this);

		m_activeTemperature.setUserData(this);
		m_standbyTemperature.setUserData(this);
		m_activeTemperature.addEventCallback(onTemperatureLabelEvent, LV_EVENT_ALL, &m_activeTemperature);
		m_standbyTemperature.addEventCallback(onTemperatureLabelEvent, LV_EVENT_ALL, &m_standbyTemperature);
		m_currentTemperature.addEventCallback(drawCurrentTemperatureEvent, LV_EVENT_DRAW_MAIN_END, this);

		// Add styles
		m_activeTemperature.addStyle(Themes::getLvglStyles().pad_normal);
		m_standbyTemperature.addStyle(Themes::getLvglStyles().pad_normal);
		m_activeTemperature.addStyle(Themes::getLvglStyles().input);
		m_standbyTemperature.addStyle(Themes::getLvglStyles().input);

		m_currentTemperature.addStyle(Themes::getComponentStyles().temperature_bar, LV_PART_INDICATOR);
		m_activeTemperature.addStyle(Themes::getComponentStyles().temperature_bar_indic, LV_PART_INDICATOR);
		m_standbyTemperature.addStyle(Themes::getComponentStyles().temperature_bar_indic, LV_PART_INDICATOR);
		// m_activeTemperature.addStyle(Themes::getLvglStyles().btn_checked, LV_STATE_CHECKED);
		// m_standbyTemperature.addStyle(Themes::getLvglStyles().btn_checked, LV_STATE_CHECKED);
	}

	void HeaterSlider::setHeaterName(const std::string_view name)
	{
		m_heaterName.setText(name);
	}

	void HeaterSlider::setHeaterState(HeaterSliderPresenter::heater_state_t state, const std::string_view str)
	{
		m_heaterState.setText(str);
		m_activeTemperature.setState(LV_STATE_CHECKED, state == HeaterSliderPresenter::heater_state_t::active);
		m_standbyTemperature.setState(LV_STATE_CHECKED, state == HeaterSliderPresenter::heater_state_t::standby);
	}

	void HeaterSlider::setHeaterMinTemperature(float temperature)
	{
		UI_LOCK();
		temperature = std::max(0.0f, temperature);
		// if (temperature == m_minTempValue)
		// 	return;

		LOG_DBG("Setting min temperature to {:g} °C", temperature);
		m_currentTemperature.setMinValue(m_minTempValue);
		updateLabelPositions();
	}

	void HeaterSlider::setHeaterMaxTemperature(float temperature)
	{
		UI_LOCK();
		if (temperature == m_maxTempValue)
			return;

		LOG_DBG("Setting max temperature to {:g} °C", temperature);
		m_maxTempValue = temperature;
		m_currentTemperature.setMaxValue(temperature);
		updateLabelPositions();
	}

	void HeaterSlider::setCurrentTemperature(float temperature)
	{
		UI_LOCK();
		if (temperature == m_currentTempValue)
			return;

		LOG_DBG("Setting current temperature to {:g} °C", temperature);
		m_currentTempValue = temperature;
		m_currentTemperature.setValue(temperature);
	}

	void HeaterSlider::setActiveTemperature(int32_t temperature, bool dragging)
	{
		if (temperature == m_activeTempValue)
			return;

		if (m_activeTemperature.hasState(LV_STATE_PRESSED) && !dragging)
			return;

		LOG_DBG("Setting active temperature to {:d} °C", temperature);
		m_activeTempValue = temperature;
		m_activeTemperature.setText(fmt::format("{:d} °C", temperature));
		updateLabelPosition(m_activeTemperature, m_activeTempValue);
	}

	void HeaterSlider::setStandbyTemperature(int32_t temperature, bool dragging)
	{
		if (temperature == m_standbyTempValue)
			return;

		if (m_standbyTemperature.hasState(LV_STATE_PRESSED) && !dragging)
			return;

		LOG_DBG("Setting standby temperature to {:d} °C", temperature);
		m_standbyTempValue = temperature;
		m_standbyTemperature.setText(fmt::format("{:d} °C", temperature));
		updateLabelPosition(m_standbyTemperature, m_standbyTempValue);
	}

	void HeaterSlider::setNumberPad(NumberPad* numberPad)
	{
		UI_LOCK();
		m_numberPad = numberPad;
	}

	void HeaterSlider::drawCurrentTemperatureEvent(lv_event_t* e)
	{
		HeaterSlider& slider = *(HeaterSlider*)lv_event_get_user_data(e);

		lv_draw_label_dsc_t label_dsc;
		lv_draw_label_dsc_init(&label_dsc);
		label_dsc.font = LV_FONT_DEFAULT;

		char buf[8];
		snprintf(buf, sizeof(buf), "%.1f", slider.m_currentTempValue);

		lv_point_t txt_size;
		lv_text_attributes_t txt_attr;
		lv_text_attributes_init(&txt_attr);
		txt_attr.letter_space = label_dsc.letter_space;
		txt_attr.line_space = label_dsc.line_space;
		txt_attr.text_flags = label_dsc.flag;
		txt_attr.max_width = LV_COORD_MAX;

		lv_text_get_size(&txt_size, buf, label_dsc.font, &txt_attr);

		lv_area_t txt_area;
		txt_area.x1 = 0;
		txt_area.x2 = txt_size.x - 1;
		txt_area.y1 = 0;
		txt_area.y2 = txt_size.y - 1;

		lv_area_t indic_area;
		lv_obj_get_coords(slider.m_currentTemperature, &indic_area);
		lv_area_set_width(&indic_area,
						  lv_area_get_width(&indic_area) * slider.m_currentTempValue /
							  (slider.m_maxTempValue - slider.m_minTempValue));

		/*If the indicator is long enough put the text inside on the right*/
		if (lv_area_get_width(&indic_area) > txt_size.x + 20)
		{
			lv_area_align(&indic_area, &txt_area, LV_ALIGN_RIGHT_MID, -10, 0);
			label_dsc.color = lv_color_white();
		}
		/*If the indicator is still short put the text out of it on the right*/
		else
		{
			lv_area_align(&indic_area, &txt_area, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
			label_dsc.color = lv_color_white();
		}
		label_dsc.text = buf;
		label_dsc.text_local = true;
		lv_layer_t* layer = lv_event_get_layer(e);
		lv_draw_label(layer, &label_dsc, &txt_area);
	}

	void HeaterSlider::onTemperatureLabelEvent(lv_event_t* e)
	{
		lv_event_code_t code = lv_event_get_code(e);
		if (code == LV_EVENT_DELETE)
			return; // Ignore delete events

		LvLabel& label = *(LvLabel*)lv_event_get_user_data(e);
		HeaterSlider& control = *(HeaterSlider*)label.getUserData();

		bool activeTemperature;
		if (label == control.m_activeTemperature)
		{
			activeTemperature = true;
		}
		else if (label == control.m_standbyTemperature)
		{
			activeTemperature = false;
		}
		else
		{
			LOG_ERROR("Unexpected label in HeaterSlider event handler");
			return; // Not a temperature label
		}
		int32_t temperature = activeTemperature ? control.m_activeTempValue : control.m_standbyTempValue;

		switch (code)
		{
		case LV_EVENT_SHORT_CLICKED:
		{
			// Open numberpad
			LOG_DBG("Label '{}' clicked", label.getName());
			break;
		}
		case LV_EVENT_PRESSED:
		{
			lv_indev_get_point(lv_indev_active(), &control.m_pressedPoint);
			lv_obj_transform_point(label, &control.m_pressedPoint, LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE);
			float pct =
				(float)(temperature - control.m_minTempValue) / (control.m_maxTempValue - control.m_minTempValue);

			control.m_pressedPointOffset.x = control.m_pressedPoint.x - label.getCoords().x1 - label.getWidth() * pct;
			break;
		}
		case LV_EVENT_PRESSING:
		{
			// Update the target temperature based on the slider position
			lv_indev_t* indev = lv_indev_active();
			if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER)
				break;
			if (lv_indev_get_scroll_obj(indev) != NULL)
				break;

			lv_point_t p;
			lv_indev_get_point(indev, &p);
			lv_obj_transform_point(label, &p, LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE);

			const int32_t range = control.m_maxTempValue - control.m_minTempValue;
			const int32_t w = control.m_currentTemperature.getWidth();
			const int32_t rel_position =
				p.x - control.m_currentTemperature.getCoords().x1 - control.m_pressedPointOffset.x;
			int32_t new_temperature =
				std::clamp((int32_t)(((range * rel_position + w / 2) / w)) + control.m_minTempValue,
						   control.m_minTempValue,
						   control.m_maxTempValue);

			if (activeTemperature)
				control.setActiveTemperature(new_temperature, true);
			else
				control.setStandbyTemperature(new_temperature, true);
			break;
		}
		case LV_EVENT_RELEASED:
		case LV_EVENT_PRESS_LOST:
		{
			LOG_DBG("Label '{}' released", label.getName());
			// Set new target temperature
			lv_point_t p;
			lv_indev_get_point(lv_indev_active(), &p);
			lv_obj_transform_point(label, &p, LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE);

			if (abs(p.x - control.m_pressedPoint.x) < 2)
			{
				if (control.m_numberPad == nullptr)
				{
					LOG_ERROR("NumberPad is not set for {}", control.getName());
					break;
				}
				control.m_numberPad->setValue(temperature);
				control.m_numberPad->setMinValue(control.m_minTempValue);
				control.m_numberPad->setMaxValue(control.m_maxTempValue);
				control.m_numberPad->setHeader(fmt::format(fmt::runtime(_("set_temperature_numpad_header")),
														   activeTemperature ? _("active") : _("standby"),
														   control.m_heaterName.getLabel().getText()));
				control.m_numberPad->setConfirmCallback(
					[&control, activeTemperature](float value)
					{ control.getPresenter()->sendTemperature(value, activeTemperature); }

				);

				openModal(control.m_numberPad);
			}
			else
			{
				control.getPresenter()->sendTemperature(temperature, activeTemperature);
			}
			break;
		}
		case LV_EVENT_REFR_EXT_DRAW_SIZE:
		{
			int32_t size =
				activeTemperature
					? control.m_currentTemperature.getCoords().y1 - control.m_activeTemperature.getCoords().y2
					: control.m_standbyTemperature.getCoords().y1 - control.m_currentTemperature.getCoords().y2;
			lv_event_set_ext_draw_size(e, size);
			break;
		}
		case LV_EVENT_DRAW_MAIN:
		{
#if 1
			lv_layer_t* layer = lv_event_get_layer(e);
			lv_area_t marker_area;
			static int32_t marker_width = 21;

			lv_draw_triangle_dsc_t marker_dsc;
			lv_draw_triangle_dsc_init(&marker_dsc);
			marker_dsc.base.layer = layer;
			marker_dsc.color = lv_obj_get_style_bg_color(label, LV_PART_INDICATOR);
			marker_dsc.opa = lv_obj_get_style_bg_opa(label, LV_PART_INDICATOR);

			lv_area_t label_area = label.getCoords();
			lv_coord_t label_width = label.getWidth();
			const int32_t range = control.m_maxTempValue - control.m_minTempValue;

			int32_t pct =
				range > 0 ? std::clamp(100 * static_cast<int32_t>(temperature - control.m_minTempValue) / range, 0, 100)
						  : 0;

			marker_area.x1 = label_area.x1 + label_width * pct / 100 - marker_width / 2;
			marker_area.x2 = marker_area.x1 + marker_width - 1;
			const lv_coord_t marker_pos_x = label_area.x1 + label_width * pct / 100;
			const lv_area_t bar_area = control.m_currentTemperature.getCoords();
			const int32_t label_radius = lv_obj_get_style_radius(label, LV_PART_MAIN);

			marker_dsc.p[0].x = marker_pos_x;
			marker_dsc.p[1].x = marker_pos_x + marker_width / 2;
			marker_dsc.p[2].x = marker_pos_x - marker_width / 2;

			if (marker_dsc.p[1].x > label_area.x2 - label_radius)
			{
				const int32_t diff = marker_dsc.p[1].x - (label_area.x2 - label_radius);
				marker_dsc.p[1].x -= diff;
				marker_dsc.p[2].x -= diff;
			}
			if (marker_dsc.p[2].x < label_area.x1 + label_radius)
			{
				const int32_t diff = label_area.x1 + label_radius - marker_dsc.p[2].x;
				marker_dsc.p[1].x += diff;
				marker_dsc.p[2].x += diff;
			}

			lv_coord_t marker_y1 = 0;
			lv_coord_t marker_y2 = 0;
			if (label == control.m_activeTemperature)
			{
				marker_y1 = bar_area.y1;
				marker_y2 = label_area.y2;
			}
			else if (label == control.m_standbyTemperature)
			{
				marker_y1 = bar_area.y2;
				marker_y2 = label_area.y1;
			}
			marker_dsc.p[0].y = marker_y1;
			marker_dsc.p[1].y = marker_y2;
			marker_dsc.p[2].y = marker_y2;

			lv_draw_triangle(layer, &marker_dsc);
#endif
			break;
		}
		}
	}

	void HeaterSlider::onToggleStateEvent(lv_event_t* e)
	{
		HeaterSlider& control = *(HeaterSlider*)lv_event_get_user_data(e);
		control.getPresenter()->cycleHeaterState();
	}

	void HeaterSlider::onShow()
	{
		updateLabelPositions();
	}

	void HeaterSlider::updateLabelPositions()
	{
		updateLabelPosition(m_activeTemperature, m_activeTempValue);
		updateLabelPosition(m_standbyTemperature, m_standbyTempValue);
	}

	void HeaterSlider::updateLabelPosition(LvObj& label, int32_t value)
	{
		LOG_DBG("Updating label '{}' position for value: {:d}", label.getName(), value);
		// Calculate the position based on the current temperature value
		const int32_t range = m_maxTempValue - m_minTempValue;
		if (range <= 0)
		{
			LOG_DBG("Invalid temperature range: min = {:g}, max = {:g}", m_minTempValue, m_maxTempValue);
			return;
		}
		lv_coord_t percentage = std::clamp(100 * (value - (int32_t)m_minTempValue) / range, 0, 100);

		label.updateLayout();
		lv_coord_t label_width = label.getWidth();
		lv_coord_t bar_width = m_currentTemperature.getWidth();

		lv_coord_t label_offset_pct = percentage * label_width / bar_width;

		label.setX(LV_PCT(percentage - label_offset_pct));
	}
} // namespace UI

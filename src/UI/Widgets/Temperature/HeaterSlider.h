#pragma once

#include "HeaterSliderPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/LVGL/LvBar.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Core/View.h"

namespace UI
{
	class HeaterSlider : public View<HeaterSliderPresenter>
	{
	  public:
		HeaterSlider(const std::string& name, LvObj& parent);

		void setHeaterName(const std::string_view name);
		void setHeaterState(HeaterSliderPresenter::heater_state_t state, const std::string_view str);
		void setHeaterMinTemperature(float temperature);
		void setHeaterMaxTemperature(float temperature);
		void setCurrentTemperature(float temperature);
		void setActiveTemperature(int32_t temperature, bool dragging = false);
		void setStandbyTemperature(int32_t temperature, bool dragging = false);

		void setNumberPad(NumberPad* numberPad);

	  private:
		static void drawCurrentTemperatureEvent(lv_event_t* e);
		static void onTemperatureLabelEvent(lv_event_t* e);
		static void onToggleStateEvent(lv_event_t* e);

		virtual void onShow() override;

		void updateLabelPositions();
		void updateLabelPosition(LvObj& label, int32_t value);

		LvContainer m_heaterInfoCont;
		Button m_heaterName;
		LvLabel m_heaterState;
		LvContainer m_temperatureCont;
		LvBar m_currentTemperature;
		Button m_activeTemperature;
		Button m_standbyTemperature;

		lv_point_t m_pressedPoint;
		lv_point_t m_pressedPointOffset;

		float m_currentTempValue = 0.0f;
		int32_t m_activeTempValue = 0;
		int32_t m_standbyTempValue = 0;
		float m_minTempValue = 0.0f;
		float m_maxTempValue = 0.0f;

		NumberPad* m_numberPad = nullptr;
	};
} // namespace UI
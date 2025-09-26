/*
 * XYControl.h
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
	class XYControl : public LvObj
	{
		using jog_cb_t = std::function<void(char axis_letter, bool forward)>;
		using home_cb_t = std::function<void()>;
		using label_cb_t = std::function<void(float position)>;

	  public:
		XYControl(const std::string& name, LvObj& parent);

		void setXPosition(float position);
		void setYPosition(float position);
		void setXHomed(bool homed);
		void setYHomed(bool homed);

		void setXDisabled(bool disabled);
		void setYDisabled(bool disabled);
		void setXJogDisabled(bool disabled);
		void setYJogDisabled(bool disabled);
		void setXHomeDisabled(bool disabled);
		void setYHomeDisabled(bool disabled);

		void setJogCallback(jog_cb_t cb);
		void setHomeXYCallback(home_cb_t cb);
		void setHomeXCallback(home_cb_t cb);
		void setHomeYCallback(home_cb_t cb);

		void setXLabelCallback(label_cb_t cb);
		void setYLabelCallback(label_cb_t cb);

	  private:
		static void onJogBtn(lv_event_t* event);
		static void onHomeBtn(lv_event_t* event);
		static void onLabelEvent(lv_event_t* event);

		void updateXLabel();
		void updateYLabel();
		void updateLabel(LvLabel& label, const char axisLetter, const float position);

		int32_t m_colDsc[5] = {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
		int32_t m_rowDsc[5] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

		LvLabel m_xLabel;
		LvLabel m_yLabel;
		Button m_xIncrementButton;
		Button m_xDecrementButton;
		Button m_yIncrementButton;
		Button m_yDecrementButton;
		Button m_homeXYButton;
		Button m_homeXButton;
		Button m_homeYButton;

		static const char sm_xAxisLetter;
		static const char sm_yAxisLetter;

		float m_xPosition = 0.0f;
		float m_yPosition = 0.0f;

		jog_cb_t m_jogCallback;

		home_cb_t m_homeXYCallback;
		home_cb_t m_homeXCallback;
		home_cb_t m_homeYCallback;

		label_cb_t m_xLabelCallback;
		label_cb_t m_yLabelCallback;
	};
} // namespace UI

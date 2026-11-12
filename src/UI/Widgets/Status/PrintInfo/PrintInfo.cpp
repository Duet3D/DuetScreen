/*
 * StatusSubViews.cpp
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#include "PrintInfo.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "i18n/i18n.h"

namespace UI
{
	PrintInfo::PrintInfo(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
		UI_LOCK();

		static int32_t printInfoColDsc[] = {LV_GRID_FR(3), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		static int32_t printInfoRowDsc[] = {
			LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		setGridDsc(printInfoColDsc, printInfoRowDsc);
		setGridCell(m_positions, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_speedCont, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 1, 1);
		setGridCell(m_flowCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 2, 1);
		setGridCell(m_timeCont, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_END, 3, 1);
		setGridCell(m_babyStepCont, LV_GRID_ALIGN_END, 1, 1, LV_GRID_ALIGN_START, 2, 1);

		/* Positions */
		m_positions.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_positions.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		// m_positions.setListGrow(1);
		m_positions.setTitle(_("status.positions"));

		/* Speed */
		static int32_t speedColDsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static int32_t speedRowDsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		m_speedCont.setHeight(LV_SIZE_CONTENT);
		m_speedCont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		m_speedCont.setGridDsc(speedColDsc, speedRowDsc);
		m_speedCont.setGridCell(m_speedHeader, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		m_speedCont.setGridCell(m_speedMultiplier, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
		m_speedCont.setGridCell(m_currentSpeed, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		m_speedCont.setGridCell(m_requestedSpeed, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		// m_speedCont.setGridCell(m_speedLabel, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);

		m_speedHeader.setText(_("status.speed_header"));
		m_speedHeader.addStyle(Themes::getLvglStyles().bg_color_header);
		m_speedHeader.addStyle(Themes::getLvglStyles().pad_normal);
		m_speedHeader.addStyle(Themes::getLvglStyles().text_emphasis);
		m_speedMultiplier.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_currentSpeed.setHeight(LV_SIZE_CONTENT);
		// m_currentSpeed.setLabelFormat("{} mm/s");
		m_currentSpeed.setStyleBgOpa(LV_OPA_0);

		m_requestedSpeed.setHeight(LV_SIZE_CONTENT);
		// m_requestedSpeed.setLabelFormat("{} mm/s");
		m_requestedSpeed.addStyle(Themes::getLvglStyles().bg_color_primary_muted, LV_PART_INDICATOR);
		m_requestedSpeed.addStyle(Themes::getLvglStyles().anim_fast);

		m_speedLabel.setAlign(LV_ALIGN_LEFT_MID, 10, 0);
		m_speedLabel.addStyle(Themes::getComponentStyles().bar_label_bg);
		m_speedLabel.addStyle(Themes::getComponentStyles().bar_label);

		m_speedFactorModal.setSize(LV_PCT(80), LV_SIZE_CONTENT);
		updateSpeed(0, 0);
		updateSpeedMultiplier(100);
		m_speedMultiplier.addClickedCallback([this](lv_event_t*) { openModal(&m_speedFactorModal); });

		/* Flow */
		static int32_t flowColDsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static int32_t flowRowDsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		m_flowCont.setHeight(LV_SIZE_CONTENT);
		m_flowCont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		m_flowCont.setGridDsc(flowColDsc, flowRowDsc);
		m_flowCont.setGridCell(m_flowHeader, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		m_flowCont.setGridCell(m_flowMultiplier, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
		m_flowCont.setGridCell(m_extruderFlow, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		m_flowHeader.setText(_("status.flow_header"));
		m_flowHeader.addStyle(Themes::getLvglStyles().bg_color_header);
		m_flowHeader.addStyle(Themes::getLvglStyles().pad_normal);
		m_flowHeader.addStyle(Themes::getLvglStyles().text_emphasis);
		m_flowMultiplier.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_extruderFlow.setHeight(LV_SIZE_CONTENT);

		m_extruderFlowLabel.setAlign(LV_ALIGN_LEFT_MID, 10, 0);
		m_extruderFlowLabel.addStyle(Themes::getComponentStyles().bar_label_bg);
		m_extruderFlowLabel.addStyle(Themes::getComponentStyles().bar_label);

		m_extrusionFactorModal.setSize(LV_PCT(80), LV_SIZE_CONTENT);
		updateFlowMultiplier(100);
		m_flowMultiplier.addClickedCallback([this](lv_event_t*) { openModal(&m_extrusionFactorModal); });

		/* Babystep */
		m_babyStepCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_babyStepCont.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_babyStepHeader.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_babyStepHeader.setMinWidth(LV_SIZE_CONTENT);
		m_babyStepHeader.setText(_("status.babystep_header"));
		m_babyStepHeader.addStyle(Themes::getLvglStyles().bg_color_header);
		m_babyStepHeader.addStyle(Themes::getLvglStyles().pad_normal);
		m_babyStepHeader.addStyle(Themes::getLvglStyles().text_emphasis);

		m_babyStepButton.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_babyStepButton.setMinWidth(LV_SIZE_CONTENT);
		m_babyStepButton.setText(_("status.babystep_value", 0.0f));
		m_babyStepButton.addClickedCallback([this](lv_event_t*) { openModal(&m_babyStepModal); });

		m_babyStepModal.setSize(LV_PCT(40), LV_PCT(70));

		/* Time */
		m_timeCont.setHeight(LV_SIZE_CONTENT);
		m_timeCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_timeHeader.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_timeHeader.setMinWidth(LV_SIZE_CONTENT);
		m_timeHeader.setText(_("status.time_header"));
		m_timeHeader.addStyle(Themes::getLvglStyles().bg_color_header);
		m_timeHeader.addStyle(Themes::getLvglStyles().pad_normal);
		m_timeHeader.addStyle(Themes::getLvglStyles().text_emphasis);

		updateLayout();
	}

	void PrintInfo::openSubView(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		LvObj* view = static_cast<LvObj*>(lv_event_get_user_data(e));
		openScreen(view, false);
	}

	void PrintInfo::onInit()
	{
		ZoneScoped;
		// Not in the constructor: the modal host may not exist yet
		if (LvObj* host = getModalHost())
		{
			m_speedFactorModal.setParent(*host);
			m_extrusionFactorModal.setParent(*host);
			m_babyStepModal.setParent(*host);
		}
	}

	void PrintInfo::onShow() {}

	void PrintInfo::onHide() {}

	void PrintInfo::setAxisCount(size_t count)
	{
		ZoneScoped;
		m_positions.setItemCount(count,
								 [this](size_t index, LvObj& parent)
								 {
									 auto btn = std::make_unique<LvLabel>(fmt::format("axis_{}", index), parent);
									 btn->setHeight(LV_SIZE_CONTENT);
									 btn->setFlexGrow(1);
									 btn->setMinWidth(LV_SIZE_CONTENT);
									 btn->setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
									 btn->addStyle(Themes::getLvglStyles().bg_light);
									 return btn;
								 });
	}

	void PrintInfo::setPosition(size_t index, char axis_letter, float value)
	{
		ZoneScoped;
		UI_LOCK();
		if (index >= m_positions.getItemCount())
		{
			LOG_ERROR("Index out of bounds for position list");
			return;
		}
		if (auto item = m_positions.getItem(index))
		{
			item->setText(fmt::format("{}\n{:.2f}", axis_letter, value));
		}
		else
		{
			LOG_ERROR("Failed to set position for index {}", index);
		}
	}

	void PrintInfo::setMaxSpeed(int32_t max_speed)
	{
		ZoneScoped;
		m_currentSpeed.setMaxValue(max_speed);
		m_requestedSpeed.setMaxValue(max_speed);
	}

	void PrintInfo::setMaxExtrusionRate(int32_t max_extrusion_rate)
	{
		ZoneScoped;
		m_extruderFlow.setMaxValue(max_extrusion_rate);
	}

	void PrintInfo::updateExtrusionRate(float /* feedrate */, float volumetric)
	{
		ZoneScoped;
		UI_LOCK();
		m_extruderFlow.setValue(static_cast<int32_t>(std::round(volumetric)));
		m_extruderFlowLabel.setText(_("status.flow_rate", volumetric));
	}

	void PrintInfo::updateSpeed(float topSpeed, float requestedSpeed)
	{
		ZoneScoped;
		m_currentSpeed.setValue(static_cast<int32_t>(std::round(topSpeed)), LV_ANIM_ON);
		m_requestedSpeed.setValue(static_cast<int32_t>(std::round(requestedSpeed)), LV_ANIM_ON);

		m_speedLabel.setText(_("status.speed_label", topSpeed, requestedSpeed));
	}

	void PrintInfo::updateFlowMultiplier(uint32_t multiplier)
	{
		ZoneScoped;
		m_flowMultiplier.setText(_("status.flow_multiplier", multiplier));
	}

	void PrintInfo::updateSpeedMultiplier(uint32_t multiplier)
	{
		ZoneScoped;
		m_speedMultiplier.setText(_("status.speed_multiplier", multiplier));
	}

	void PrintInfo::updateElapsedTime(uint32_t elapsed)
	{
		ZoneScoped;
		int32_t hours = elapsed / 3600;
		int32_t minutes = (elapsed % 3600) / 60;
		int32_t seconds = elapsed % 60;
		std::string elapsedStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_elapsedTime.setText(_("status.elapsed_time", elapsedStr));
	}

	void PrintInfo::updateRemainingTime(uint32_t remaining)
	{
		ZoneScoped;
		UI_LOCK();
		int32_t hours = remaining / 3600;
		int32_t minutes = (remaining % 3600) / 60;
		int32_t seconds = remaining % 60;
		std::string remainingStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_remainingTime.setText(_("status.remaining_time", remainingStr));
	}

	void PrintInfo::updateBabyStep(float babystep)
	{
		ZoneScoped;
		UI_LOCK();
		m_babyStepButton.setText(_("status.babystep_value", babystep));
	}

	void PrintInfo::setNumberPad(ModalNumberPad* numberPad)
	{
		ZoneScoped;
		m_babyStepModal.setNumberPad(numberPad);
	}
} // namespace UI

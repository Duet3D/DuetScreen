/*
 * StatusSubViews.cpp
 *
 *  Created on: 2025-02-27
 *      Author: Andy Everitt
 */

#include "PrintInfo.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "i18n/i18n.h"

namespace UI
{
	PrintInfo::PrintInfo(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();

		static int32_t printInfoColDsc[] = {LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
		static int32_t printInfoRowDsc[] = {
			LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		setGridDsc(printInfoColDsc, printInfoRowDsc);
		setGridCell(m_positions, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_speedCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_flowCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
		setGridCell(m_timeCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_END, 3, 1);
		setGridCell(m_babyStep, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 3);

		/* Positions */
		m_positions.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_positions.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		// m_positions.setListGrow(1);
		m_positions.setTitle(_("status.positions"));

		/* Speed */
		m_speedCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_speedCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		m_speedHeader.setText(_("status.speed_header"));
		m_speedHeader.hide();
		m_speedFactorModal.setSize(LV_PCT(70), LV_SIZE_CONTENT);
		updateSpeed(0, 0);
		updateSpeedMultiplier(100);
		m_speedMultiplier.addClickedCallback(
			[](lv_event_t* e)
			{
				auto& printInfo = *static_cast<PrintInfo*>(lv_event_get_user_data(e));
				printInfo.m_presenter->openSpeedFactorModal();
			},
			this);

		/* Flow */
		m_flowCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_flowHeader.setText(_("status.flow_header"));
		m_flowHeader.hide();
		m_extrusionFactorModal.setSize(LV_PCT(80), LV_PCT(70));
		updateFlowMultiplier(100);
		m_flowMultiplier.addClickedCallback(
			[](lv_event_t* e)
			{
				auto& printInfo = *static_cast<PrintInfo*>(lv_event_get_user_data(e));
				openModal(&printInfo.m_extrusionFactorModal);
			},
			this);

		/* Time */
		m_timeCont.setHeight(LV_SIZE_CONTENT);
		m_timeCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);

		/* Babystep */
		m_babyStep.setStylePad(0);
	}

	void PrintInfo::openSubView(lv_event_t* e)
	{
		UI_LOCK();
		LvObj* view = static_cast<LvObj*>(lv_event_get_user_data(e));
		openScreen(view, false);
	}

	void PrintInfo::onShow()
	{
		if (!m_initialised)
		{
			// Can't put this in the constructor as it would cause `HomeView::instance()` to be called within itself
			m_speedFactorModal.setParent(HomeView::instance().getMainWindow());
			m_extrusionFactorModal.setParent(HomeView::instance().getMainWindow());
			m_initialised = true;
		}
	}

	void PrintInfo::onHide() {}

	void PrintInfo::setAxisCount(size_t count)
	{
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

	void PrintInfo::updateExtrusionRate(float feedrate, float volumetric)
	{
		UI_LOCK();
		m_extruderFeedrate.setText(_("status.extrusion_speed", feedrate));
		m_flowRate.setText(_("status.flow_rate", volumetric));
	}

	void PrintInfo::updateSpeed(float topSpeed, float requestedSpeed)
	{
		m_currentSpeed.setText(_("status.current_speed", topSpeed));
		m_requestedSpeed.setText(_("status.requested_speed", requestedSpeed));
	}

	void PrintInfo::updateFlowMultiplier(uint32_t multiplier)
	{
		m_flowMultiplier.setText(_("status.flow_multiplier", multiplier));
	}

	void PrintInfo::updateSpeedMultiplier(uint32_t multiplier)
	{
		m_speedMultiplier.setText(_("status.speed_multiplier", multiplier));
	}

	void PrintInfo::updateElapsedTime(uint32_t elapsed)
	{
		int32_t hours = elapsed / 3600;
		int32_t minutes = (elapsed % 3600) / 60;
		int32_t seconds = elapsed % 60;
		std::string elapsedStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_elapsedTime.setText(_("status.elapsed_time", elapsedStr));
	}

	void PrintInfo::updateRemainingTime(uint32_t remaining)
	{
		UI_LOCK();
		int32_t hours = remaining / 3600;
		int32_t minutes = (remaining % 3600) / 60;
		int32_t seconds = remaining % 60;
		std::string remainingStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_remainingTime.setText(_("status.remaining_time", remainingStr));
	}
} // namespace UI

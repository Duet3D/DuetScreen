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
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	PrintInfo::PrintInfo(const std::string& name, LvObj& parent)
		: LvContainer(name, parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();

		static int32_t printInfoColDsc[] = {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
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
		m_positions.setTitle(_("status_positions"));

		/* Speed */
		m_speedCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_speedHeader.setText(_("status_speed_header"));
		updateSpeed(0, 0);
		updateSpeedMultiplier(100);

		/* Flow */
		m_flowCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		updateFlowMultiplier(100);

		/* Time */
		m_timeCont.setHeight(LV_SIZE_CONTENT);
		m_timeCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_speedInfo.setSize(LV_PCT(100), LV_PCT(100));
		m_speedInfo.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_speedInfo.hide();

		m_speedInfo.addStyle(Themes::getLvglStyles().card, 0);
		m_speedInfo.addStyle(Themes::getLvglStyles().no_border, 0);
	}

	bool PrintInfo::back()
	{
		UI_LOCK();
		for (LvObj* subView : {&m_speedInfo})
		{
			if (subView->isVisible())
			{
				subView->hide();
				return true;
			}
		}
		return false;
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
			m_speedMultiplier.addEventCallback(openSubView, LV_EVENT_CLICKED, &HomeView::instance().getFineTuneView());
			m_flowMultiplier.addEventCallback(openSubView, LV_EVENT_CLICKED, &HomeView::instance().getFineTuneView());
			m_initialised = true;
		}

		m_babyStep.activate();
	}

	void PrintInfo::onHide()
	{
		m_babyStep.deactivate();
	}

	void PrintInfo::setAxisCount(size_t count)
	{
		m_positions.setItemCount(count,
								 [this](size_t index, LvObj& parent)
								 {
									 auto btn = std::make_shared<Button>(fmt::format("axis_{}", index), parent);
									 btn->setHeight(LV_SIZE_CONTENT);
									 btn->setFlexGrow(1);
									 btn->setMinWidth(LV_SIZE_CONTENT);
									 btn->addEventCallback(
										 openSubView, LV_EVENT_CLICKED, &HomeView::instance().getMoveView());
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
		auto item = m_positions.getItem(index);
		if (item)
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
		m_flowRate.setText(fmt::format(fmt::runtime(_("status_extrusion_speed")), feedrate));
		m_flowRate.setText(fmt::format(fmt::runtime(_("status_flow_rate")), volumetric));
	}

	void PrintInfo::updateSpeed(float topSpeed, float requestedSpeed)
	{
		m_currentSpeed.setText(fmt::format(fmt::runtime(_("status_current_speed")), topSpeed));
		m_requestedSpeed.setText(fmt::format(fmt::runtime(_("status_top_speed")), requestedSpeed));
		m_speedInfo.updateSpeed(topSpeed, requestedSpeed);
	}

	void PrintInfo::updateFlowMultiplier(uint32_t multiplier)
	{
		m_flowMultiplier.setText(fmt::format(fmt::runtime(_("status_flow_multiplier")), multiplier));
	}

	void PrintInfo::updateSpeedMultiplier(uint32_t multiplier)
	{
		m_speedMultiplier.setText(fmt::format(fmt::runtime(_("status_speed_multiplier")), multiplier));
		m_speedInfo.updateSpeedMultiplier(multiplier);
	}

	void PrintInfo::updateElapsedTime(uint32_t elapsed)
	{
		int32_t hours = elapsed / 3600;
		int32_t minutes = (elapsed % 3600) / 60;
		int32_t seconds = elapsed % 60;
		std::string elapsedStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_elapsedTime.setText(fmt::format(fmt::runtime(_("status_elapsed_time")), elapsedStr));
	}

	void PrintInfo::updateRemainingTime(uint32_t remaining)
	{
		UI_LOCK();
		int32_t hours = remaining / 3600;
		int32_t minutes = (remaining % 3600) / 60;
		int32_t seconds = remaining % 60;
		std::string remainingStr = fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds);
		m_remainingTime.setText(fmt::format(fmt::runtime(_("status_remaining_time")), remainingStr));
	}

	void PrintInfo::updateLayer(float height, float maxHeight)
	{
		UI_LOCK();
		m_speedInfo.updatePrintHeight(maxHeight);
		m_speedInfo.updatePrintHeight(height);
	}

	void PrintInfo::updateFanSpeed(uint32_t speed)
	{
		// m_fanSpeed.setText(fmt::format(fmt::runtime(_("status_fan_speed")), speed));
	}

	void PrintInfo::updateAcceleration(uint32_t acceleration)
	{
		m_speedInfo.updateAcceleration(acceleration);
	}

	void PrintInfo::updateZOffset(float offset)
	{
		m_speedInfo.updateZOffset(offset);
	}

	void PrintInfo::updateLayerNumber(uint32_t layer)
	{
		m_speedInfo.updateLayerNumber(layer);
	}

	PrintInfo::SpeedInfo::SpeedInfo(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		for (size_t i = 0; i < lv_obj_get_child_cnt(getRoot()); i++)
		{
			lv_obj_t* child = lv_obj_get_child(getRoot(), i);
			lv_obj_set_align(child, LV_ALIGN_LEFT_MID);
			lv_obj_set_width(child, LV_PCT(100));
			lv_obj_set_height(child, LV_SIZE_CONTENT);
		}

		updateSpeed(0, 0);
		updateSpeedMultiplier(0);
		updateAcceleration(0);
		updateZOffset(0);
		updatePrintHeight(0);
		updateLayerNumber(0);
	}

	void PrintInfo::SpeedInfo::updateSpeed(float topSpeed, float requestedSpeed)
	{
		m_speed.setText(fmt::format(fmt::runtime(_("status_speed_detailed")), topSpeed, requestedSpeed));
	}

	void PrintInfo::SpeedInfo::updateSpeedMultiplier(uint32_t multiplier)
	{
		m_speedMultiplier.setText(fmt::format(fmt::runtime(_("status_speed_multiplier")), multiplier));
	}

	void PrintInfo::SpeedInfo::updateAcceleration(uint32_t acceleration)
	{
		m_acceleration.setText(fmt::format(fmt::runtime(_("status_acceleration")), acceleration));
	}

	void PrintInfo::SpeedInfo::updateZOffset(float offset)
	{
		m_z_offset.setText(fmt::format(fmt::runtime(_("status_z_offset")), offset));
	}

	void PrintInfo::SpeedInfo::updatePrintHeight(float height)
	{
		m_z_height.setText(fmt::format(fmt::runtime(_("status_print_height")), height));
	}

	void PrintInfo::SpeedInfo::updateLayerNumber(uint32_t layer)
	{
		m_layer.setText(fmt::format(fmt::runtime(_("status_layer_number")), layer));
	}

} // namespace UI

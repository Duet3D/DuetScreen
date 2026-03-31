/*
 * ExtruderControl.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "ExtruderControl.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <algorithm>

namespace UI
{
	static const std::vector<float> s_defaultDistanceValues = {1.0f, 2.0f, 5.0f, 10.0f};
	static const std::vector<float> s_defaultFeedrateValues = {1.0f, 5.0f, 20.0f, 50.0f};

	ExtruderControl::ExtruderControl(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

		m_controlsContainer.setWidth(LV_PCT(100));
		m_controlsContainer.setFlexGrow(1);

		static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static int32_t row_dsc[] = {LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_TEMPLATE_LAST};
		m_controlsContainer.setGridDsc(col_dsc, row_dsc);
		m_controlsContainer.setGridCell(m_retractBtn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_controlsContainer.setGridCell(m_extrudeBtn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_controlsContainer.setGridCell(m_distanceSelector, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 1, 1);
		m_controlsContainer.setGridCell(m_feedrateSelector, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);

		m_retractBtn.setText(_("extrude.retract"));
		m_extrudeBtn.setText(_("extrude.extrude"));
		m_retractBtn.setIcon("retract.png");
		m_extrudeBtn.setIcon("extrude.png");
		m_distanceSelector.setLabel(_("extrude.feed_dist", Units::getDisplayedDistanceUnit()));
		m_feedrateSelector.setLabel(_("extrude.feed_rate", Units::getDisplayedSpeedUnit()));

		m_distanceSelector.setMinValue(0.1f);
		m_feedrateSelector.setMinValue(0.1f);
		if (m_distanceSelector.getValues().empty())
		{
			m_distanceSelector.setValueBtns(s_defaultDistanceValues);
		}
		if (m_feedrateSelector.getValues().empty())
		{
			m_feedrateSelector.setValueBtns(s_defaultFeedrateValues);
		}
		m_distanceSelector.setStorageKey(ID_MVS_EXTRUSION_DISTANCES);
		m_feedrateSelector.setStorageKey(ID_MVS_EXTRUSION_FEEDRATES);

		m_retractBtn.addClickedCallback(onRetractEvent, this);
		m_extrudeBtn.addClickedCallback(onExtrudeEvent, this);

		m_retractBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_extrudeBtn.addStyle(Themes::getLvglStyles().actionBtn);
	}

	void ExtruderControl::clear()
	{
		ZoneScoped;
		m_retractBtn.setState(LV_STATE_DISABLED, true);
		m_extrudeBtn.setState(LV_STATE_DISABLED, true);
	}

	void ExtruderControl::setExtrudeDisabled(bool disabled)
	{
		ZoneScoped;
		if (disabled == m_extrudeBtn.hasState(LV_STATE_DISABLED))
		{
			return;
		}

		m_extrudeBtn.setState(LV_STATE_DISABLED, disabled, true);
	}

	void ExtruderControl::setRetractDisabled(bool disabled)
	{
		ZoneScoped;
		if (disabled == m_retractBtn.hasState(LV_STATE_DISABLED))
		{
			return;
		}

		m_retractBtn.setState(LV_STATE_DISABLED, disabled, true);
	}

	float ExtruderControl::getDistanceValue() const
	{
		ZoneScoped;
		return m_distanceSelector.getValue();
	}

	float ExtruderControl::getFeedrateValue() const
	{
		ZoneScoped;
		return m_feedrateSelector.getValue();
	}

	void ExtruderControl::setNumberPad(ModalNumberPad* np)
	{
		ZoneScoped;
		UI_LOCK();
		m_numberPad = np;
		m_distanceSelector.setNumberPad(np);
		m_feedrateSelector.setNumberPad(np);
	}

	void ExtruderControl::onRetractEvent(lv_event_t* event)
	{
		ZoneScoped;
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));
		float dist = -control->getDistanceValue();
		float rate = control->getFeedrateValue();

		LOG_DBG("Retracting {} at {} in {}", dist, rate, control->getName());
		control->getPresenter()->extrude(dist, rate);
	}

	void ExtruderControl::onExtrudeEvent(lv_event_t* event)
	{
		ZoneScoped;
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));
		float dist = control->getDistanceValue();
		float rate = control->getFeedrateValue();

		LOG_DBG("Extruding {} at {} in {}", dist, rate, control->getName());
		control->getPresenter()->extrude(dist, rate);
	}
} // namespace UI

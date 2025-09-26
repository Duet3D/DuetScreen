/*
 * ExtruderControl.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "ExtruderControl.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>

namespace UI
{
	static const std::vector<float> s_defaultDistanceValues = {1.0f, 5.0f, 10.0f};
	static const std::vector<float> s_defaultFeedrateValues = {1.0f, 5.0f, 20.0f};

	ExtruderControl::ExtruderControl(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

		m_toolSelect.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_filamentContainer.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_controlsContainer.setWidth(LV_PCT(100));
		m_controlsContainer.setFlexGrow(1);

		m_toolSelect.setTitle(_("tool_select"));

		m_filamentContainer.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_filamentSelect.setHeight(LV_SIZE_CONTENT);
		m_filamentSelect.setFlexGrow(1);
		m_filamentChangeBtn.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_filamentUnloadBtn.setSize(LV_SIZE_CONTENT, LV_PCT(100));

		m_filamentSelect.setLabel(_("filament_select"));
		m_filamentChangeBtn.setText(_("filament_change"));
		m_filamentUnloadBtn.setText(_("unload"));

		static int32_t col_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		m_controlsContainer.setGridDsc(col_dsc, row_dsc);
		m_controlsContainer.setGridCell(m_distanceInput, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_controlsContainer.setGridCell(m_feedrateInput, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		m_controlsContainer.setGridCell(m_retractBtn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_controlsContainer.setGridCell(m_extrudeBtn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		m_distanceInput.setTitle(_("extrude_feed_dist"));
		m_feedrateInput.setTitle(_("extrude_feed_rate"));

		m_distanceInput.setListFlow(LV_FLEX_FLOW_ROW);
		m_distanceInput.setListGrow(1);

		m_feedrateInput.setListFlow(LV_FLEX_FLOW_ROW);
		m_feedrateInput.setListGrow(1);

		m_distanceValues = StorageHelper::getData<std::vector<float>>(ID_EXTRUSION_DISTANCES, s_defaultDistanceValues);
		m_feedrateValues = StorageHelper::getData<std::vector<float>>(ID_EXTRUSION_FEEDRATES, s_defaultFeedrateValues);
		m_selectedDistanceIndex =
			std::min(StorageHelper::getData<size_t>(ID_EXTRUSION_SELECTED_DISTANCE, 0), m_distanceValues.size() - 1);
		m_selectedFeedrateIndex =
			std::min(StorageHelper::getData<size_t>(ID_EXTRUSION_SELECTED_FEEDRATE, 0), m_feedrateValues.size() - 1);
		m_distanceInput.setItemCount(m_distanceValues.size(), this, &ExtruderControl::createDistanceButton);
		m_feedrateInput.setItemCount(m_feedrateValues.size(), this, &ExtruderControl::createFeedrateButton);

		m_toolSelect.setListFlow(LV_FLEX_FLOW_ROW);
		m_toolSelect.setListSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_filamentSelect.addEventCallback(onFilamentSelectEvent, LV_EVENT_VALUE_CHANGED, this);
		m_filamentChangeBtn.addClickedCallback(onFilamentChangeEvent, this);
		m_filamentUnloadBtn.addClickedCallback(onFilamentUnloadEvent, this);
		m_retractBtn.addClickedCallback(onRetractEvent, this);
		m_extrudeBtn.addClickedCallback(onExtrudeEvent, this);

		m_toolSelect.addStyle(Themes::getLvglStyles().no_border);
		m_filamentContainer.addStyle(Themes::getLvglStyles().no_border);
		m_filamentSelect.addStyle(Themes::getLvglStyles().no_border);
		m_controlsContainer.addStyle(Themes::getLvglStyles().no_border);
		m_retractBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_extrudeBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_filamentChangeBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_filamentUnloadBtn.addStyle(Themes::getLvglStyles().actionBtn);
		m_distanceInput.addStyle(Themes::getLvglStyles().no_border);
		m_feedrateInput.addStyle(Themes::getLvglStyles().no_border);
	}

	void ExtruderControl::clear()
	{
		m_toolSelect.clear();
		setFilamentDisabled(true);
		m_retractBtn.setState(LV_STATE_DISABLED, true);
		m_extrudeBtn.setState(LV_STATE_DISABLED, true);
	}

	void ExtruderControl::setToolCallback(tool_select_cb_t cb)
	{
		UI_LOCK();
		LOG_DBG("Setting tool select callback for {}", getName());
		m_toolSelectCb = std::move(cb);
	}

	void ExtruderControl::setToolCount(const size_t count)
	{
		UI_LOCK();
		if (count == m_toolSelect.getItemCount())
		{
			return;
		}

		LOG_DBG("Setting tool count to {} for {}", count, getName());
		m_toolSelect.setItemCount(count, this, &ExtruderControl::createToolButton);
	}

	void ExtruderControl::setToolName(const size_t index, const std::string& name)
	{
		UI_LOCK();
		if (index >= m_toolSelect.getItemCount())
		{
			LOG_WARN("Index {} out of bounds for tool names in {}", index, getName());
			return;
		}

		LOG_DBG("Setting tool name at index {} to '{}' for {}", index, name, getName());
		auto btn = m_toolSelect.getItem(index);
		if (!btn)
		{
			LOG_ERROR("Failed to get tool button at index {} in {}", index, getName());
			return;
		}
		btn->setText(name);
	}

	void ExtruderControl::setCurrentTool(const int32_t index)
	{
		UI_LOCK();
		LOG_DBG("Setting current tool to {} for {}", index, getName());
		if (index == m_currentToolIndex)
		{
			return;
		}

		m_currentToolIndex = index;

		for (size_t i = 0; i < m_toolSelect.getItemCount(); ++i)
		{
			auto btn = m_toolSelect.getItem(i);
			if (!btn)
			{
				continue;
			}

			btn->setChecked(static_cast<int32_t>(index) == i);
		}
		setFilamentDisabled(index < 0);
		m_loadedFilament = "some_placeholder"; // This is a hack
		m_retractBtn.setState(LV_STATE_DISABLED, index < 0, true);
		m_extrudeBtn.setState(LV_STATE_DISABLED, index < 0, true);
	}

	void ExtruderControl::setFilamentDisabled(bool disabled)
	{
		if (disabled == m_filamentContainer.hasState(LV_STATE_DISABLED))
		{
			return;
		}

		m_filamentContainer.setState(LV_STATE_DISABLED, disabled, true);
		m_filamentSelect.setOptions(disabled ? std::vector<std::string>() : m_filamentOptions);
		// m_filamentSelect.setText(disabled ? "" : m_loadedFilament);
	}

	void ExtruderControl::setFilamentOptions(const std::vector<std::string>& options)
	{
		UI_LOCK();
		LOG_DBG("Setting filament options for {}", getName());
		m_filamentOptions = options;
		m_filamentSelect.setOptions(options);
	}

	void ExtruderControl::setFilamentSelected(const std::string& filament)
	{
		UI_LOCK();
		LOG_DBG("Setting selected filament to '{}' for {}", filament, getName());

		if (filament == m_loadedFilament)
		{
			return;
		}

		m_filamentSelect.setSelected(filament);
		m_filamentSelect.setText(filament);
		m_loadedFilament = filament;
		m_filamentChangeBtn.hide();
	}

	void ExtruderControl::setFilamentCallback(filament_cb_t cb)
	{
		UI_LOCK();
		LOG_DBG("Setting filament callback for {}", getName());
		m_filamentCb = std::move(cb);
	}

	void ExtruderControl::setExtrudeCallback(extrude_cb_t cb)
	{
		UI_LOCK();
		LOG_DBG("Setting extrude callback for {}", getName());
		m_extrudeCb = std::move(cb);
	}

	void ExtruderControl::setDistanceCallback(distance_cb_t cb)
	{
		UI_LOCK();
		LOG_DBG("Setting distance callback for {}", getName());
		m_distanceCb = std::move(cb);
	}

	void ExtruderControl::setFeedrateCallback(feedrate_cb_t cb)
	{
		UI_LOCK();
		LOG_DBG("Setting feedrate callback for {}", getName());
		m_feedrateCb = std::move(cb);
	}

	void ExtruderControl::setDistanceValue(size_t index, float value)
	{
		UI_LOCK();
		LOG_DBG("Setting distance value to {} for {}", value, getName());
		if (index < m_distanceValues.size())
		{
			m_distanceValues[index] = value;
			StorageHelper::setData(ID_EXTRUSION_DISTANCES, m_distanceValues);
			auto btn = m_distanceInput.getItem(index);
			if (!btn)
			{
				LOG_ERROR("Failed to get distance button at index {} in {}", index, getName());
				return;
			}
			btn->setText(fmt::format("{:g}", value));
		}
		else
		{
			LOG_WARN("Index {} out of bounds for distance values in {}", index, getName());
			return;
		}
	}

	void ExtruderControl::setFeedrateValue(size_t index, float value)
	{
		UI_LOCK();
		LOG_DBG("Setting feedrate value to {} for {}", value, getName());
		if (index < m_feedrateValues.size())
		{
			m_feedrateValues[index] = value;
			StorageHelper::setData(ID_EXTRUSION_FEEDRATES, m_feedrateValues);
			auto btn = m_feedrateInput.getItem(index);
			if (!btn)
			{
				LOG_ERROR("Failed to get feedrate button at index {} in {}", index, getName());
				return;
			}
			btn->setText(fmt::format("{:g}", value));
		}
		else
		{
			LOG_WARN("Index {} out of bounds for feedrate values in {}", index, getName());
			return;
		}
	}

	float ExtruderControl::getDistanceValue(size_t index) const
	{
		UI_LOCK();
		if (index < m_distanceValues.size())
		{
			LOG_DBG("Getting distance value at index {}: {} for {}", index, m_distanceValues[index], getName());
			return m_distanceValues[index];
		}
		LOG_WARN("Index {} out of bounds for distance values in {}", index, getName());
		return 0.0f;
	}

	float ExtruderControl::getFeedrateValue(size_t index) const
	{
		UI_LOCK();
		if (index < m_feedrateValues.size())
		{
			LOG_DBG("Getting feedrate value at index {}: {} for {}", index, m_feedrateValues[index], getName());
			return m_feedrateValues[index];
		}
		LOG_WARN("Index {} out of bounds for feedrate values in {}", index, getName());
		return 0.0f;
	}

	void ExtruderControl::onToolSelectEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto btn = static_cast<lv_obj_t*>(lv_event_get_target(event));
		if (!btn)
		{
			LOG_ERROR("Failed to get button from event");
			return;
		}
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		size_t index = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn)));
		LOG_DBG("Tool button clicked for index {} in {}", index, control->getName());

		if (control && control->m_toolSelectCb)
		{
			LOG_DBG("Calling tool select callback for index {} in {}", index, control->getName());
			control->m_toolSelectCb(index);
		}
	}

	void ExtruderControl::onFilamentSelectEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		std::string selected_filament = control->m_filamentSelect.getSelectedString();
		control->m_filamentSelect.setText(selected_filament);

		if (selected_filament == control->m_loadedFilament)
		{
			control->m_filamentChangeBtn.hide();
			return;
		}

		control->m_filamentChangeBtn.show();
	}

	void ExtruderControl::onFilamentChangeEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		if (control && control->m_filamentCb)
		{
			std::string selected = control->m_filamentSelect.getSelectedString();
			LOG_DBG("Calling filament callback for '{}' in {}", selected, control->getName());
			control->m_filamentCb(selected);
		}
	}

	void ExtruderControl::onFilamentUnloadEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		if (control && control->m_filamentCb)
		{
			LOG_DBG("Calling filament unload callback in {}", control->getName());
			control->m_filamentCb("");
		}
	}

	void ExtruderControl::onDistanceEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		size_t index = static_cast<size_t>(
			reinterpret_cast<uintptr_t>(lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_target(event)))));

		lv_event_code_t code = lv_event_get_code(event);

		switch (code)
		{
		case LV_EVENT_CLICKED:
		{
			control->m_distanceInput.getItem(control->m_selectedDistanceIndex)->setChecked(false);
			control->m_selectedDistanceIndex = index;
			control->m_distanceInput.getItem(index)->setChecked(true);
			StorageHelper::setData(ID_EXTRUSION_SELECTED_DISTANCE, index);
			break;
		}
		case LV_EVENT_LONG_PRESSED:
		{
			if (control && control->m_distanceCb)
			{
				LOG_DBG("Calling distance callback with value {} in {}",
						control->getDistanceValue(index),
						control->getName());
				control->m_distanceCb(index, control->getDistanceValue(index));
			}
			break;
		}
		}
	}

	void ExtruderControl::onFeedrateEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));

		size_t index = static_cast<size_t>(
			reinterpret_cast<uintptr_t>(lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_target(event)))));

		lv_event_code_t code = lv_event_get_code(event);

		switch (code)
		{
		case LV_EVENT_CLICKED:
		{
			control->m_feedrateInput.getItem(control->m_selectedFeedrateIndex)->setChecked(false);
			control->m_selectedFeedrateIndex = index;
			control->m_feedrateInput.getItem(index)->setChecked(true);
			StorageHelper::setData(ID_EXTRUSION_SELECTED_FEEDRATE, index);
			break;
		}
		case LV_EVENT_LONG_PRESSED:
		{
			if (control && control->m_feedrateCb)
			{
				LOG_DBG("Calling feedrate callback with value {} in {}",
						control->getFeedrateValue(index),
						control->getName());
				control->m_feedrateCb(index, control->getFeedrateValue(index));
			}
			break;
		}
		}
	}

	void ExtruderControl::onRetractEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));
		float dist = -control->getDistanceValue(control->m_selectedDistanceIndex);
		float rate = control->getFeedrateValue(control->m_selectedFeedrateIndex);

		LOG_DBG("Retracting {} at {} in {}", dist, rate, control->getName());
		if (control && control->m_extrudeCb)
		{
			control->m_extrudeCb(dist, rate);
		}
	}

	void ExtruderControl::onExtrudeEvent(lv_event_t* event)
	{
		UI_LOCK();
		auto control = static_cast<ExtruderControl*>(lv_event_get_user_data(event));
		float dist = control->getDistanceValue(control->m_selectedDistanceIndex);
		float rate = control->getFeedrateValue(control->m_selectedFeedrateIndex);

		LOG_DBG("Extruding {} at {} in {}", dist, rate, control->getName());
		if (control && control->m_extrudeCb)
		{
			control->m_extrudeCb(dist, rate);
		}
	}

	void ExtruderControl::onShow()
	{
		std::string filament = m_loadedFilament;
		m_loadedFilament = "some_placeholder";
		setFilamentSelected(filament);
	}

	std::shared_ptr<Button> ExtruderControl::createBaseListButton(size_t index, LvObj& parent)
	{
		UI_LOCK();
		LOG_DBG("Creating base list button {} for {}", index, lv_obj_get_name(parent));
		auto btn = std::make_shared<Button>(fmt::format("{}", index), parent);
		btn->setFlexGrow(1);
		btn->setHeight(LV_SIZE_CONTENT);
		btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(index)));
		return btn;
	}

	std::shared_ptr<Button> ExtruderControl::createToolButton(size_t index, LvObj& parent)
	{
		LOG_DBG("Creating tool button {} for {}", index, getName());
		auto btn = createBaseListButton(index, parent);
		btn->addClickedCallback(onToolSelectEvent, this);
		btn->addStyle(Themes::getLvglStyles().actionBtn);
		return btn;
	}

	std::shared_ptr<Button> ExtruderControl::createDistanceButton(size_t index, LvObj& parent)
	{
		LOG_DBG("Creating distance button {} for {}", index, getName());
		auto btn = createBaseListButton(index, parent);
		btn->addEventCallback(onDistanceEvent, LV_EVENT_ALL, this);
		btn->setHeight(LV_PCT(100));
		btn->setChecked(index == m_selectedDistanceIndex);
		btn->addStyle(Themes::getLvglStyles().long_press, 0);
		if (index < m_distanceValues.size())
		{
			btn->setText(fmt::format("{:g}", m_distanceValues[index]));
		}
		else
		{
			LOG_WARN("Index {} out of bounds for distance values in {}", index, getName());
			btn->setText(_("unknown"));
		}
		return btn;
	}

	std::shared_ptr<Button> ExtruderControl::createFeedrateButton(size_t index, LvObj& parent)
	{
		LOG_DBG("Creating feedrate button {} for {}", index, getName());
		auto btn = createBaseListButton(index, parent);
		btn->addEventCallback(onFeedrateEvent, LV_EVENT_ALL, this);
		btn->setHeight(LV_PCT(100));
		btn->setChecked(index == m_selectedFeedrateIndex);
		btn->addStyle(Themes::getLvglStyles().long_press, 0);
		if (index < m_feedrateValues.size())
		{
			btn->setText(fmt::format("{:g}", m_feedrateValues[index]));
		}
		else
		{
			LOG_WARN("Index {} out of bounds for feedrate values in {}", index, getName());
			btn->setText(_("unknown"));
		}
		return btn;
	}
} // namespace UI

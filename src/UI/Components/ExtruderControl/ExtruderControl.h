/*
 * ExtruderControl.h
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/DropdownMenu.h"
#include "UI/Components/Input/TextBox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"

namespace UI
{
	class ExtruderControl : public LvContainer
	{
		using tool_select_cb_t = std::function<void(size_t index)>;
		using filament_cb_t = std::function<void(const std::string& filament)>;
		using extrude_cb_t = std::function<void(float distance, float feedrate)>;
		using distance_cb_t = std::function<void(size_t index, float distance)>;
		using feedrate_cb_t = std::function<void(size_t index, float feedrate)>;

	  public:
		ExtruderControl(const std::string& name, LvObj& parent);

		void clear();

		void setToolCallback(tool_select_cb_t cb);
		void setToolCount(size_t count);
		void setToolName(size_t index, const std::string& name);
		void setCurrentTool(int32_t index);

		void setFilamentDisabled(bool disabled);
		void setFilamentOptions(const std::vector<std::string>& options);
		void setFilamentSelected(const std::string& filament);
		void setFilamentCallback(filament_cb_t cb);

		void setExtrudeCallback(extrude_cb_t cb);
		void setDistanceCallback(distance_cb_t cb);
		void setFeedrateCallback(feedrate_cb_t cb);

		void setDistanceValue(size_t index, float value);
		void setFeedrateValue(size_t index, float value);

		float getDistanceValue(size_t index) const;
		float getFeedrateValue(size_t index) const;

	  private:
		static void onToolSelectEvent(lv_event_t* event);
		static void onFilamentSelectEvent(lv_event_t* event);
		static void onFilamentChangeEvent(lv_event_t* event);
		static void onFilamentUnloadEvent(lv_event_t* event);
		static void onDistanceEvent(lv_event_t* event);
		static void onFeedrateEvent(lv_event_t* event);
		static void onRetractEvent(lv_event_t* event);
		static void onExtrudeEvent(lv_event_t* event);

		void onShow() override;

		std::shared_ptr<Button> createBaseListButton(size_t index, LvObj& parent);
		std::shared_ptr<Button> createToolButton(size_t index, LvObj& parent);
		std::shared_ptr<Button> createDistanceButton(size_t index, LvObj& parent);
		std::shared_ptr<Button> createFeedrateButton(size_t index, LvObj& parent);

		List<Button> m_toolSelect{"tool_select", getRoot()};

		LvContainer m_filamentContainer{"filament", getRoot()};
		DropdownMenu m_filamentSelect{"filament_select", m_filamentContainer};
		Button m_filamentChangeBtn{"filament_change", m_filamentContainer};
		Button m_filamentUnloadBtn{"filament_load_unload", m_filamentContainer};

		LvContainer m_controlsContainer{"controls", getRoot()};
		Button m_retractBtn{"retract", m_controlsContainer, LV_SYMBOL_UP};
		Button m_extrudeBtn{"extrude", m_controlsContainer, LV_SYMBOL_DOWN};
		List<Button> m_distanceInput{"distance_input", m_controlsContainer};
		List<Button> m_feedrateInput{"feedrate_input", m_controlsContainer};

		std::vector<float> m_distanceValues;
		std::vector<float> m_feedrateValues;
		size_t m_selectedDistanceIndex;
		size_t m_selectedFeedrateIndex;

		tool_select_cb_t m_toolSelectCb; // Callback for when a tool is selected
		filament_cb_t m_filamentCb;		 // Callback for when a filament selection changes
		extrude_cb_t m_extrudeCb;		 // Callback for when extrude/retract is clicked
		distance_cb_t m_distanceCb;		 // Callback for when distance input is clicked
		feedrate_cb_t m_feedrateCb;		 // Callback for when feedrate input is clicked

		int32_t m_currentToolIndex = -1;
		std::vector<std::string> m_filamentOptions;
		std::string m_loadedFilament;
	};
} // namespace UI

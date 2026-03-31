/*
 * Tool.h
 *
 *  Created on: 17 Feb 2021
 *      Author: manuel
 */

#pragma once

// #include <cstdint>
#include "Axis.h"
#include "Configuration.h"
#include "Fan.h"
#include "Heat.h"
#include "Spindle.h"
#include <Duet3D/General/FreelistManager.h>
#include <Duet3D/General/String.h>
#include <Duet3D/General/StringRef.h>
#include <Duet3D/General/function_ref.h>
#include <memory>
#include <sys/types.h>

namespace OM
{
	// Status that a tool may report to us. Must be in alphabetical order.
	enum class ToolStatus
	{
		active = 0,
		off,
		standby,
		unknown,
	};

	struct ToolStatusMapEntry
	{
		const char* key;
		ToolStatus val;
	};

	// This table must be kept in case-insensitive alphabetical order of the search string.
	const ToolStatusMapEntry toolStatusMap[] = {
		{"active", ToolStatus::active},
		{"off", ToolStatus::off},
		{"standby", ToolStatus::standby},
		{"unknown", ToolStatus::unknown},
	};

	struct ToolHeater
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<ToolHeater>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<ToolHeater>(p); }

		size_t index;
		int32_t activeTemp;
		int32_t standbyTemp;
		Heat::HeaterPtr heater;

		void Reset();
	};

	using ToolHeaterPtr = std::shared_ptr<ToolHeater>;

	struct Tool
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<Tool>(); }
		void operator delete(void* p) noexcept;

		// tool number
		size_t index;
		String<MAX_TOOL_NAME_LENGTH> name;
		ToolHeaterPtr heaters[MAX_HEATERS_PER_TOOL];
		Move::ExtruderAxisPtr extruders[MAX_EXTRUDERS_PER_TOOL];
		float mix[MAX_EXTRUDERS_PER_TOOL];
		FanPtr fans[MAX_FANS];
		SpindlePtr spindle;
		int32_t spindleRpm;
		float offsets[MAX_TOTAL_AXES];
		ToolStatus status;
		int8_t filamentExtruder;

		std::string GetName() const;

		ToolHeaterPtr GetHeater(const size_t toolHeaterIndex);
		ToolHeaterPtr GetOrCreateHeater(const size_t toolHeaterIndex, const size_t heaterIndex);

		Move::ExtruderAxisPtr GetExtruder(const size_t toolExtruderIndex) const;
		Move::ExtruderAxisPtr GetOrCreateExtruder(const size_t toolExtruderIndex, const size_t extruderIndex);
		size_t GetExtruderCount() const;

		FanPtr GetFan(const size_t toolFanIndex);
		FanPtr GetOrCreateFan(const size_t toolFanIndex, const size_t fanIndex);
		size_t GetFanCount() const;

		StringRef GetFilament() const;

		int32_t GetHeaterTarget(const size_t toolHeaterIndex, const bool active);
		bool GetHeaterTemps(const StringRef& ref, const bool active);
		bool SetHeaterTemps(const size_t toolHeaterIndex, const int32_t temp, const bool active);
		size_t GetHeaterCount() const;
		bool HasHeater(const size_t heaterIndex) const;
		void IterateHeaters(function_ref<void(ToolHeaterPtr, size_t)> func, const size_t startAt = 0);
		void IterateExtruders(function_ref<void(Move::ExtruderAxisPtr, size_t)> func, const size_t startAt = 0);
		void IterateFans(function_ref<void(FanPtr, size_t)> func, const size_t startAt = 0);
		size_t RemoveHeatersFrom(const size_t toolHeaterIndex);
		size_t RemoveExtrudersFrom(const size_t toolExtruderIndex);
		size_t RemoveFansFrom(const size_t toolFanIndex);
		void UpdateTemp(const size_t toolHeaterIndex, const int32_t temp, const bool active);
		const char* GetStatusStr() const;
		void ToggleState();
		void ToggleHeaterState(const size_t toolHeaterIndex);
		void ToggleSpindleState();
		void UpdateSpindleTarget(const int32_t rpm);

		void ChangeFilament(const std::string& filament);
		void LoadFilament(const std::string& filament);
		void UnloadFilament();

		void Reset();
	};

	using ToolPtr = std::shared_ptr<Tool>;

	ToolPtr GetTool(const size_t index);
	ToolPtr GetOrCreateTool(const size_t index);
	ToolPtr GetToolBySlot(const size_t slot);
	size_t GetToolCount();
	bool IterateToolsWhile(function_ref<bool(ToolPtr, size_t)> func, const size_t startAt = 0);
	size_t RemoveTool(const size_t index, const bool allFollowing);

	bool UpdateToolHeater(const size_t toolIndex, const size_t toolHeaterIndex, const size_t heaterIndex);
	bool RemoveToolHeaters(const size_t toolIndex, const size_t firstIndexToDelete = 0);

	bool UpdateToolExtruder(const size_t toolIndex, const size_t toolExtruderIndex, const size_t extruderIndex);
	bool RemoveToolExtruders(const size_t toolIndex, const size_t firstIndexToDelete = 0);
	bool UpdateToolMix(const size_t toolIndex, const size_t toolExtruderIndex, const float mix);

	bool UpdateToolFan(const size_t toolIndex, const size_t toolFanIndex, const size_t fanIndex);
	bool RemoveToolFans(const size_t toolIndex, const size_t firstIndexToDelete = 0);
	bool UpdateToolOffset(const size_t toolIndex, const size_t axisIndex, const float offset);

	bool UpdateToolFilamentExtruder(const size_t toolIndex, const int8_t extruderIndex);

	bool UpdateToolTemp(const size_t toolIndex, const size_t toolHeaterIndex, const int32_t temp, const bool active);
	bool UpdateToolName(const size_t toolIndex, const char* name);
	bool UpdateToolStatus(const size_t toolIndex, const char* statusStr);

	bool UpdateToolSpindle(const size_t toolIndex, const int8_t spindleIndex);
	bool UpdateToolSpindleRpm(const size_t toolIndex, const int32_t rpm);

	void SetCurrentTool(const int32_t toolIndex);
	ToolPtr GetCurrentTool();
	float GetCurrentToolAxisOffset(const Move::AxisPtr& axis);
} // namespace OM

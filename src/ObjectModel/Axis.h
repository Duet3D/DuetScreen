/*
 * Axis.h
 *
 *  Created on: 17 Feb 2021
 *      Author: manuel
 */

#pragma once

// #include <cstdint>
#include "Configuration.h"
#include "utils/UnitSystem.h"
#include <Duet3D/General/FreelistManager.h>
#include <Duet3D/General/String.h>
#include <Duet3D/General/function_ref.h>
#include <memory>
#include <sys/types.h>
#include <vector>

namespace OM::Move
{
	enum Workplaces
	{
		G54,
		G55,
		G56,
		G57,
		G58,
		G59,
		G59_1,
		G59_2,
		G59_3,
		MaxTotalWorkplaces
	};

	struct Axis
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<Axis>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<Axis>(p); }

		size_t index;
		uint32_t acceleration;
		float babystep;
		char letter[2];
		float workplaceOffsets[Workplaces::MaxTotalWorkplaces];
		float userPosition;
		float machinePosition;
		float minPosition;
		float maxPosition;
		int32_t maxSpeed; // mm/s
		uint8_t homed : 1, visible : 1;

		void Reset();
		void Home();
		void MoveAbsolute(float position, uint32_t feedrate, bool machineCoordinates = false);
		void MoveRelative(float distance, uint32_t feedrate);
	};

	using AxisPtr = std::shared_ptr<Axis>;

	struct ExtruderAxis
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<ExtruderAxis>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<ExtruderAxis>(p); }

		size_t index;
		float position;
		float factor;
		float stepsPerMm;
		float filamentDiameter;
		float pressureAdvance;
		int32_t maxSpeed; // mm/s
		String<MAX_FILAMENT_NAME_LENGTH> filamentName;

		void Reset();
	};

	using ExtruderAxisPtr = std::shared_ptr<ExtruderAxis>;

	struct Kinematics
	{
		std::string name;

		void Reset() { name.clear(); }
		bool IsDelta() const { return name == "delta" || name == "linearDelta"; } // name changed in RRF3.7
	};

	void Reset();

	std::vector<AxisPtr> GetAxes(const bool includeHidden = false);
	AxisPtr GetAxis(const size_t index);
	AxisPtr GetAxisBySlot(const size_t slot, const bool includeHidden = false);
	AxisPtr GetAxisByLetter(const char letter);
	AxisPtr GetOrCreateAxis(const size_t index);
	size_t GetAxisCount(const bool includeHidden = false);
	bool IterateAxesWhile(function_ref<bool(AxisPtr, size_t)> func, const size_t startAt = 0);
	size_t RemoveAxis(const size_t index, const bool allFollowing);

	bool SetAcceleration(size_t index, uint32_t acceleration);
	bool SetBabystepOffset(size_t index, float f);
	bool SetAxisHomedStatus(size_t index, bool homed);
	bool SetAxisLetter(size_t index, char letter);
	bool SetAxisUserPosition(size_t index, float f);
	bool SetAxisMachinePosition(size_t index, float f);
	bool SetAxisMinPosition(size_t index, float f);
	bool SetAxisMaxPosition(size_t index, float f);
	bool SetAxisMaxSpeed(size_t index, int32_t f);
	bool SetAxisVisible(size_t index, bool visible);
	bool SetAxisWorkplaceOffset(size_t axisIndex, size_t workplaceIndex, float offset);
	bool SetCurrentWorkplaceNumber(uint8_t workplaceNumber);
	uint8_t GetCurrentWorkplaceNumber();
	void SetPrintingAcceleration(uint32_t printingAcceleration);
	uint32_t GetPrintingAcceleration();
	bool GetNoMovesBeforeHoming();
	void SetNoMovesBeforeHoming(bool val);

	ExtruderAxisPtr GetExtruderAxis(const size_t index);
	ExtruderAxisPtr GetExtruderAxisBySlot(const size_t slot);
	ExtruderAxisPtr GetOrCreateExtruderAxis(const size_t index);
	size_t GetExtruderAxisCount();
	bool IterateExtruderAxesWhile(function_ref<bool(ExtruderAxisPtr, size_t)> func, const size_t startAt = 0);
	size_t RemoveExtruderAxis(const size_t index, const bool allFollowing);

	bool SetExtruderPosition(size_t index, float f);
	bool SetExtruderFactor(size_t index, float f);
	bool SetExtruderStepsPerMm(size_t index, float f);
	bool SetExtruderFilamentDiameter(size_t index, float f);
	bool SetExtruderPressureAdvance(size_t index, float f);
	bool SetExtruderMaxSpeed(size_t index, int32_t f);
	bool SetExtruderFilamentName(size_t index, const char* name);

	void SetExtrusionRate(float rate);
	float GetExtrusionRate();
	float GetVolumetricFlow();

	float GetSpeedFactor();
	void SetSpeedFactor(float factor);
	float GetCurrentMoveRequestedSpeed();
	void SetCurrentMoveRequestedSpeed(float speed);
	float GetCurrentMoveTopSpeed();
	void SetCurrentMoveTopSpeed(float speed);

	void SetKinematicsName(const std::string& name);
	const Kinematics& GetKinematics();

	void Extrude(float distance, float feedrate);

	void SetDistanceUnit(size_t index, Units::UnitSystem unit);
	Units::UnitSystem GetCurrentDistanceUnit();
} // namespace OM::Move

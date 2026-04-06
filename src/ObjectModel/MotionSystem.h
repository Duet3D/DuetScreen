/*
 * MotionSystem.h
 *
 * Stores per-motion-system data from move:motionSystems in the Duet object model.
 * Used for multi-motion-system printers (e.g. IDEX).
 */

#pragma once

#include "Configuration.h"
#include <Duet3D/General/FreelistManager.h>
#include <Duet3D/General/function_ref.h>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace OM::Move
{
	struct MotionSystem
	{
		struct CurrentMove
		{
			float acceleration;
			float deceleration;
			float distance;
			float duration;
			float extrusionRate;
			std::optional<float> laserPwm;
			float requestedSpeed;
			float topSpeed;

			void Reset();
		};

		struct Rotation
		{
			float angle;
			std::array<float, 2> centre;

			void Reset();
		};

		void* operator new(size_t) noexcept { return FreelistManager::Allocate<MotionSystem>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<MotionSystem>(p); }

		size_t index;
		CurrentMove currentMove;
		std::optional<int32_t> currentObject;
		int32_t currentTool;  // -1 = none
		int32_t nextTool;
		int32_t previousTool;
		float printingAcceleration;
		Rotation rotation;
		float speedFactor;
		float travelAcceleration;
		std::array<float, MAX_TOTAL_AXES> userPosition;
		float virtualEPos;
		int32_t workplaceNumber;

		void Reset();
	};

	using MotionSystemPtr = std::shared_ptr<MotionSystem>;

	MotionSystemPtr GetMotionSystem(size_t index);
	MotionSystemPtr GetOrCreateMotionSystem(size_t index);
	size_t RemoveMotionSystem(size_t index, bool allFollowing);
	size_t GetMotionSystemCount();
	bool IterateMotionSystemsWhile(function_ref<bool(MotionSystemPtr, size_t)> func, size_t startAt = 0);

	bool SetMotionSystemCurrentMoveAcceleration(size_t index, float acceleration);
	bool SetMotionSystemCurrentMoveDeceleration(size_t index, float deceleration);
	bool SetMotionSystemCurrentMoveDistance(size_t index, float distance);
	bool SetMotionSystemCurrentMoveDuration(size_t index, float duration);
	bool SetMotionSystemCurrentMoveExtrusionRate(size_t index, float rate);
	bool SetMotionSystemCurrentMoveLaserPwm(size_t index, float pwm);
	bool ClearMotionSystemCurrentMoveLaserPwm(size_t index);
	bool SetMotionSystemCurrentMoveRequestedSpeed(size_t index, float speed);
	bool SetMotionSystemCurrentMoveTopSpeed(size_t index, float speed);

	bool SetMotionSystemCurrentObject(size_t index, int32_t currentObject);
	bool ClearMotionSystemCurrentObject(size_t index);
	bool SetMotionSystemCurrentTool(size_t index, int32_t tool);
	bool SetMotionSystemNextTool(size_t index, int32_t tool);
	bool SetMotionSystemPreviousTool(size_t index, int32_t tool);
	bool SetMotionSystemPrintingAcceleration(size_t index, float acceleration);
	bool SetMotionSystemRotationAngle(size_t index, float angle);
	bool SetMotionSystemRotationCentre(size_t index, size_t centreIndex, float value);
	bool SetMotionSystemSpeedFactor(size_t index, float factor);
	bool SetMotionSystemTravelAcceleration(size_t index, float acceleration);
	bool SetMotionSystemUserPosition(size_t index, size_t axisIndex, float position);
	bool SetMotionSystemVirtualEPos(size_t index, float epos);
	bool SetMotionSystemWorkplaceNumber(size_t index, int32_t workplaceNumber);
} // namespace OM::Move

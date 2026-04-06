/*
 * MotionSystem.cpp
 *
 * Stores per-motion-system data from move:motionSystems in the Duet object model.
 */

#include "MotionSystem.h"

#include "Configuration.h"
#include "Debug.h"
#include "ListHelpers.h"
#include <Duet3D/General/Vector.h>

namespace OM::Move
{
	typedef Vector<MotionSystemPtr, MAX_MOTION_SYSTEMS> MotionSystemList;
	static MotionSystemList s_motionSystems;

	void MotionSystem::CurrentMove::Reset()
	{
		acceleration = 0.0f;
		deceleration = 0.0f;
		distance = 0.0f;
		duration = 0.0f;
		extrusionRate = 0.0f;
		laserPwm = std::nullopt;
		requestedSpeed = 0.0f;
		topSpeed = 0.0f;
	}

	void MotionSystem::Rotation::Reset()
	{
		angle = 0.0f;
		centre = {0.0f, 0.0f};
	}

	void MotionSystem::Reset()
	{
		currentMove.Reset();
		currentObject = std::nullopt;
		currentTool = -1;
		nextTool = -1;
		previousTool = -1;
		printingAcceleration = 10000.0f;
		rotation.Reset();
		speedFactor = 1.0f;
		travelAcceleration = 10000.0f;
		userPosition.fill(0.0f);
		virtualEPos = 0.0f;
		workplaceNumber = 0;
	}

	MotionSystemPtr GetMotionSystem(const size_t index)
	{
		if (index >= MAX_MOTION_SYSTEMS)
		{
			return nullptr;
		}
		return GetOrCreate<MotionSystemList, MotionSystem>(s_motionSystems, index, false);
	}

	MotionSystemPtr GetOrCreateMotionSystem(const size_t index)
	{
		if (index >= MAX_MOTION_SYSTEMS)
		{
			LOG_ERROR("MotionSystem index {:d} greater than MAX_MOTION_SYSTEMS", index);
			return nullptr;
		}
		return GetOrCreate<MotionSystemList, MotionSystem>(s_motionSystems, index, true);
	}

	size_t RemoveMotionSystem(const size_t index, const bool allFollowing)
	{
		return Remove<MotionSystemList, MotionSystem>(s_motionSystems, index, allFollowing);
	}

	size_t GetMotionSystemCount()
	{
		return s_motionSystems.Size();
	}

	bool IterateMotionSystemsWhile(function_ref<bool(MotionSystemPtr, size_t)> func, const size_t startAt)
	{
		return s_motionSystems.IterateWhile(func, startAt);
	}

	static MotionSystemPtr getWritableMotionSystem(const size_t index)
	{
		MotionSystemPtr ms = GetOrCreateMotionSystem(index);
		if (ms == nullptr)
		{
			LOG_ERROR("Could not get or create motionSystem {:d}", index);
		}
		return ms;
	}

#define MOTION_SYSTEM_SETTER(funcName, valType, varName)                                                               \
	bool funcName(size_t index, valType val)                                                                           \
	{                                                                                                                  \
		MotionSystemPtr ms = getWritableMotionSystem(index);                                                           \
		if (ms == nullptr)                                                                                             \
		{                                                                                                              \
			return false;                                                                                              \
		}                                                                                                              \
		ms->varName = val;                                                                                             \
		return true;                                                                                                   \
	}

	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveAcceleration, float, currentMove.acceleration);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveDeceleration, float, currentMove.deceleration);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveDistance, float, currentMove.distance);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveDuration, float, currentMove.duration);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveExtrusionRate, float, currentMove.extrusionRate);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveRequestedSpeed, float, currentMove.requestedSpeed);
	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentMoveTopSpeed, float, currentMove.topSpeed);

	MOTION_SYSTEM_SETTER(SetMotionSystemCurrentTool, int32_t, currentTool);
	MOTION_SYSTEM_SETTER(SetMotionSystemNextTool, int32_t, nextTool);
	MOTION_SYSTEM_SETTER(SetMotionSystemPreviousTool, int32_t, previousTool);
	MOTION_SYSTEM_SETTER(SetMotionSystemPrintingAcceleration, float, printingAcceleration);
	MOTION_SYSTEM_SETTER(SetMotionSystemSpeedFactor, float, speedFactor);
	MOTION_SYSTEM_SETTER(SetMotionSystemTravelAcceleration, float, travelAcceleration);
	MOTION_SYSTEM_SETTER(SetMotionSystemVirtualEPos, float, virtualEPos);
	MOTION_SYSTEM_SETTER(SetMotionSystemWorkplaceNumber, int32_t, workplaceNumber);

	bool SetMotionSystemCurrentMoveLaserPwm(const size_t index, const float pwm)
	{
		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->currentMove.laserPwm = pwm;
		return true;
	}

	bool ClearMotionSystemCurrentMoveLaserPwm(const size_t index)
	{
		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->currentMove.laserPwm = std::nullopt;
		return true;
	}

	bool SetMotionSystemCurrentObject(const size_t index, const int32_t currentObject)
	{
		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->currentObject = currentObject;
		return true;
	}

	bool ClearMotionSystemCurrentObject(const size_t index)
	{
		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->currentObject = std::nullopt;
		return true;
	}

	bool SetMotionSystemRotationAngle(const size_t index, const float angle)
	{
		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->rotation.angle = angle;
		return true;
	}

	bool SetMotionSystemRotationCentre(const size_t index, const size_t centreIndex, const float value)
	{
		if (centreIndex >= 2)
		{
			return false;
		}

		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->rotation.centre[centreIndex] = value;
		return true;
	}

	bool SetMotionSystemUserPosition(const size_t index, const size_t axisIndex, const float position)
	{
		if (axisIndex >= MAX_TOTAL_AXES)
		{
			return false;
		}

		MotionSystemPtr ms = getWritableMotionSystem(index);
		if (ms == nullptr)
		{
			return false;
		}
		ms->userPosition[axisIndex] = position;
		return true;
	}

} // namespace OM::Move

#pragma once

#include "Subscribers/Subscribers.h"

class MoveSubscribers : public SubscriberMap
{
  public:
	MoveSubscribers()
	{
		addSubscriber("move:axes^:acceleration", acceleration);
		addSubscriber("move:axes^:babystep", babyStep);
		addSubscriber("move:axes^:homed", axisHomed);
		addSubscriber("move:axes^:letter", axisLetter);
		addSubscriber("move:axes^:machinePosition", axisMachinePosition);
		addSubscriber("move:axes^:min", axisMinPosition);
		addSubscriber("move:axes^:max", axisMaxPosition);
		addSubscriber("move:axes^:userPosition", axisUserPosition);
		addSubscriber("move:axes^:speed", axisMaxSpeed);
		addSubscriber("move:axes^:visible", axisVisible);
		addSubscriber("move:axes^:workplaceOffsets^", axisWorkplaceOffset);
		// addSubscriber("move:axes:next", axisNext);
		addSubscriber("move:extruders^:factor", extrusionFactor);
		addSubscriber("move:extruders^:filamentDiameter", extruderFilamentDiameter);
		addSubscriber("move:extruders^:filament", extruderFilamentName);
		addSubscriber("move:extruders^:position", extruderPosition);
		addSubscriber("move:extruders^:pressureAdvance", extruderPressureAdvance);
		addSubscriber("move:extruders^:speed", extruderMaxSpeed);
		addSubscriber("move:extruders^:stepsPerMm", extruderStepsPerMm);
		addSubscriber("move:kinematics:name", kinematicsName);
		addSubscriber("move:speedFactor", speedFactor);
		addSubscriber("move:workplaceNumber", workplaceNumber);
		addSubscriber("move:noMovesBeforeHoming", noMovesBeforeHoming);
		addSubscriber("move:printingAcceleration", printingAcceleration);
		addSubscriber("move:currentMove:requestedSpeed", currentMoveRequestedSpeed);
		addSubscriber("move:currentMove:topSpeed", currentMoveTopSpeed);
		addSubscriber("move:currentMove:extrusionRate", currentMoveExtrusionRate);
		addSubscriber("move:compensation:file", compensationFile);
		addSubscriber("inputs^:distanceUnit", distanceUnit);

		addSubscriber("move:motionSystems^:currentMove:extrusionRate", motionSystemCurrentMoveExtrusionRate);
		addSubscriber("move:motionSystems^:currentMove:acceleration", motionSystemCurrentMoveAcceleration);
		addSubscriber("move:motionSystems^:currentMove:deceleration", motionSystemCurrentMoveDeceleration);
		addSubscriber("move:motionSystems^:currentMove:distance", motionSystemCurrentMoveDistance);
		addSubscriber("move:motionSystems^:currentMove:duration", motionSystemCurrentMoveDuration);
		addSubscriber("move:motionSystems^:currentMove:laserPwm", motionSystemCurrentMoveLaserPwm);
		addSubscriber("move:motionSystems^:currentMove:requestedSpeed", motionSystemCurrentMoveRequestedSpeed);
		addSubscriber("move:motionSystems^:currentMove:topSpeed", motionSystemCurrentMoveTopSpeed);
		addSubscriber("move:motionSystems^:currentObject", motionSystemCurrentObject);
		addSubscriber("move:motionSystems^:currentTool", motionSystemCurrentTool);
		addSubscriber("move:motionSystems^:nextTool", motionSystemNextTool);
		addSubscriber("move:motionSystems^:previousTool", motionSystemPreviousTool);
		addSubscriber("move:motionSystems^:printingAcceleration", motionSystemPrintingAcceleration);
		addSubscriber("move:motionSystems^:rotation:angle", motionSystemRotationAngle);
		addSubscriber("move:motionSystems^:rotation:centre^", motionSystemRotationCentre);
		addSubscriber("move:motionSystems^:speedFactor", motionSystemSpeedFactor);
		addSubscriber("move:motionSystems^:travelAcceleration", motionSystemTravelAcceleration);
		addSubscriber("move:motionSystems^:userPosition^", motionSystemUserPosition);
		addSubscriber("move:motionSystems^:virtualEPos", motionSystemVirtualEPos);
		addSubscriber("move:motionSystems^:workplaceNumber", motionSystemWorkplaceNumber);

		addArrayEndSubscriber("move:axes^", axesArrayEnd);
		addArrayEndSubscriber("move:extruders^", extrudersArrayEnd);
		addArrayEndSubscriber("move:motionSystems^", motionSystemsArrayEnd);
	}

  private:
	static bool acceleration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool babyStep(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisHomed(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[]);
	static bool axisLetter(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool axisMachinePosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisMinPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisMaxPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisUserPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisMaxSpeed(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool axisVisible(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[]);
	static bool axisWorkplaceOffset(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool axisNext(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool extrusionFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool extruderFilamentDiameter(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool extruderFilamentName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool extruderPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool extruderPressureAdvance(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool extruderMaxSpeed(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool extruderStepsPerMm(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool kinematicsName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool speedFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool workplaceNumber(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool noMovesBeforeHoming(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[]);
	static bool printingAcceleration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[]);
	static bool currentMoveRequestedSpeed(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool currentMoveTopSpeed(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool currentMoveExtrusionRate(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool compensationFile(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool distanceUnit(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);

	static bool axesArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
	static bool extrudersArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);

	static bool motionSystemCurrentMoveExtrusionRate(Comm::JsonDecoder* decoder,
													 const float& data,
													 const size_t indices[]);
	static bool motionSystemCurrentMoveAcceleration(Comm::JsonDecoder* decoder,
													const float& data,
													const size_t indices[]);
	static bool motionSystemCurrentMoveDeceleration(Comm::JsonDecoder* decoder,
													const float& data,
													const size_t indices[]);
	static bool motionSystemCurrentMoveDistance(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemCurrentMoveDuration(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemCurrentMoveLaserPwm(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool motionSystemCurrentMoveRequestedSpeed(Comm::JsonDecoder* decoder,
													  const float& data,
													  const size_t indices[]);
	static bool motionSystemCurrentMoveTopSpeed(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemCurrentObject(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool motionSystemCurrentTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool motionSystemNextTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool motionSystemPreviousTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool motionSystemPrintingAcceleration(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemRotationAngle(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemRotationCentre(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemSpeedFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemTravelAcceleration(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemUserPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemVirtualEPos(Comm::JsonDecoder* decoder, const float& data, const size_t indices[]);
	static bool motionSystemWorkplaceNumber(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool motionSystemsArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
};

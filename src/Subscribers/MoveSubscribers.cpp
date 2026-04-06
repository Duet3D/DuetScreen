#include "Debug.h"

#include "Hardware/Duet.h"
#include "MoveSubscribers.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Heightmap.h"
#include "ObjectModel/MotionSystem.h"
#include "UI/Core/Model.h"

static size_t getAxisIndex(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	return decoder->GetArrayStartIndex() + indices[0];
}

bool MoveSubscribers::acceleration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAcceleration(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->babystep = {:d}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::babyStep(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetBabystepOffset(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->babystep = {:g}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisHomed(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisHomedStatus(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->homed = {:d}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisLetter(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisLetter(index, data[0]))
	{
		LOG_ERROR("Failed to set axis[{:d}]->letter = {:s}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisMachinePosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisMachinePosition(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->machinePosition = {:g}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisMinPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisMinPosition(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->min = {:g}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisMaxPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisMaxPosition(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->max = {:g}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisUserPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisUserPosition(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->userPosition = {:g}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisMaxSpeed(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisMaxSpeed(index, data / 60))
	{
		LOG_ERROR("Failed to set axis[{:d}]->maxSpeed = {:d}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisVisible(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisVisible(index, data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->visible = {:d}", index, data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axisWorkplaceOffset(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	if (!OM::Move::SetAxisWorkplaceOffset(index, indices[1], data))
	{
		LOG_ERROR("Failed to set axis[{:d}]->workplaceOffset[{:d}] = {:g}", index, indices[1], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extrusionFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderFactor(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->factor = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderFilamentDiameter(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderFilamentDiameter(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->filamentDiameter = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderFilamentName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderFilamentName(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->filamentName = {:s}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderPosition(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->position = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderPressureAdvance(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderPressureAdvance(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->pressureAdvance = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderMaxSpeed(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderMaxSpeed(indices[0], data / 60))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->maxSpeed = {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::extruderStepsPerMm(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetExtruderStepsPerMm(indices[0], data))
	{
		LOG_ERROR("Failed to set extruderAxis[{:d}]->stepsPerMm = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::kinematicsName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetKinematicsName(data);
	Model::get().post<EventType::KinematicsName>(OM::Move::GetKinematics().name);
	return true;
}

bool MoveSubscribers::speedFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetSpeedFactor(data);
	Model::get().post<EventType::SpeedFactor>();
	return true;
}

bool MoveSubscribers::workplaceNumber(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	if (!OM::Move::SetCurrentWorkplaceNumber((uint8_t)data))
	{
		LOG_ERROR("Failed to set workplace number = {:d}", data);
		return false;
	}
	Model::get().post<EventType::WorkplaceNumber>();
	return true;
}

bool MoveSubscribers::noMovesBeforeHoming(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetNoMovesBeforeHoming(data);
	Model::get().post<EventType::NoMoveBeforeHoming>();
	return true;
}

bool MoveSubscribers::printingAcceleration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetPrintingAcceleration(data);
	Model::get().post<EventType::PrintingAcceleration>(OM::Move::GetPrintingAcceleration());
	return true;
}

bool MoveSubscribers::currentMoveRequestedSpeed(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetCurrentMoveRequestedSpeed(data);
	Model::get().post<EventType::CurrentMoveRequestedSpeed>();
	return true;
}

bool MoveSubscribers::currentMoveTopSpeed(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetCurrentMoveTopSpeed(data);
	Model::get().post<EventType::CurrentMoveTopSpeed>();
	return true;
}

bool MoveSubscribers::currentMoveExtrusionRate(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::Move::SetExtrusionRate(data);
	Model::get().post<EventType::CurrentMoveExtrusionSpeed>();
	return true;
}

bool MoveSubscribers::compensationFile(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetCurrentHeightmap(data == nullptr ? "" : data);
	Model::get().post<EventType::CompensationFile>();
	return true;
}

bool MoveSubscribers::distanceUnit(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	LOG_DBG("New distance unit: {:s}, channel: {:d}", data, indices[0]);
	if (strcmp(data, "mm") == 0)
	{
		OM::Move::SetDistanceUnit(indices[0], Units::UnitSystem::Metric);
	}
	else if (strcmp(data, "in") == 0)
	{
		OM::Move::SetDistanceUnit(indices[0], Units::UnitSystem::Imperial);
	}
	else
	{
		LOG_ERROR("Unknown distance unit: {:s}", data);
		return false;
	}
	return true;
}

bool MoveSubscribers::axesArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	const Comm::Seq* seq = decoder->GetSeq();
	if (seq && seq->seqid == Comm::rcvSeqsMove && indices[0] >= MAX_REPORTED_AXES)
	{
		/**
		 * There might be more axes available
		 *
		 * TODO: this won't handle the case where the single `move.axes` request still doesn't return all axes but it
		 * can handle 10 currently. Will need to check the `next` field in the response to handle more than that.
		 * Likely I will postpone handling that until the Duet comms are reworked to handle request/response IDs (#69)
		 */
		Comm::DUET.RequestModel("move.axes", fmt::format("vna{:d}", index));
		return true;
	}
	OM::Move::RemoveAxis(index, true);
	Model::get().post<EventType::AxesData>();
	return true;
}

/* Not used */
bool MoveSubscribers::axisNext(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	const size_t index = getAxisIndex(decoder, indices);
	LOG_DBG("Next axis index: {:d} (current index: {:d})", data, index);
	if (data != 0)
	{
		// This means there are more axes to come, so we should request the next one
		Comm::DUET.RequestModel("move.axes", fmt::format("vna{:d}", data));
	}
	return true;
}

bool MoveSubscribers::extrudersArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::Move::RemoveExtruderAxis(indices[0], true);
	Model::get().post<EventType::ExtruderData>();
	return true;
}

bool MoveSubscribers::motionSystemCurrentTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentTool(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentTool = {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentObject(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);

	if (data == nullptr)
	{
		if (!OM::Move::ClearMotionSystemCurrentObject(indices[0]))
		{
			LOG_ERROR("Failed to clear motionSystem[{:d}]->currentObject", indices[0]);
			return false;
		}
		return true;
	}

	int32_t currentObject = -1;
	if (!Comm::GetInteger(data, currentObject))
	{
		LOG_ERROR("Failed to parse motionSystem[{:d}]->currentObject = {:s}", indices[0], data);
		return false;
	}

	if (!OM::Move::SetMotionSystemCurrentObject(indices[0], currentObject))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentObject = {:d}", indices[0], currentObject);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemNextTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemNextTool(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->nextTool = {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemPreviousTool(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemPreviousTool(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->previousTool = {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemPrintingAcceleration(Comm::JsonDecoder* decoder,
													   const float& data,
													   const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemPrintingAcceleration(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->printingAcceleration = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemTravelAcceleration(Comm::JsonDecoder* decoder,
													 const float& data,
													 const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemTravelAcceleration(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->travelAcceleration = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemRotationAngle(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemRotationAngle(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->rotation.angle = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemRotationCentre(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemRotationCentre(indices[0], indices[1], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->rotation.centre[{:d}] = {:g}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemSpeedFactor(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemSpeedFactor(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->speedFactor = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemWorkplaceNumber(Comm::JsonDecoder* decoder,
												  const int32_t& data,
												  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemWorkplaceNumber(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->workplaceNumber = {:d}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemUserPosition(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemUserPosition(indices[0], indices[1], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->userPosition[{:d}] = {:g}", indices[0], indices[1], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveAcceleration(Comm::JsonDecoder* decoder,
														  const float& data,
														  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveAcceleration(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.acceleration = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveDeceleration(Comm::JsonDecoder* decoder,
														  const float& data,
														  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveDeceleration(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.deceleration = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveDistance(Comm::JsonDecoder* decoder,
													  const float& data,
													  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveDistance(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.distance = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveDuration(Comm::JsonDecoder* decoder,
													  const float& data,
													  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveDuration(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.duration = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveLaserPwm(Comm::JsonDecoder* decoder,
													  const char* data,
													  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);

	if (data == nullptr)
	{
		if (!OM::Move::ClearMotionSystemCurrentMoveLaserPwm(indices[0]))
		{
			LOG_ERROR("Failed to clear motionSystem[{:d}]->currentMove.laserPwm", indices[0]);
			return false;
		}
		return true;
	}

	float laserPwm = 0.0f;
	if (!Comm::GetFloat(data, laserPwm))
	{
		LOG_ERROR("Failed to parse motionSystem[{:d}]->currentMove.laserPwm = {:s}", indices[0], data);
		return false;
	}

	if (!OM::Move::SetMotionSystemCurrentMoveLaserPwm(indices[0], laserPwm))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.laserPwm = {:g}", indices[0], laserPwm);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemVirtualEPos(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemVirtualEPos(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->virtualEPos = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveRequestedSpeed(Comm::JsonDecoder* decoder,
															const float& data,
															const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveRequestedSpeed(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.requestedSpeed = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveTopSpeed(Comm::JsonDecoder* decoder,
													  const float& data,
													  const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveTopSpeed(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.topSpeed = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemCurrentMoveExtrusionRate(Comm::JsonDecoder* decoder,
														   const float& data,
														   const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (!OM::Move::SetMotionSystemCurrentMoveExtrusionRate(indices[0], data))
	{
		LOG_ERROR("Failed to set motionSystem[{:d}]->currentMove.extrusionRate = {:g}", indices[0], data);
		return false;
	}
	return true;
}

bool MoveSubscribers::motionSystemsArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::Move::RemoveMotionSystem(indices[0], true);
	Model::get().post<EventType::MotionSystemData>();
	return true;
}

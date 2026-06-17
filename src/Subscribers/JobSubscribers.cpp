#include "Debug.h"

#include "JobSubscribers.h"
#include "ObjectModel/Job.h"
#include "UI/Core/Model.h"

bool JobSubscribers::currentFileName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetJobName(data);
	Model::get().post<EventType::JobFileName>(OM::GetJobName());
	return true;
}

bool JobSubscribers::lastFileName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetLastJobName(data);
	Model::get().post<EventType::JobLastFileName>(OM::GetLastJobName());
	return true;
}

bool JobSubscribers::printTime(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetPrintTime(data);
	Model::get().post<EventType::JobPrintTime>();
	return true;
}

bool JobSubscribers::simulatedTime(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	uint32_t val = 0;
	Comm::GetUnsignedInteger(data, val);
	OM::SetSimulatedTime(val);
	Model::get().post<EventType::JobPrintTime>();
	return true;
}

bool JobSubscribers::height(Comm::JsonDecoder* decoder, const float& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetPrintHeight(data);
	Model::get().post<EventType::JobHeight>();
	return true;
}

bool JobSubscribers::duration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetPrintDuration(data);
	Model::get().post<EventType::JobDuration>();
	if (OM::GetSimulatedTime() > 0)
	{
		OM::SetPrintRemaining(OM::RemainingTimeType::SIMULATED,
							  OM::GetSimulatedTime() - (OM::GetPrintDuration() - OM::GetWarmUpDuration()));
		Model::get().post<EventType::JobTimeLeft>();
	}
	return true;
}

bool JobSubscribers::filamentTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	uint32_t val = 0;
	Comm::GetUnsignedInteger(data, val);
	OM::SetPrintRemaining(OM::RemainingTimeType::FILAMENT, val);
	Model::get().post<EventType::JobTimeLeft>();
	return true;
}

bool JobSubscribers::fileTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	uint32_t val = 0;
	Comm::GetUnsignedInteger(data, val);
	OM::SetPrintRemaining(OM::RemainingTimeType::FILE, val);
	Model::get().post<EventType::JobTimeLeft>();
	return true;
}

bool JobSubscribers::slicerTimeLeft(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	uint32_t val = 0;
	Comm::GetUnsignedInteger(data, val);
	OM::SetPrintRemaining(OM::RemainingTimeType::SLICER, val);
	Model::get().post<EventType::JobTimeLeft>();
	return true;
}

bool JobSubscribers::filePosition(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetFilePosition(data);
	Model::get().post<EventType::JobFileProgress>();
	return true;
}

bool JobSubscribers::fileSize(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetFileSize(data);
	Model::get().post<EventType::JobFileProgress>();
	return true;
}

bool JobSubscribers::warmUpDuration(Comm::JsonDecoder* decoder, const uint32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetWarmUpDuration(data);
	Model::get().post<EventType::JobWarmupDuration>();
	return true;
}

bool JobSubscribers::nullBuild(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(data);
	LOG_DBG("Job: build is null");
	OM::RemoveJobObject(indices[0], true);
	Model::get().post<EventType::JobBuild>();
	return true;
}

bool JobSubscribers::currentObject(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(indices);
	OM::SetCurrentJobObject(static_cast<int8_t>(data));
	Model::get().post<EventType::JobCurrentObject>();
	return true;
}

bool JobSubscribers::nullObject(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(data);
	OM::RemoveJobObject(indices[0], false);
	Model::get().post<EventType::JobObjectData>();
	return true;
}

bool JobSubscribers::objectCancelled(Comm::JsonDecoder* decoder, const bool& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	auto jobObject = OM::GetOrCreateJobObject(indices[0]);
	if (jobObject == nullptr)
	{
		LOG_WARN("Job object {:d} not found", indices[0]);
		return false;
	}
	jobObject->cancelled = data;
	return true;
}

bool JobSubscribers::objectName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	auto jobObject = OM::GetOrCreateJobObject(indices[0]);
	if (jobObject == nullptr)
	{
		LOG_WARN("Job object {:d} not found", indices[0]);
		return false;
	}
	jobObject->name = data;
	return true;
}

bool JobSubscribers::objectX(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	auto jobObject = OM::GetOrCreateJobObject(indices[0]);
	if (jobObject == nullptr)
	{
		LOG_WARN("Job object {:d} not found", indices[0]);
		return false;
	}
	if (indices[1] >= 2)
	{
		LOG_WARN("Job object {:d} x index {:d} out of range", indices[0], indices[1]);
		return false;
	}
	jobObject->bounds.x[indices[1]] = data;
	return true;
}

bool JobSubscribers::objectY(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	auto jobObject = OM::GetOrCreateJobObject(indices[0]);
	if (jobObject == nullptr)
	{
		LOG_WARN("Job object {:d} not found", indices[0]);
		return false;
	}
	if (indices[1] >= 2)
	{
		LOG_WARN("Job object {:d} y index {:d} out of range", indices[0], indices[1]);
		return false;
	}
	jobObject->bounds.y[indices[1]] = data;
	return true;
}

bool JobSubscribers::objectArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::RemoveJobObject(indices[0], true);
	Model::get().post<EventType::JobObjectData>();
	return true;
}

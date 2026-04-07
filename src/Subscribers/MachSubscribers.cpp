/*
 * MachSubscribers.cpp
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#include "MachSubscribers.h"
#include "Debug.h"
#include "ObjectModel/MACH.h"
#include "UI/Core/Model.h"

// ── global.jobs ───────────────────────────────────────────────────────────────

bool MachSubscribers::jobs(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::MACH::SetJob(indices[0], data); // data == nullptr → clears the slot
	LOG_DBG("global.jobs[{:d}] = {:s}", indices[0], data ? data : "null");
	return true;
}

bool MachSubscribers::jobsArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::MACH::ClearJobsFrom(indices[0]);
	Model::get().post<EventType::MachJobs>();
	return true;
}

// ── global.currentJob ────────────────────────────────────────────────────────

bool MachSubscribers::currentJob(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[0] >= 2)
	{
		LOG_WARN("global.currentJob index {:d} out of range", indices[0]);
		return false;
	}
	OM::MACH::SetCurrentJob(indices[0], data);
	LOG_DBG("global.currentJob[{:d}] = {:s}", indices[0], data ? data : "null");
	Model::get().post<EventType::MachCurrentJob>();
	return true;
}

// ── global.nextJob ────────────────────────────────────────────────────────────

bool MachSubscribers::nextJob(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[0] >= 2)
	{
		LOG_WARN("global.nextJob index {:d} out of range", indices[0]);
		return false;
	}
	OM::MACH::SetNextJob(indices[0], data);
	LOG_DBG("global.nextJob[{:d}] = {:s}", indices[0], data ? data : "null");
	Model::get().post<EventType::MachNextJob>();
	return true;
}

// ── global.job_state ─────────────────────────────────────────────────────────

bool MachSubscribers::jobState(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[0] >= 2)
	{
		LOG_WARN("global.job_state index {:d} out of range", indices[0]);
		return false;
	}
	if (data == nullptr)
	{
		LOG_WARN("global.job_state[{:d}] is null but expected non-null string", indices[0]);
		return false;
	}
	OM::MACH::SetJobState(indices[0], data);
	LOG_DBG("global.job_state[{:d}] = {:s}", indices[0], data);
	Model::get().post<EventType::MachJobState>();
	return true;
}

// ── global.jobHistory ────────────────────────────────────────────────────────

// Fires when an outer entry global.jobHistory[i] is null.
bool MachSubscribers::jobHistoryNull(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	UNUSED(data);
	OM::MACH::ClearJobHistoryEntry(indices[0]);
	LOG_DBG("global.jobHistory[{:d}] = null", indices[0]);
	return true;
}

// Fires for inner string elements (indices[1] == 0 → name).
bool MachSubscribers::jobHistoryName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[1] != 0)
	{
		return false;
	}
	OM::MACH::SetJobHistoryName(indices[0], data);
	LOG_DBG("global.jobHistory[{:d}].name = {:s}", indices[0], data ? data : "null");
	return true;
}

// Fires for inner integer elements (indices[1] == 1 → counter).
bool MachSubscribers::jobHistoryCount(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	if (indices[1] != 1)
	{
		return false;
	}
	OM::MACH::SetJobHistoryCount(indices[0], data);
	LOG_DBG("global.jobHistory[{:d}].count = {:d}", indices[0], data);
	return true;
}

bool MachSubscribers::jobHistoryArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[])
{
	ZoneScoped;
	UNUSED(decoder);
	OM::MACH::ClearJobHistoryFrom(indices[0]);
	Model::get().post<EventType::MachJobHistory>();
	return true;
}

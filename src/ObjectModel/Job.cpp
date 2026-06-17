/*
 * Job.cpp
 *
 *  Created on: 10 Jan 2024
 *      Author: Andy Everitt
 */

#include "Debug.h"

#include "Configuration.h"
#include "Duet3D/General/String.h"
#include "Duet3D/General/Vector.h"
#include "Hardware/Duet.h"
#include "Job.h"
#include "ListHelpers.h"
#include "nameof.hpp"

#define ATTR_SETTR_GETTR(funcName, type, varName)                                                                      \
	void Set##funcName(const type value)                                                                               \
	{                                                                                                                  \
		varName = value;                                                                                               \
	}                                                                                                                  \
	type Get##funcName()                                                                                               \
	{                                                                                                                  \
		return varName;                                                                                                \
	}

namespace OM
{
	typedef Vector<std::shared_ptr<JobObject>, MAX_TRACKED_OBJECTS> JobObjectList;
	static JobObjectList s_jobObjects;
	static int8_t s_currentJobObjectIndex = -1;

	static std::string s_jobName;
	static std::string s_lastJobName;
	static uint32_t s_printTime = 0;
	static uint32_t s_simulatedTime = 0;
	static uint32_t s_printDuration = 0;
	static uint32_t s_warmUpDuration = 0;
	static float s_printHeight = 0.0f;
	static uint32_t s_filePosition = 0;
	static uint32_t s_fileSize = 0;
	static struct
	{
		uint32_t filament = 0;
		uint32_t file = 0;
		uint32_t slicer = 0;
		uint32_t simulated = 0;
	} s_printRemaining;

	ATTR_SETTR_GETTR(PrintTime, uint32_t, s_printTime)
	ATTR_SETTR_GETTR(SimulatedTime, uint32_t, s_simulatedTime)
	ATTR_SETTR_GETTR(PrintDuration, uint32_t, s_printDuration)
	ATTR_SETTR_GETTR(WarmUpDuration, uint32_t, s_warmUpDuration)
	ATTR_SETTR_GETTR(PrintHeight, float, s_printHeight)
	ATTR_SETTR_GETTR(FilePosition, uint32_t, s_filePosition)
	ATTR_SETTR_GETTR(FileSize, uint32_t, s_fileSize)

	void SetPrintRemaining(RemainingTimeType type, const uint32_t printRemaining)
	{
		switch (type)
		{
		case RemainingTimeType::FILAMENT:
			s_printRemaining.filament = printRemaining;
			break;
		case RemainingTimeType::FILE:
			s_printRemaining.file = printRemaining;
			break;
		case RemainingTimeType::SLICER:
			s_printRemaining.slicer = printRemaining;
			break;
		case RemainingTimeType::SIMULATED:
			s_printRemaining.simulated = printRemaining;
			break;
		case RemainingTimeType::AUTO:
			LOG_WARN("AUTO is not a valid type for SetPrintRemaining\n");
			break;
		}
	}

	uint32_t GetPrintRemaining(RemainingTimeType type)
	{
		switch (type)
		{
		case RemainingTimeType::FILAMENT:
			return s_printRemaining.filament;
		case RemainingTimeType::FILE:
			return s_printRemaining.file;
		case RemainingTimeType::SLICER:
			return s_printRemaining.slicer;
		case RemainingTimeType::SIMULATED:
			return s_printRemaining.simulated;
		case RemainingTimeType::AUTO:
			if (s_printRemaining.simulated > 0)
			{
				return s_printRemaining.simulated;
			}
			if (s_printRemaining.slicer > 0)
			{
				return s_printRemaining.slicer;
			}
			if (s_printRemaining.filament > 0)
			{
				return s_printRemaining.filament;
			}
			return s_printRemaining.file;
		}

		LOG_FATAL_THROW("Invalid RemainingTimeType: {:d}", (int)type);
		std::unreachable();
	}

	void SetJobName(const char* name)
	{
		s_jobName = name;
	}

	const std::string& GetJobName()
	{
		return s_jobName;
	}

	void SetLastJobName(const char* name)
	{
		s_lastJobName = name;
	}

	const std::string& GetLastJobName()
	{
		return s_lastJobName;
	}

	void JobObject::Reset()
	{
		index = 0;
		cancelled = false;
		name.clear();
		bounds.x[0] = 0;
		bounds.x[1] = 0;
		bounds.y[0] = 0;
		bounds.y[1] = 0;
	}

	void SetCurrentJobObject(int8_t index)
	{
		if (index >= (int)MAX_TRACKED_OBJECTS)
		{
			LOG_WARN("JobObject index {:d} out of range\n", index);
			index = -1;
		}
		LOG_VERBOSE("Setting current job object index to {:d}", index);
		s_currentJobObjectIndex = index;
	}

	int8_t GetCurrentJobObjectIndex()
	{
		return s_currentJobObjectIndex;
	}

	std::shared_ptr<JobObject> GetJobObject(const size_t index)
	{
		if (index >= MAX_TRACKED_OBJECTS)
		{
			LOG_WARN("JobObject index {:d} out of range\n", index);
			return nullptr;
		}
		return GetOrCreate<JobObjectList, JobObject>(s_jobObjects, index, false);
	}

	std::shared_ptr<JobObject> GetOrCreateJobObject(const size_t index)
	{
		if (index >= MAX_TRACKED_OBJECTS)
		{
			LOG_WARN("JobObject index {:d} out of range\n", index);
			return nullptr;
		}
		return GetOrCreate<JobObjectList, JobObject>(s_jobObjects, index, true);
	}

	size_t GetJobObjectCount()
	{
		return s_jobObjects.Size();
	}

	bool IterateJobObjectsWhile(function_ref<bool(std::shared_ptr<JobObject>, size_t)> func, const size_t startAt)
	{
		return s_jobObjects.IterateWhile(func, startAt);
	}

	size_t RemoveJobObject(const size_t index, const bool allFollowing)
	{
		return Remove<JobObjectList, JobObject>(s_jobObjects, index, allFollowing);
	}

	size_t ClearJobObjects()
	{
		size_t count = GetJobObjectCount();
		s_jobObjects.Clear();
		return count - GetJobObjectCount();
	}

	bool IsJobObjectActive(const size_t index)
	{
		auto jobObject = GetJobObject(index);
		return jobObject != nullptr && !jobObject->cancelled;
	}

	void SetJobObjectActive(const size_t index, const bool active)
	{
		auto jobObject = GetJobObject(index);
		if (jobObject == nullptr)
		{
			LOG_WARN("Failed to get job object {:d}\n", index);
			return;
		};
		Comm::DUET.SendGcodef("M486 {:c}{:d}\n", active ? 'U' : 'P', index);
	}

	void CancelCurrentJobObject()
	{
		Comm::DUET.SendGcode("M486 C\n");
	}
} // namespace OM

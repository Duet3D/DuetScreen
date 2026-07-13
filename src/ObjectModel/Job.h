/*
 * Job.h
 *
 *  Created on: 10 Jan 2024
 *      Author: Andy Everitt
 */

#ifndef JNI_OBJECTMODEL_JOB_HPP_
#define JNI_OBJECTMODEL_JOB_HPP_

#include "Duet3D/General/FreelistManager.h"
#include "Duet3D/General/StringRef.h"
#include "Duet3D/General/function_ref.h"
#include <memory>
#include <string>
#include <sys/types.h>

namespace OM
{
	struct JobObject
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<JobObject>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<JobObject>(p); }

		struct Point
		{
			int32_t x[2];
			int32_t y[2];
		};

		size_t index;
		bool cancelled;
		std::string name;
		Point bounds;

		void Reset();
	};

	enum class RemainingTimeType
	{
		FILAMENT = 0,
		FILE,
		SLICER,
		SIMULATED,
		AUTO
	};

	enum class JobProgressSource
	{
		DURATION = 0,
		FILE = 1
	};

	void SetJobName(const char* name);
	const std::string& GetJobName();

	void SetLastJobName(const char* name);
	const std::string& GetLastJobName();

	void SetPrintTime(const uint32_t printTime);
	uint32_t GetPrintTime();

	void SetSimulatedTime(const uint32_t simulatedTime);
	uint32_t GetSimulatedTime();

	void SetPrintDuration(const uint32_t printDuration);
	uint32_t GetPrintDuration();

	void SetWarmUpDuration(const uint32_t warmUpDuration);
	uint32_t GetWarmUpDuration();

	void SetPrintHeight(const float height);
	float GetPrintHeight();

	void SetFilePosition(const uint32_t filePosition);
	uint32_t GetFilePosition();

	void SetFileSize(const uint32_t fileSize);
	uint32_t GetFileSize();

	void SetPrintRemaining(RemainingTimeType type, const uint32_t printRemaining);
	uint32_t GetPrintRemaining(RemainingTimeType type);

	void SetCurrentJobObject(int8_t index);
	int8_t GetCurrentJobObjectIndex();

	std::shared_ptr<JobObject> GetJobObject(const size_t index);
	std::shared_ptr<JobObject> GetOrCreateJobObject(const size_t index);
	size_t GetJobObjectCount();
	bool IterateJobObjectsWhile(function_ref<bool(std::shared_ptr<JobObject>, size_t)> func, const size_t startAt = 0);
	size_t RemoveJobObject(const size_t index, const bool allFollowing);
	size_t ClearJobObjects();

	bool IsJobObjectActive(const size_t index);
	void SetJobObjectActive(const size_t index, const bool active);
	void CancelCurrentJobObject();
} // namespace OM

#endif /* JNI_OBJECTMODEL_JOB_HPP_ */

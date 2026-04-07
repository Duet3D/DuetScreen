/*
 * MACH.h
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace OM::MACH
{
	// global.jobs — nullable array of job name strings
	// Passing nullptr for name clears that slot.
	void SetJob(size_t index, const char* name);
	void ClearJobsFrom(size_t firstIndex);
	size_t GetJobCount();
	// Returns std::nullopt if the slot was null or index is out of range.
	[[nodiscard]] std::optional<std::string_view> GetJob(size_t index);
	[[nodiscard]] const std::vector<std::optional<std::string>>& GetJobs();

	// global.currentJob — nullable string[2]
	// Passing nullptr for value clears that slot.
	void SetCurrentJob(size_t index, const char* value);
	void ClearCurrentJob();
	// Returns std::nullopt if the slot was null or index is out of range.
	[[nodiscard]] std::optional<std::string_view> GetCurrentJob(size_t index);

	// global.nextJob — nullable string[2]
	// Passing nullptr for value clears that slot.
	void SetNextJob(size_t index, const char* value);
	void ClearNextJob();
	// Returns std::nullopt if the slot was null or index is out of range.
	[[nodiscard]] std::optional<std::string_view> GetNextJob(size_t index);

	// global.jobState — non-nullable string[2]
	void SetJobState(size_t index, std::string_view value);
	void ClearJobState();
	[[nodiscard]] std::string_view GetJobState(size_t index);

	// global.jobHistory — array of nullable [name, count] pairs
	struct JobHistoryEntry
	{
		std::string name;
		int32_t count = 0;
	};

	// Setting name to nullptr marks that history slot as invalid.
	void SetJobHistoryName(size_t index, const char* name);
	void SetJobHistoryCount(size_t index, int32_t count);
	void ClearJobHistoryEntry(size_t index);
	void ClearJobHistoryFrom(size_t firstIndex);
	size_t GetJobHistoryCount();
	[[nodiscard]] std::vector<JobHistoryEntry> GetJobHistory();
	// Returns std::nullopt if the entry at index is absent or was marked invalid (null).
	[[nodiscard]] std::optional<JobHistoryEntry> GetJobHistoryEntry(size_t index);

} // namespace OM::MACH

/*
 * MACH.cpp
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#include "MACH.h"
#include "Debug.h"
#include <optional>
#include <vector>

namespace OM::MACH
{
	// global.jobs storage — nullopt means the JSON element was null
	static std::vector<std::optional<std::string>> s_jobs;

	// global.currentJob / global.nextJob storage — nullopt means the JSON element was null
	static std::array<std::optional<std::string>, 2> s_currentJob;
	static std::array<std::optional<std::string>, 2> s_nextJob;
	static std::array<std::string, 2> s_jobState;

	// global.jobHistory storage - maps to s_jobs
	static std::vector<int32_t> s_jobHistory;

	// ── global.jobs ──────────────────────────────────────────────────────────

	void SetJob(size_t index, const char* name)
	{
		if (index >= s_jobs.size())
		{
			s_jobs.resize(index + 1);
		}
		if (name != nullptr)
		{
			s_jobs[index] = name;
		}
		else
		{
			s_jobs[index] = std::nullopt;
		}
	}

	void ClearJobsFrom(size_t firstIndex)
	{
		if (firstIndex < s_jobs.size())
		{
			s_jobs.resize(firstIndex);
		}
	}

	size_t GetJobCount()
	{
		return s_jobs.size();
	}

	std::optional<std::string_view> GetJob(size_t index)
	{
		if (index < s_jobs.size() && s_jobs[index].has_value())
		{
			return std::string_view{*s_jobs[index]};
		}
		return std::nullopt;
	}

	const std::vector<std::optional<std::string>>& GetJobs()
	{
		return s_jobs;
	}

	// ── global.currentJob ────────────────────────────────────────────────────

	void SetCurrentJob(size_t index, const char* value)
	{
		if (index < s_currentJob.size())
		{
			s_currentJob[index] = value ? std::optional<std::string>{value} : std::nullopt;
		}
	}

	void ClearCurrentJob()
	{
		s_currentJob.fill(std::nullopt);
	}

	std::optional<std::string_view> GetCurrentJob(size_t index)
	{
		if (index < s_currentJob.size() && s_currentJob[index].has_value())
		{
			return std::string_view{*s_currentJob[index]};
		}
		return std::nullopt;
	}

	// ── global.nextJob ───────────────────────────────────────────────────────

	void SetNextJob(size_t index, const char* value)
	{
		if (index < s_nextJob.size())
		{
			s_nextJob[index] = value ? std::optional<std::string>{value} : std::nullopt;
		}
	}

	void ClearNextJob()
	{
		s_nextJob.fill(std::nullopt);
	}

	std::optional<std::string_view> GetNextJob(size_t index)
	{
		if (index < s_nextJob.size() && s_nextJob[index].has_value())
		{
			return std::string_view{*s_nextJob[index]};
		}
		return std::nullopt;
	}

	// ── global.jobState ──────────────────────────────────────────────────────

	void SetJobState(size_t index, std::string_view value)
	{
		if (index < s_jobState.size())
		{
			s_jobState[index] = value;
		}
	}

	void ClearJobState()
	{
		s_jobState.fill(std::string{});
	}

	std::string_view GetJobState(size_t index)
	{
		if (index < s_jobState.size())
		{
			return std::string_view{s_jobState[index]};
		}
		return {};
	}

	// ── global.jobHistory ────────────────────────────────────────────────────

	void SetJobHistoryCount(size_t index, int32_t count)
	{
		if (index >= s_jobHistory.size())
		{
			s_jobHistory.resize(index + 1);
		}
		s_jobHistory[index] = count;
	}

	void ClearJobHistoryFrom(size_t firstIndex)
	{
		if (firstIndex < s_jobHistory.size())
		{
			s_jobHistory.resize(firstIndex);
		}
	}

	size_t GetJobHistoryCount()
	{
		return s_jobHistory.size();
	}

	std::vector<JobHistoryEntry> GetJobHistory()
	{
		std::vector<JobHistoryEntry> history;
		history.reserve(s_jobs.size());
		for (size_t i = 0; i < s_jobs.size(); i++)
		{
			std::optional<std::string>& job = s_jobs[i];
			if (job.has_value())
			{
				JobHistoryEntry entry;
				entry.name = *job;

				entry.count = i < s_jobHistory.size() ? s_jobHistory[i] : 0;

				history.push_back(entry);
			}
		}
		return history;
	}

} // namespace OM::MACH

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

	// global.jobHistory storage — nullopt means the outer JSON element was null
	static std::vector<std::optional<JobHistoryEntry>> s_jobHistory;

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

	// ── global.jobHistory ────────────────────────────────────────────────────

	void SetJobHistoryName(size_t index, const char* name)
	{
		if (index >= s_jobHistory.size())
		{
			s_jobHistory.resize(index + 1);
		}
		if (name != nullptr)
		{
			if (!s_jobHistory[index].has_value())
			{
				s_jobHistory[index].emplace();
			}
			s_jobHistory[index]->name = name;
		}
		else
		{
			s_jobHistory[index] = std::nullopt;
		}
	}

	void SetJobHistoryCount(size_t index, int32_t count)
	{
		if (index >= s_jobHistory.size())
		{
			s_jobHistory.resize(index + 1);
		}
		if (!s_jobHistory[index].has_value())
		{
			s_jobHistory[index].emplace();
		}
		s_jobHistory[index]->count = count;
	}

	void ClearJobHistoryEntry(size_t index)
	{
		if (index < s_jobHistory.size())
		{
			s_jobHistory[index] = std::nullopt;
		}
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
		history.reserve(s_jobHistory.size());
		for (const auto& entry : s_jobHistory)
		{
			if (entry.has_value())
			{
				history.push_back(*entry);
			}
		}
		return history;
	}

	std::optional<JobHistoryEntry> GetJobHistoryEntry(size_t index)
	{
		if (index < s_jobHistory.size() && s_jobHistory[index].has_value())
		{
			return s_jobHistory[index];
		}
		return std::nullopt;
	}

} // namespace OM::MACH

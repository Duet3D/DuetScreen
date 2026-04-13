/*
 * JobSelectPresenter.cpp
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#include "JobSelectPresenter.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "JobSelectView.h"
#include "ObjectModel/MACH.h"
#include <regex>

namespace UI
{
	void JobSelectPresenter::selectNextJob(std::string_view jobName)
	{
		std::string escapedJobName(jobName);
		escapedJobName = std::regex_replace(escapedJobName, std::regex("\""), "\"\"");
		escapedJobName = std::regex_replace(escapedJobName, std::regex("\'"), "\'\'");
		Comm::DUET.SendGcodef("set global.next_job[0] = \"{:s}\"\n", escapedJobName);
	}

	void JobSelectPresenter::onActivate()
	{
		newJobs();
		newCurrentJob();
		newNextJob();
		newJobState();
		newJobHistory();
	}

	void JobSelectPresenter::onInit()
	{
		registerEventListener<EventType::MachJobs>(this, &JobSelectPresenter::newJobs);
		registerEventListener<EventType::MachCurrentJob>(this, &JobSelectPresenter::newCurrentJob);
		registerEventListener<EventType::MachNextJob>(this, &JobSelectPresenter::newNextJob);
		registerEventListener<EventType::MachJobState>(this, &JobSelectPresenter::newJobState);
		registerEventListener<EventType::MachJobHistory>(this, &JobSelectPresenter::newJobHistory);
	}

	void JobSelectPresenter::newJobs()
	{
		LOG_DBG("Received new MachJobs event");
		getView()->setJobs(OM::MACH::GetJobs());
	}

	void JobSelectPresenter::newCurrentJob()
	{
		LOG_DBG("Received new MachCurrentJob event");
		for (size_t i = 0; i < 2; i++)
		{
			auto jobName = OM::MACH::GetCurrentJob(i).value_or("No Job");
			getView()->setCurrentJob(i, jobName);
		}
	}

	void JobSelectPresenter::newNextJob()
	{
		LOG_DBG("Received new MachNextJob event");
		for (size_t i = 0; i < 2; i++)
		{
			auto jobName = OM::MACH::GetNextJob(i).value_or("");
			getView()->setNextJob(i, jobName);
		}
	}

	void JobSelectPresenter::newJobState()
	{
		LOG_DBG("Received new MachJobState event");
		for (size_t i = 0; i < 2; i++)
		{
			getView()->setCurrentJobState(i, OM::MACH::GetJobState(i));
		}
	}

	void JobSelectPresenter::newJobHistory()
	{
		LOG_DBG("Received new MachJobHistory event");
		getView()->updateJobHistory(OM::MACH::GetJobHistory());
	}
} // namespace UI

/*
 * MachSubscribers.h
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#pragma once

#include "Subscribers/Subscribers.h"

class MachSubscribers : public SubscriberMap
{
  public:
	MachSubscribers()
	{
		// global.jobs — nullable string[]
		addSubscriber("global:jobs^", jobs);
		addArrayEndSubscriber("global:jobs^", jobsArrayEnd);

		// global.currentJob — nullable string[2]
		addSubscriber("global:current_job^", currentJob);

		// global.nextJob — nullable string[2]
		addSubscriber("global:next_job^", nextJob);

		// global.job_state — non-nullable string[2]
		addSubscriber("global:job_state^", jobState);

		// global.jobHistory — [[name, count], ...] with null slots
		// global:job_history^  fires when an outer element is null
		// global:job_history^^ fires for each inner element (string name or int count)
		addSubscriber("global:job_history^", jobHistoryNull);
		addSubscriber("global:job_history^^", jobHistoryName);	// const char*  → indices[1] == 0
		addSubscriber("global:job_history^^", jobHistoryCount); // const int32_t& → indices[1] == 1
		addArrayEndSubscriber("global:job_history^", jobHistoryArrayEnd);
	}

  private:
	static bool jobs(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool jobsArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);

	static bool currentJob(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool nextJob(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool jobState(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);

	static bool jobHistoryNull(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool jobHistoryName(Comm::JsonDecoder* decoder, const char* data, const size_t indices[]);
	static bool jobHistoryCount(Comm::JsonDecoder* decoder, const int32_t& data, const size_t indices[]);
	static bool jobHistoryArrayEnd(Comm::JsonDecoder* decoder, const size_t indices[]);
};


/*
 * JobSelectPresenter.h
 *
 *  Created on: 2026-04-07
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class JobSelectView;

	class JobSelectPresenter : public Presenter<JobSelectView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(JobSelectPresenter, JobSelectView);

		// Setters

		// Getters

		// Actions
		void selectNextJob(std::string_view jobName);

		// Observers
		void newJobs();
		void newCurrentJob();
		void newNextJob();
		void newJobState();
		void newJobHistory();

	  protected:
		void onInit() override;
		void onActivate() override {}
		void onDeactivate() override {}

		void onConnect() override {}
		void onDisconnect() override {}
	};
} // namespace UI

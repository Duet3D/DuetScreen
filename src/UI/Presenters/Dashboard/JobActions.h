/*
 * JobActions.h
 *
 * The global actions that control the job, for any view: `actions.job.pause`, `actions.job.resume`,
 * `actions.job.print_again`, and `actions.job.ask_cancel`, which asks the shared question (Modals/JobCancelPresenter.h)
 * before cancelling. Whether each is offered now is published as `job.pause_control` and so on
 * (UI/Xml/Data/JobSource.h).
 */

#pragma once

namespace UI::JobActions
{
	/** Register the job actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::JobActions

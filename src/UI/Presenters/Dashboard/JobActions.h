/*
 * JobActions.h
 *
 * The global actions that control the job, for any view: `job.pause`, `job.resume` and `job.print_again`. Whether
 * each is offered now is published as `job.pause_control` and so on (UI/Xml/Data/JobSource.h). Cancelling is asked
 * about first, with the shared question's global `job.ask_cancel` (Modals/JobCancelPresenter.h).
 */

#pragma once

namespace UI::JobActions
{
	/** Register the job actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::JobActions

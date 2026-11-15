/*
 * FilesActions.h
 *
 * The global actions of the shared file dialog (src/UI/Xml/Data/FilesSource.h), for any view: `actions.files.ask_start`
 * and `actions.files.ask_run` open it for a job or a macro listed by `source.files.*`, and its buttons raise
 * `actions.files.confirm` and `actions.files.ask_delete`. `actions.files.refresh` lists the jobs and macros again. What each does is in its registration, and in the UI API
 * reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::FilesActions
{
	/** Register the file dialog actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::FilesActions

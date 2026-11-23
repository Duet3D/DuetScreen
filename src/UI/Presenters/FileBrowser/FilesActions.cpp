/*
 * FilesActions.cpp
 */

#include "FilesActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/FilesSource.h"
#include "UI/Xml/View/Actions.h"

namespace UI::FilesActions
{
	namespace
	{
		/** The shared file dialog's source; nullptr, with a warning, while the data sources aren't running */
		Xml::FilesSource* files()
		{
			auto* source = Xml::DataSources::find<Xml::FilesSource>();
			if (source == nullptr)
			{
				LOG_WARN("The files source isn't running");
			}
			return source;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("files.refresh",
								"List the jobs and macros (source.files.recent and macros) again, e.g. after uploading "
								"some",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = files())
									{
										source->refresh();
									}
								});
		Xml::Actions::addGlobal("files.ask_start",
								"Ask whether to start the job, with its details and thumbnail. From a row of "
								"source.files.recent.",
								[](const Xml::ActionArgs& args)
								{
									const auto* row = args.row<Xml::RecentJobRow>();
									auto* source = files();
									if (row != nullptr && source != nullptr)
									{
										source->askStart(*row);
									}
								});
		Xml::Actions::addGlobal("files.ask_run",
								"Ask whether to run the macro. From a row of source.files.macros.",
								[](const Xml::ActionArgs& args)
								{
									const auto* row = args.row<Xml::MacroRow>();
									auto* source = files();
									if (row != nullptr && source != nullptr)
									{
										source->askRun(*row);
									}
								});
		Xml::Actions::addGlobal("files.confirm",
								"Do what the file dialog asks (start the job, and show the dashboard, or run the "
								"macro), and close it",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = files())
									{
										source->confirm();
									}
								});
		Xml::Actions::addGlobal("files.ask_delete",
								"Ask whether to delete the job the file dialog is about, as the dialog's next step; "
								"deleting it closes the dialog",
								[](const Xml::ActionArgs& args)
								{
									if (auto* source = files())
									{
										source->askDelete(args.target);
									}
								});
	}
} // namespace UI::FilesActions

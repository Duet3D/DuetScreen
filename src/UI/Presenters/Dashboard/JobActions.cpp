/*
 * JobActions.cpp
 */

#include "JobActions.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Presenters/Modals/JobCancelPresenter.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <fmt/format.h>
#include <string>

namespace UI::JobActions
{
	void registerAll()
	{
		Xml::Actions::addGlobal("job.pause",
								"Pause the job",
								"tr.actions.job_pause",
								[](const Xml::ActionArgs&) { OM::FileSystem::PausePrint(); });
		Xml::Actions::addGlobal("job.resume",
								"Resume the paused job",
								"tr.actions.job_resume",
								[](const Xml::ActionArgs&) { OM::FileSystem::ResumePrint(); });
		Xml::Actions::addGlobal("job.print_again",
								"Start the last job again",
								[](const Xml::ActionArgs&)
								{
									std::string last;
									{
										MODEL_LOCK();
										last = OM::Tree::get().root().at("job.lastFileName").text();
									}
									if (!last.empty())
									{
										Xml::Services::sendGcode(fmt::format("M23 \"{:s}\"\nM24\n", last));
									}
								});
		Xml::Actions::addGlobal("job.ask_cancel",
								"Ask whether to cancel the job; it is cancelled, which also turns the heaters off, "
								"only if the user says so",
								"tr.actions.job_ask_cancel",
								[](const Xml::ActionArgs&)
								{
									if (auto* question = JobCancelPresenter::shared())
									{
										question->ask();
									}
								});
	}
} // namespace UI::JobActions

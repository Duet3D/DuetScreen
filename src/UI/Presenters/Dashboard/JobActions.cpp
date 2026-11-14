/*
 * JobActions.cpp
 */

#include "JobActions.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <fmt/format.h>
#include <string>

namespace UI::JobActions
{
	void registerAll()
	{
		Xml::Actions::addGlobal(
			"job.pause", "Pause the job", [](const Xml::ActionArgs&) { OM::FileSystem::PausePrint(); });
		Xml::Actions::addGlobal(
			"job.resume", "Resume the paused job", [](const Xml::ActionArgs&) { OM::FileSystem::ResumePrint(); });
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
	}
} // namespace UI::JobActions

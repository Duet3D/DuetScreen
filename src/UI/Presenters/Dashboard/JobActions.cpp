/*
 * JobActions.cpp
 */

#include "JobActions.h"
#include "ObjectModel/Files.h"
#include "UI/Xml/View/Actions.h"

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
								[](const Xml::ActionArgs&) { OM::FileSystem::PrintAgain(); });
	}
} // namespace UI::JobActions

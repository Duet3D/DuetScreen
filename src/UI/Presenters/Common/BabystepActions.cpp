/*
 * BabystepActions.cpp
 */

#include "BabystepActions.h"
#include "Debug.h"
#include "UI/Xml/Data/BabystepSource.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"

namespace UI::BabystepActions
{
	namespace
	{
		/** Babystepping's source; nullptr, with a warning, while the data sources aren't running */
		Xml::BabystepSource* babystep()
		{
			auto* source = Xml::DataSources::find<Xml::BabystepSource>();
			if (source == nullptr)
			{
				LOG_WARN("The babystepping source isn't running");
			}
			return source;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("babystep.up",
								"Move Z up by the chosen step (M290)",
								"tr.actions.babystep_up",
								[](const Xml::ActionArgs&)
								{
									if (const auto* source = babystep())
									{
										source->step(1);
									}
								});
		Xml::Actions::addGlobal("babystep.down",
								"Move Z down by the chosen step (M290)",
								"tr.actions.babystep_down",
								[](const Xml::ActionArgs&)
								{
									if (const auto* source = babystep())
									{
										source->step(-1);
									}
								});
		Xml::Actions::addGlobal("babystep.reset",
								"Undo all babystepping",
								[](const Xml::ActionArgs&) { Xml::Services::sendGcode("M290 R0 S0"); });
		Xml::Actions::addGlobal("babystep.open",
								"Open the shared babystepping dialog",
								"tr.actions.babystep_open",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = babystep())
									{
										source->setDialogOpen(true);
									}
								});
		Xml::Actions::addGlobal("babystep.close",
								"Close the shared babystepping dialog",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = babystep())
									{
										source->setDialogOpen(false);
									}
								});
	}
} // namespace UI::BabystepActions

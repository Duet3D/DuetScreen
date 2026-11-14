/*
 * BabystepActions.cpp
 */

#include "BabystepActions.h"
#include "BabystepDialogPresenter.h"
#include "UI/Xml/View/Actions.h"

namespace UI::BabystepActions
{
	void registerAll()
	{
		Xml::Actions::addGlobal("babystep.open",
								"Open the shared babystepping dialog",
								"tr.actions.babystep_open",
								[](const Xml::ActionArgs&)
								{
									if (auto* dialog = BabystepDialogPresenter::shared())
									{
										dialog->open();
									}
								});
	}
} // namespace UI::BabystepActions

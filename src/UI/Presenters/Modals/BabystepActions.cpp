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
								[](const Xml::ActionArgs&)
								{
									if (auto* dialog = BabystepDialogPresenter::shared())
									{
										dialog->open();
									}
								});
	}
} // namespace UI::BabystepActions

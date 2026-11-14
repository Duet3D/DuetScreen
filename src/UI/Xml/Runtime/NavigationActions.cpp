/*
 * NavigationActions.cpp
 */

#include "NavigationActions.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::NavigationActions
{
	/** `actions.navigation.open`: open the screen named by the argument */
	static void open(const ActionArgs& args)
	{
		openScreen(args.arg, true);
	}

	void registerAll()
	{
		Actions::addGlobal("navigation.open",
						   "Open the screen registered as `arg`, e.g. `arg=\"control\"`",
						   "tr.actions.navigation_open",
						   open);
		Actions::addGlobal(
			"navigation.home", "Close every screen and show the home screens", [](const ActionArgs&) { home(); });
		Actions::addGlobal(
			"navigation.back", "Close the top modal, or else go back a screen", [](const ActionArgs&) { back(); });
	}
} // namespace UI::Xml::NavigationActions

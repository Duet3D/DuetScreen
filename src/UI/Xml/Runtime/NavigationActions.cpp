/*
 * NavigationActions.cpp
 */

#include "NavigationActions.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Layout/DsPages.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::NavigationActions
{
	/** `actions.navigation.open`: open the screen named by the argument */
	static void open(const ActionArgs& args)
	{
		openScreen(args.arg, true);
	}

	/** `actions.navigation.open_page`: open the page named by the argument in the stack of pages around the target */
	static void openPage(const ActionArgs& args)
	{
		if (DsPages* pages = DsPages::around(args.target, args.arg))
		{
			pages->open(args.arg);
		}
		else
		{
			LOG_WARN("actions.navigation.open_page: no <ds_pages> around the widget has a page called \"{:s}\"",
					 args.arg);
		}
	}

	void registerAll()
	{
		Actions::addGlobal("navigation.open",
						   "Open the screen registered as `arg`, e.g. `arg=\"control\"`",
						   "tr.actions.navigation_open",
						   open);
		Actions::addGlobal("navigation.open_page",
						   "Open the page called `arg` in the <ds_pages> around the widget, e.g. `arg=\"colours\"`",
						   openPage);
		Actions::addGlobal(
			"navigation.home", "Close every screen and show the home screens", [](const ActionArgs&) { home(); });
		Actions::addGlobal(
			"navigation.back", "Close the top modal, or else go back a screen", [](const ActionArgs&) { back(); });
	}
} // namespace UI::Xml::NavigationActions

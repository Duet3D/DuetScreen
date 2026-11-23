/*
 * NavigationActions.cpp
 */

#include "NavigationActions.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Layout/DsPages.h"
#include "UI/Widgets/Layout/TabView.h"
#include "UI/Xml/View/Actions.h"

namespace UI::Xml::NavigationActions
{
	/**
	 * `actions.navigation.open`: open the screen named by the argument, e.g. `files`; `files/jobs` opens it on the tab
	 * called `jobs` of the first tab view in it that has one
	 */
	static void open(const ActionArgs& args)
	{
		const std::string_view arg = args.arg;
		const size_t slash = arg.find('/');
		const std::string_view screen = arg.substr(0, slash);
		if (!openScreen(screen, true) || slash == std::string_view::npos)
		{
			return;
		}

		struct Wanted
		{
			std::string_view tab; // The tab's name
			bool found = false;	  // A tab view had it, and shows it now
		} wanted{.tab = arg.substr(slash + 1)};
		LvObj* view = findView(screen);
		if (view == nullptr)
		{
			return;
		}
		lv_obj_tree_walk(
			view->getRootPtr(),
			[](lv_obj_t* obj, void* data)
			{
				auto& wanted = *static_cast<Wanted*>(data);
				LvObj* wrapper = LvObj::fromPtr(obj);
				auto* tabs = wrapper != nullptr ? dynamic_cast<TabView*>(wrapper) : nullptr;
				const size_t index = tabs != nullptr ? tabs->indexOfTab(wanted.tab) : 0;
				if (tabs == nullptr || index >= tabs->getTabCount())
				{
					return LV_OBJ_TREE_WALK_NEXT;
				}
				tabs->setActiveTab(index, false);
				wanted.found = true;
				return LV_OBJ_TREE_WALK_END;
			},
			&wanted);
		if (!wanted.found)
		{
			LOG_WARN("actions.navigation.open: \"{:s}\" has no tab called \"{:s}\"", screen, wanted.tab);
		}
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
		Actions::addGlobal(
			"navigation.open",
			"Open the screen registered as `arg`, e.g. `arg=\"control\"`; `arg=\"files/jobs\"` opens it on its "
			"tab called `jobs`",
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

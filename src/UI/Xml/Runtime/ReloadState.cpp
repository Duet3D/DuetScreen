/*
 * ReloadState.cpp
 */

#include "ReloadState.h"
#include "Debug.h"
#include "UI/Widgets/Layout/TabView.h"
#include <array>

namespace UI::Xml
{
	namespace
	{
		/** `obj`'s name, or the one LVGL makes up for it if it has none (e.g. "lv_obj_2") */
		std::string resolvedName(const lv_obj_t* obj)
		{
			std::array<char, 128> name{};
			lv_obj_get_name_resolved(obj, name.data(), name.size());
			return name.data();
		}

		/** The path of names from `obj`'s screen down to `obj`, which lv_obj_get_child_by_name() finds it by */
		std::string pathOf(const lv_obj_t* obj)
		{
			std::string path;
			for (; lv_obj_get_parent(obj) != nullptr; obj = lv_obj_get_parent(obj))
			{
				path.insert(0, path.empty() ? resolvedName(obj) : resolvedName(obj) + "/");
			}
			return path;
		}

		/** The tab view `obj` is the root of, or nullptr */
		TabView* tabViewOf(lv_obj_t* obj)
		{
			LvObj* view = obj != nullptr ? LvObj::fromPtr(obj) : nullptr;
			return view != nullptr ? dynamic_cast<TabView*>(view) : nullptr;
		}
	} // namespace

	ReloadState ReloadState::capture(const std::vector<lv_obj_t*>& roots)
	{
		UI_LOCK();
		ReloadState state;
		state.m_navigation = saveNavigationState();
		for (lv_obj_t* root : roots)
		{
			captureTabs(root, state.m_tabs);
		}
		return state;
	}

	void ReloadState::captureTabs(lv_obj_t* root, std::vector<TabSelection>& tabs)
	{
		lv_obj_tree_walk(
			root,
			[](lv_obj_t* obj, void* userData)
			{
				TabView* tabView = tabViewOf(obj);
				if (tabView != nullptr && tabView->getActiveTabIndex() < tabView->getTabCount())
				{
					auto& selections = *static_cast<std::vector<TabSelection>*>(userData);
					selections.push_back({.screen = lv_obj_get_screen(obj),
										  .path = pathOf(obj),
										  .tab = resolvedName(tabView->getActiveTab().getRootPtr())});
				}
				return LV_OBJ_TREE_WALK_NEXT;
			},
			&tabs);
	}

	void ReloadState::restore() const
	{
		ZoneScoped;
		UI_LOCK();
		// Tabs first, so a screen opens on the right tab instead of sliding to it
		restoreTabs();
		restoreNavigationState(m_navigation);
	}

	void ReloadState::restoreTabs() const
	{
		for (const TabSelection& selection : m_tabs)
		{
			TabView* tabView = tabViewOf(lv_obj_get_child_by_name(selection.screen, selection.path.c_str()));
			if (tabView == nullptr)
			{
				continue;
			}
			for (size_t i = 0; i < tabView->getTabCount(); i++)
			{
				if (resolvedName(tabView->getTab(i)->getRootPtr()) == selection.tab)
				{
					if (!tabView->isTabDisabled(i))
					{
						tabView->setActiveTab(i, false);
					}
					break;
				}
			}
		}
	}
} // namespace UI::Xml

/*
 * ReloadState.h
 *
 * Where the user is in the UI, kept across a reload: the open screens, and the tab each tab view shows. A reload
 * rebuilds every view, which would otherwise leave the user on the home screen with every first tab selected.
 *
 * Screens are remembered by the names they are registered under (see UI/Core/Navigation.h), and tab views by their
 * widget path from the screen, so whatever the new XML no longer has (or has moved) simply isn't restored. A tab that
 * is now disabled isn't selected again.
 */

#pragma once

#include "UI/Core/Navigation.h"
#include "lvgl/lvgl.h"
#include <string>
#include <vector>

namespace UI::Xml
{
	class ReloadState
	{
	  public:
		/** Remember where the user is: the open screens, and the tab shown by every tab view inside `roots` */
		static ReloadState capture(const std::vector<lv_obj_t*>& roots);

		/** Put the user back where capture() found them, once the views have been rebuilt */
		void restore() const;

	  private:
		/** The tab a tab view showed */
		struct TabSelection
		{
			lv_obj_t* screen; // The screen the tab view is on, which a reload keeps
			std::string path; // The tab view's widget path from its screen (see lv_obj_get_child_by_name())
			std::string tab;  // The name of the tab it showed
		};

		/** Every tab view's selection in `root`, added to `tabs` */
		static void captureTabs(lv_obj_t* root, std::vector<TabSelection>& tabs);

		/** Select the remembered tab of each tab view that is still there */
		void restoreTabs() const;

		NavigationState m_navigation;	  // The open and returnable screens
		std::vector<TabSelection> m_tabs; // Every tab view's selected tab
	};
} // namespace UI::Xml

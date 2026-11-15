/*
 * TabView.h
 *
 *  Created on: 2025-11-06
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Widgets/Buttons/Button.h"
#include "UI/Lvgl/LvContainer.h"
#include "UI/Widgets/Layout/List.h"
#include <vector>

namespace UI
{
	class TabView : public LvContainer
	{
	  public:
		using TabButton = Button;

		TabView(const std::string& name, LvObj& parent);

		/** For widgets built by the XML engine, whose parent has no C++ wrapper */
		TabView(const std::string& name, lv_obj_t* parent);

		LvContainer& addTab(std::string_view tab_name);
		bool renameTab(size_t index, std::string_view new_name);

		LvContainer& getActiveTab();
		TabButton* getTabButton(size_t index);
		LvContainer* getTab(size_t index);

		/** The index of the tab called `name` (its content's name, the `<ds_tabs-tab>`'s), or getTabCount() if none */
		size_t indexOfTab(std::string_view name) const;
		size_t getTabCount() const { return m_tabs.size(); };
		size_t getActiveTabIndex() const { return m_currentTabIndex; };

		/**
		 * Show tab `index`, sliding it in if animations are enabled and `animate`. While the tab view is hidden (it or
		 * anything it is inside), the tab is only marked shown: it comes alive with the tab view.
		 */
		void setActiveTab(size_t index, bool animate = true);

		/** Put the tab bar on side `dir` (LV_DIR_TOP, _BOTTOM, _LEFT or _RIGHT) of the content, styling its buttons to suit */
		void setTabBarPosition(lv_dir_t dir);

		bool disableTab(size_t index, bool disable);

		/**
		 * Show or hide tab `index`'s button. A hidden tab can't be selected: hiding the selected tab selects the next
		 * one available, and showing a tab while none is selected selects it.
		 * @return false if there is no such tab
		 */
		bool showTab(size_t index, bool shown);

		/** Put tab `index`, and those after it, at the far end of the tab bar, e.g. the bottom of a bar down a side */
		void pinToEnd(size_t index);

		/** Whether tab `index`'s button is disabled; false if there is no such tab */
		bool isTabDisabled(size_t index);

		/** Whether tab `index` can be selected: its button is shown and not disabled; false if there is no such tab */
		bool isTabAvailable(size_t index);

	  private:
		/** Lay out the tab bar and content area */
		void init();

		/** Tab `index` can no longer be selected: if it is the one selected, select the next available, or none */
		void leaveTab(size_t index);

		/**
		 * Style `button` for the side the tab bar is on. Across the top or bottom, the tabs share the bar's width, the
		 * icon over the text, and the selected one is edged on one side (tab_btn). Down a side, each is a row as wide
		 * as the bar, the icon before the text, and the selected one looks like any chosen option (btn_checked).
		 */
		void styleTabButton(TabButton& button);

		List<TabButton> m_tabButtons{"tab_buttons", getRoot()};
		LvContainer m_tabContent{"tab_content", getRoot()};

		std::vector<std::unique_ptr<LvContainer>> m_tabs;
		size_t m_currentTabIndex = -1;
		lv_dir_t m_barSide = LV_DIR_TOP;			 // The side of the content the tab bar is on
		std::unique_ptr<LvContainer> m_endSpacer; // Before the first tab pinned to the end (pinToEnd()), if any
	};
} // namespace UI

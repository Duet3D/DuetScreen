/*
 * TabView.cpp
 *
 *  Created on: 2025-11-06
 *      Author: Andy Everitt
 */

#include "TabView.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Lvgl/Transitions.h"

namespace UI
{
	namespace
	{
		/** The size of a tab's icon in a tab bar down a side, where it goes before the text */
		constexpr lv_coord_t SIDE_TAB_ICON_SIZE = 28;

		/** The gap between a tab bar down a side and the content; a bar over or under the content touches it */
		constexpr lv_coord_t SIDE_TAB_GAP = 5;

		/** How far a tab bar over or under the content overlaps it, so the selected tab joins its page */
		constexpr lv_coord_t TOP_TAB_OVERLAP = 5;
	} // namespace

	TabView::TabView(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		init();
	}

	TabView::TabView(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		init();
	}

	void TabView::init()
	{
		setStylePad(0);
		setTabBarPosition(LV_DIR_TOP); // Sets the gap between the tab bar and the content

		m_tabButtons.setStylePad(0, LV_STATE_USER_1, Padding::VERTICAL);
		m_tabButtons.getListContainer().setStylePad(0);
		m_tabButtons.getListContainer().setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_tabContent.setSize(LV_PCT(100), LV_PCT(100));
		m_tabContent.setFlexGrow(1);
		m_tabContent.setStylePad(0);
	}

	LvContainer& TabView::addTab(std::string_view tab_name)
	{
		ZoneScoped;
		UI_LOCK();
		// Create new tab container
		m_tabs.emplace_back(std::make_unique<LvContainer>(fmt::format("tab_{}", m_tabs.size()), m_tabContent));
		LvContainer& new_tab = *m_tabs.back();

		new_tab.addStyle(Themes::getLvglStyles().tab_card);
		new_tab.setSize(LV_PCT(100), LV_PCT(100));
		new_tab.setStylePad(0);

		// Create corresponding tab button
		TabButton& tab_button =
			m_tabButtons.addItem([&](size_t index, LvObj& parent)
								 { return std::make_unique<TabButton>(fmt::format("tab_button_{}", index), parent); });
		tab_button.setText(tab_name);
		styleTabButton(tab_button);
		tab_button.addClickedCallback(
			[](lv_event_t* e)
			{
				TabView* tab_view = static_cast<TabView*>(lv_event_get_user_data(e));
				LvObj* target = LvObj::fromPtr(lv_event_get_target_obj(e));
				size_t index = 0;
				tab_view->m_tabButtons.iterateListItems(
					[&](size_t i, LvObj& obj)
					{
						if (&obj == target)
						{
							index = i;
						}
					});
				tab_view->setActiveTab(index);
			},
			this);

		// Select the new tab if none is selected: it is the first, or every earlier one is disabled
		if (m_currentTabIndex >= m_tabs.size() - 1)
		{
			setActiveTab(m_tabs.size() - 1);
		}
		else
		{
			new_tab.hide();
		}

		return new_tab;
	}

	bool TabView::renameTab(size_t index, std::string_view new_name)
	{
		ZoneScoped;
		UI_LOCK();
		if (TabButton* tab_button = m_tabButtons.getItem(index))
		{
			tab_button->setText(new_name);
			return true;
		}

		LOG_ERROR("Tab index {} out of range", index);
		return false;
	}

	LvContainer& TabView::getActiveTab()
	{
		ZoneScoped;
		UI_LOCK();
		return *m_tabs.at(m_currentTabIndex);
	}

	TabView::TabButton* TabView::getTabButton(size_t index)
	{
		ZoneScoped;
		UI_LOCK();
		return m_tabButtons.getItem(index);
	}

	size_t TabView::indexOfTab(std::string_view name) const
	{
		for (size_t i = 0; i < m_tabs.size(); i++)
		{
			const char* tabName = lv_obj_get_name(m_tabs[i]->getRootPtr());
			if (tabName != nullptr && name == tabName)
			{
				return i;
			}
		}
		return m_tabs.size();
	}

	LvContainer* TabView::getTab(size_t index)
	{
		ZoneScoped;
		UI_LOCK();
		if (index >= m_tabs.size())
		{
			return nullptr;
		}
		return m_tabs.at(index).get();
	}

	void TabView::setActiveTab(size_t index, bool animate)
	{
		ZoneScoped;
		UI_LOCK();
		if (index == m_currentTabIndex)
		{
			return;
		}

		if (index >= m_tabs.size())
		{
			LOG_ERROR("Tab index {} out of range", index);
			return;
		}

		m_tabContent.updateLayout();

		const size_t previous_index = m_currentTabIndex;
		LvContainer* previous_tab = (previous_index < m_tabs.size()) ? m_tabs.at(previous_index).get() : nullptr;
		LvContainer& next_tab = *m_tabs.at(index);

		m_currentTabIndex = index;
		m_tabButtons.iterateListItems([&](size_t i, TabButton& button) { button.setChecked(i == index); });

		if (!isShownOnScreen())
		{
			// Nothing on screen to show or hide: the tabs' views come alive or not when the tab view is shown
			if (previous_tab != nullptr)
			{
				Transitions::stopSlide(*previous_tab);
				previous_tab->setFlag(LV_OBJ_FLAG_HIDDEN, true);
			}
			Transitions::stopSlide(next_tab);
			next_tab.setFlag(LV_OBJ_FLAG_HIDDEN, false);
			return;
		}

		// A dialog opened from the tab being left goes with it, as one the tab hosted itself would
		closeDialogsNotAround(getRootPtr());

		const int32_t direction = (index > previous_index) ? 1 : -1;
		Transitions::slide(previous_tab, next_tab, direction, m_tabContent.getWidth(), animate);
	}

	void TabView::styleTabButton(TabButton& button)
	{
		const lv_style_t* base = Themes::getComponentStyles().tab_button;
		const lv_style_t* top = Themes::getComponentStyles().tab_button_top;
		const lv_style_t* side = Themes::getComponentStyles().tab_button_side;
		if (m_barSide == LV_DIR_LEFT || m_barSide == LV_DIR_RIGHT)
		{
			button.removeStyle(base, LV_STATE_CHECKED);
			button.removeStyle(top);
			button.addStyle(side);
			button.setIconBeforeText(true, SIDE_TAB_ICON_SIZE);
		}
		else
		{
			button.removeStyle(side);
			button.addStyle(top);
			button.addStyle(base, LV_STATE_CHECKED); // The selected tab, as LVGL's tabs
			button.setIconBeforeText(false);
		}
	}

	void TabView::pinToEnd(size_t index)
	{
		ZoneScoped;
		UI_LOCK();
		TabButton* button = m_tabButtons.getItem(index);
		if (button == nullptr)
		{
			LOG_ERROR("Tab index {} out of range", index);
			return;
		}
		if (m_endSpacer == nullptr)
		{
			m_endSpacer = std::make_unique<LvContainer>("end_spacer", m_tabButtons.getListContainer());
			m_endSpacer->setSize(1, 1);
			m_endSpacer->setFlexGrow(1);
			m_endSpacer->setStylePad(0);
			m_endSpacer->setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		}
		lv_obj_move_to_index(m_endSpacer->getRootPtr(), lv_obj_get_index(button->getRootPtr()));
	}

	void TabView::setTabBarPosition(lv_dir_t dir)
	{
		ZoneScoped;
		UI_LOCK();
		m_barSide = dir;
		const bool side = dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT;
		if (side)
		{
			m_tabButtons.addStyle(Themes::getComponentStyles().tab_bar_side);
		}
		else
		{
			m_tabButtons.removeStyle(Themes::getComponentStyles().tab_bar_side);
		}
		setStylePad(side ? SIDE_TAB_GAP : -TOP_TAB_OVERLAP, LV_PART_MAIN, Padding::ROW);
		setStylePad(side ? SIDE_TAB_GAP : -TOP_TAB_OVERLAP, LV_PART_MAIN, Padding::COLUMN);
		m_tabButtons.iterateListItems([this](size_t, TabButton& button) { styleTabButton(button); });
		m_tabButtons.setFlag(LV_OBJ_FLAG_FLOATING, false);
		m_tabButtons.setState(LV_STATE_USER_1, dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM);
		m_tabButtons.setState(LV_STATE_USER_2, dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT);
		switch (dir)
		{
		case LV_DIR_TOP:
			setFlexFlow(LV_FLEX_FLOW_COLUMN);
			m_tabButtons.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_tabButtons.getListContainer().setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_tabButtons.getListContainer().setFlexFlow(LV_FLEX_FLOW_ROW);
			break;
		case LV_DIR_BOTTOM:
			setFlexFlow(LV_FLEX_FLOW_COLUMN_REVERSE);
			m_tabButtons.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_tabButtons.getListContainer().setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_tabButtons.getListContainer().setFlexFlow(LV_FLEX_FLOW_ROW);
			break;
		case LV_DIR_LEFT:
			setFlexFlow(LV_FLEX_FLOW_ROW);
			m_tabButtons.setSize(LV_SIZE_CONTENT, LV_PCT(100));
			m_tabButtons.getListContainer().setSize(LV_SIZE_CONTENT, LV_PCT(100));
			m_tabButtons.getListContainer().setFlexFlow(LV_FLEX_FLOW_COLUMN);
			break;
		case LV_DIR_RIGHT:
			setFlexFlow(LV_FLEX_FLOW_ROW_REVERSE);
			m_tabButtons.setSize(LV_SIZE_CONTENT, LV_PCT(100));
			m_tabButtons.getListContainer().setSize(LV_SIZE_CONTENT, LV_PCT(100));
			m_tabButtons.getListContainer().setFlexFlow(LV_FLEX_FLOW_COLUMN);
			break;
		default:
			LOG_ERROR("Invalid tab bar direction");
			return;
		}
		return;
	}

	bool TabView::isTabDisabled(size_t index)
	{
		UI_LOCK();
		TabButton* tab_button = m_tabButtons.getItem(index);
		return tab_button != nullptr && tab_button->hasState(LV_STATE_DISABLED);
	}

	bool TabView::isTabAvailable(size_t index)
	{
		UI_LOCK();
		TabButton* tab_button = m_tabButtons.getItem(index);
		return tab_button != nullptr && !tab_button->hasState(LV_STATE_DISABLED) &&
			   !tab_button->hasFlag(LV_OBJ_FLAG_HIDDEN);
	}

	bool TabView::disableTab(size_t index, bool disable)
	{
		ZoneScoped;
		UI_LOCK();
		TabButton* tab_button = m_tabButtons.getItem(index);
		if (!tab_button)
		{
			LOG_ERROR("Tab index {} out of range", index);
			return false;
		}

		if (tab_button->hasState(LV_STATE_DISABLED) == disable)
		{
			return true; // Already in desired state
		}

		tab_button->setDisabled(disable);
		if (disable)
		{
			leaveTab(index);
		}
		return true;
	}

	bool TabView::showTab(size_t index, bool shown)
	{
		ZoneScoped;
		UI_LOCK();
		TabButton* tab_button = m_tabButtons.getItem(index);
		if (!tab_button)
		{
			LOG_ERROR("Tab index {} out of range", index);
			return false;
		}

		if (tab_button->hasFlag(LV_OBJ_FLAG_HIDDEN) != shown)
		{
			return true; // Already in desired state
		}

		tab_button->setFlag(LV_OBJ_FLAG_HIDDEN, !shown);
		if (!shown)
		{
			leaveTab(index);
		}
		else if (m_currentTabIndex >= m_tabs.size() && isTabAvailable(index))
		{
			setActiveTab(index, false);
		}
		return true;
	}

	void TabView::leaveTab(size_t index)
	{
		if (index != m_currentTabIndex)
		{
			return;
		}
		for (size_t i = 1; i < m_tabs.size(); i++)
		{
			const size_t next = (index + i) % m_tabs.size();
			if (isTabAvailable(next))
			{
				setActiveTab(next, false);
				return;
			}
		}
		m_currentTabIndex = -1; // No active tab
		m_tabs.at(index)->hide();
	}
} // namespace UI

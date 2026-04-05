/*
 * TabView.cpp
 *
 *  Created on: 2025-11-06
 *      Author: Andy Everitt
 */

#include "TabView.h"
#include "Debug.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "UI/Components/LVGL/Transitions.h"
#include "utils/StorageHelper.h"

namespace UI
{
	namespace
	{
		static void tabSetXAnimCb(void* var, int32_t value)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(var));
			if (obj == nullptr)
			{
				return;
			}
			obj->setX(static_cast<lv_coord_t>(value));
		}

		static void outgoingTabDeletedCb(lv_anim_t* anim)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(anim->var));
			if (obj == nullptr)
			{
				return;
			}

			obj->setX(0);
			obj->hide();
		}

		static void incomingTabDeletedCb(lv_anim_t* anim)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(anim->var));
			if (obj == nullptr)
			{
				return;
			}

			obj->setX(0);
		}
	} // namespace

	TabView::TabView(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		setTabBarPosition(LV_DIR_TOP);
		setStylePad(0);
		setStylePad(-5, LV_PART_MAIN, Padding::ROW);
		setStylePad(-5, LV_PART_MAIN, Padding::COLUMN);

		m_tabButtons.setStylePad(0, LV_STATE_USER_1, Padding::VERTICAL);
		m_tabButtons.setStylePad(0, LV_STATE_USER_2, Padding::HORIZONTAL);
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
		tab_button.addStyle(Themes::getLvglStyles().text_header, LV_PART_MAIN);
		tab_button.addStyle(Themes::getComponentStyles().tab_button, LV_PART_MAIN);
		tab_button.setText(tab_name);
		tab_button.setFlexGrow(1);
		tab_button.setSize(LV_PCT(100), LV_PCT(100));
		tab_button.setMinWidth(LV_SIZE_CONTENT);
		tab_button.setMinHeight(LV_SIZE_CONTENT);
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

		// If this is the first tab, activate it
		if (m_tabs.size() == 1)
		{
			setActiveTab(0);
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

	void TabView::setActiveTab(size_t index)
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

		const bool animate = StorageHelper::getData(ID_UI_ANIMATIONS_ENABLED) && previous_tab != nullptr;
		if (!animate)
		{
			if (previous_tab != nullptr)
			{
				previous_tab->setX(0);
				previous_tab->hide();
			}

			next_tab.setX(0);
			next_tab.show(true);
			return;
		}

		const lv_coord_t width = m_tabContent.getWidth();
		const int32_t direction = (index > previous_index) ? 1 : -1;
		const lv_coord_t start_x = static_cast<lv_coord_t>(direction * width);
		const lv_coord_t end_x = static_cast<lv_coord_t>(-direction * width);

		/* Cancel any in-flight position animations before starting a new transition. */
		lv_anim_delete(previous_tab->getRootPtr(), tabSetXAnimCb);
		lv_anim_delete(next_tab.getRootPtr(), tabSetXAnimCb);

		previous_tab->setX(0);
		next_tab.setX(start_x);
		next_tab.show(true);

		const uint32_t slideDurationMs = Transitions::animDurationMs();

		LvAnim outgoing_anim;
		outgoing_anim.setVar(previous_tab->getRootPtr());
		outgoing_anim.setValues(0, end_x);
		outgoing_anim.setExecCb(tabSetXAnimCb);
		outgoing_anim.setDuration(slideDurationMs);
		outgoing_anim.setPathCb(lv_anim_path_ease_in_out);
		outgoing_anim.setDeletedCb(outgoingTabDeletedCb);
		outgoing_anim.start();

		LvAnim incoming_anim;
		incoming_anim.setVar(next_tab.getRootPtr());
		incoming_anim.setValues(start_x, 0);
		incoming_anim.setExecCb(tabSetXAnimCb);
		incoming_anim.setDuration(slideDurationMs);
		incoming_anim.setPathCb(lv_anim_path_ease_in_out);
		incoming_anim.setDeletedCb(incomingTabDeletedCb);
		incoming_anim.start();
	}

	void TabView::setTabBarPosition(lv_dir_t dir)
	{
		ZoneScoped;
		UI_LOCK();
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

		if (disable && index == m_currentTabIndex)
		{
			// If disabling the active tab, switch to the first available tab
			bool switched = false;
			for (size_t i = 0; i < m_tabs.size(); i++)
			{
				size_t shifted_index = (index + i + 1) % m_tabs.size();
				TabButton* next_button = m_tabButtons.getItem(shifted_index);
				if (shifted_index != index && next_button && !next_button->hasState(LV_STATE_DISABLED))
				{
					setActiveTab(shifted_index);
					switched = true;
					break;
				}
			}
			if (!switched)
			{
				m_currentTabIndex = -1; // No active tab
				m_tabs.at(index)->hide();
			}
		}

		return true;
	}
} // namespace UI

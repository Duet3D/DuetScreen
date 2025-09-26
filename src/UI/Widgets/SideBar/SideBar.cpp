/*
 * SideBar.cpp
 *
 *  Created on: 2025-01-16
 *      Author: Andy Everitt
 */

#include "SideBar.h"
#include "Debug.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

#define ESTOP_SIZE 80

namespace UI
{
	SideBar::SideBar(const std::string& name, LvObj& parent)
		: View(name, parent)
		, m_btns("buttons", getRoot())
		, m_homeBtn("home", m_btns, _("home"))
		, m_backBtn("back", m_btns, _("back"))
		, m_menuBtn("menu", m_btns, _("menu"))
		, m_macrosBtn("macros", m_btns, _("macros"))
		, m_eStopBtn("estop", m_btns)
		, m_appDrawer("app_drawer", getRoot())
		, m_appDrawerModalBg("app_drawer_modal_bg", getRoot())
	{
		LOG_VERBOSE("Creating SideBar");

		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_btns.setSize(LV_PCT(100), LV_PCT(100));
		m_btns.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_btns.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_btns.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_backBtn.setWidth(LV_PCT(100));
		m_homeBtn.setWidth(LV_PCT(100));
		m_macrosBtn.setWidth(LV_PCT(100));
		m_menuBtn.setWidth(LV_PCT(100));

		setExtDrawSize(lv_obj_get_width(getScreen()));
		m_btns.setExtDrawSize(400);

		m_backBtn.setFlexGrow(1);
		m_homeBtn.setFlexGrow(1);
		m_macrosBtn.setFlexGrow(1);
		m_menuBtn.setFlexGrow(1);

		m_eStopBtn.setSize(ESTOP_SIZE, ESTOP_SIZE);

		m_homeBtn.setIcon(IMAGE_ASSET("home.png"));

		m_backBtn.addClickedCallback(backBtnEvent, this);
		m_homeBtn.addClickedCallback(homeBtnEvent, this);
		m_macrosBtn.addClickedCallback(macrosBtnEvent, this);
		m_menuBtn.addClickedCallback(menuBtnEvent, this);

		m_appDrawer.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_appDrawer.setAlign(LV_ALIGN_RIGHT_MID, 0, 0);
		m_appDrawer.hide(true);

		m_appDrawerModalBg.setSize(lv_obj_get_width(getScreen()), LV_PCT(100));
		m_appDrawerModalBg.hide(true);
		m_appDrawerModalBg.addEventCallback(
			[](lv_event_t* e)
			{
				auto& sidebar = *static_cast<SideBar*>(lv_event_get_user_data(e));
				sidebar.showAppDrawer(false);
			},
			LV_EVENT_CLICKED,
			this);

		enableBackButton(false);
		enableHomeButton(false);

		addStyle(Themes::getComponentStyles().sidebar, LV_PART_MAIN);
		addStyle(Themes::getLvglStyles().pad_zero);
		m_btns.addStyle(Themes::getLvglStyles().bg_dark);
		m_appDrawerModalBg.addStyle(Themes::getLvglStyles().bg_modal);
	}

	void SideBar::enableHomeButton(bool enable)
	{
		m_homeBtn.setDisabled(!enable);
	}

	void SideBar::enableBackButton(bool enable)
	{
		m_backBtn.setDisabled(!enable);
		showAppDrawer(false);
	}

	void SideBar::backBtnEvent(lv_event_t* e)
	{
		UI::back();
	}

	void SideBar::homeBtnEvent(lv_event_t* e)
	{
		UI::home();
	}

	void SideBar::macrosBtnEvent(lv_event_t* e)
	{
		LOG_INFO("Macros button pressed");
		FileView& macrosView = HomeView::instance().getMacroView();
		macrosView.getPresenter()->setBaseFolder(FilePresenter::BaseFolder::MACROS);
		openScreen(&macrosView, true);
	}

	void SideBar::menuBtnEvent(lv_event_t* e)
	{
		LOG_DBG("Menu button pressed");
		SideBar& sidebar = *static_cast<SideBar*>(lv_event_get_user_data(e));

		sidebar.showAppDrawer(!sidebar.m_appDrawer.hasState(LV_STATE_USER_1));
	}

	void SideBar::showAppDrawer(bool show, bool animate)
	{
		if (show == m_appDrawer.hasState(LV_STATE_USER_1))
		{
			LOG_DBG("App drawer is already {}", show ? "shown" : "hidden");
			return;
		}

		m_appDrawer.updateLayout();
		m_appDrawer.setState(LV_STATE_USER_1, show);
		m_appDrawerModalBg.setVisible(show);
		int32_t end = show ? m_appDrawer.getWidth() : 0;

		if (animate == LV_ANIM_ON)
		{
			LvAnim anim;
			anim.setDuration(300);
			anim.setVar(this);
			int32_t start = m_appDrawer.getX();
			anim.setValues(start, end);
			anim.setExecCb(
				[](void* var, int32_t value)
				{
					auto& drawer = static_cast<SideBar*>(var)->m_appDrawer;
					drawer.setX(value);
				});
			anim.setDeletedCb(
				[](lv_anim_t* anim)
				{
					auto& sideBar = *static_cast<SideBar*>(anim->var);
					sideBar.m_appDrawer.setVisible(sideBar.m_appDrawer.hasState(LV_STATE_USER_1));
					sideBar.m_appDrawerModalBg.setVisible(sideBar.m_appDrawer.hasState(LV_STATE_USER_1));
				});

			if (show)
			{
				m_appDrawerModalBg.setVisible(true);
				m_appDrawer.setVisible(true);
			}
			anim.start();
		}
		else
		{
			m_appDrawer.setX(end);
			m_appDrawer.setVisible(show);
		}
	}

	void SideBar::onShow()
	{
		m_appDrawer.init();
	}
} // namespace UI
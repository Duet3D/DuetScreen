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
#include "i18n/i18n.h"

#define ESTOP_SIZE 80
#define APP_DRAWER_ICON "apps.png"
#define APP_DRAWER_CLOSE_ICON "menu_close.png"

#define SIDE_BAR_TEXT 0

namespace UI
{
	SideBar::SideBar(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		LOG_VERBOSE("Creating SideBar");

		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_btns.setSize(LV_PCT(100), LV_PCT(100));
		m_btns.setMinWidth(LV_SIZE_CONTENT);
		m_btns.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_btns.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_btns.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_homeBtn.setWidth(LV_PCT(100));
		m_homeBtn.setMinWidth(LV_SIZE_CONTENT);
		m_homeBtn.setFlexGrow(1);
		m_homeBtn.setText(_("side_bar.home"));
		m_homeBtn.setIcon("home.png");
		m_homeBtn.addClickedCallback(homeBtnEvent, this);
		m_homeBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_homeBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);

#if SIDE_BAR_BACK_BUTTON
		m_backBtn.setWidth(LV_PCT(100));
		m_backBtn.setMinWidth(LV_SIZE_CONTENT);
		m_backBtn.setFlexGrow(1);
		m_backBtn.setText(_("side_bar.back"));
		m_backBtn.setIcon("back.png");
		m_backBtn.addClickedCallback(backBtnEvent, this);
		m_backBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_backBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);
#endif

		m_controlBtn.setWidth(LV_PCT(100));
		m_controlBtn.setMinWidth(LV_SIZE_CONTENT);
		m_controlBtn.setFlexGrow(1);
		m_controlBtn.setText(_("side_bar.control"));
		m_controlBtn.setIcon("control.png");
		m_controlBtn.addClickedCallback(controlBtnEvent, this);
		m_controlBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_controlBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);

		m_filesBtn.setWidth(LV_PCT(100));
		m_filesBtn.setMinWidth(LV_SIZE_CONTENT);
		m_filesBtn.setFlexGrow(1);
		m_filesBtn.setText(_("side_bar.files"));
		m_filesBtn.setIcon("macros.png");
		m_filesBtn.addClickedCallback(filesBtnEvent, this);
		m_filesBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_filesBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);

#if SIDE_BAR_APP_DRAWER
		m_menuBtn.setWidth(LV_PCT(100));
		m_menuBtn.setMinWidth(LV_SIZE_CONTENT);
		m_menuBtn.setFlexGrow(1);
		m_menuBtn.setText(_("side_bar.menu"));
		m_menuBtn.setIcon(APP_DRAWER_ICON);
		m_menuBtn.addClickedCallback(menuBtnEvent, this);
		m_menuBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_menuBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);
#endif

#if SIDE_BAR_CONSOLE_BUTTON
		m_consoleBtn.setWidth(LV_PCT(100));
		m_consoleBtn.setMinWidth(LV_SIZE_CONTENT);
		m_consoleBtn.setFlexGrow(1);
		m_consoleBtn.setText(_("side_bar.console"));
		m_consoleBtn.setIcon("console.png");
		m_consoleBtn.addClickedCallback(consoleBtnEvent, this);
		m_consoleBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_consoleBtn.addStyle(Themes::getComponentStyles().console_btn_anim, LV_STATE_USER_1);
		m_consoleBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);
#endif

#if SIDE_BAR_SETTINGS_BUTTON
		m_settingsBtn.setWidth(LV_PCT(100));
		m_settingsBtn.setMinWidth(LV_SIZE_CONTENT);
		m_settingsBtn.setFlexGrow(1);
		m_settingsBtn.setText(_("side_bar.settings"));
		m_settingsBtn.setIcon("settings.png");
		m_settingsBtn.addClickedCallback(settingsBtnEvent, this);
		m_settingsBtn.addStyle(Themes::getComponentStyles().sidebar_btn);
		m_settingsBtn.getLabel().setLongMode(LV_LABEL_LONG_MODE_CLIP);
#endif

		setExtDrawSize(lv_obj_get_width(getScreenPtr()));
		m_btns.setExtDrawSize(400);

		m_eStopBtn.setSize(ESTOP_SIZE, ESTOP_SIZE);

#if SIDE_BAR_APP_DRAWER
		m_appDrawer.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_appDrawer.setAlign(LV_ALIGN_RIGHT_MID, 0, 0);
		m_appDrawer.hide(true);

		m_appDrawerModalBg.setSize(lv_obj_get_width(getScreenPtr()), LV_PCT(100));
		m_appDrawerModalBg.hide(true);
		m_appDrawerModalBg.addEventCallback(
			[](lv_event_t* e)
			{
				auto& sidebar = *static_cast<SideBar*>(lv_event_get_user_data(e));
				sidebar.showAppDrawer(false);
			},
			LV_EVENT_CLICKED,
			this);
#endif

#if SIDE_BAR_BACK_BUTTON
		enableBackButton(false);
#endif
		enableHomeButton(false);

		addStyle(Themes::getComponentStyles().sidebar, LV_PART_MAIN);
		addStyle(Themes::getLvglStyles().pad_zero);
		m_btns.addStyle(Themes::getLvglStyles().bg_dark);
#if SIDE_BAR_APP_DRAWER
		m_appDrawerModalBg.addStyle(Themes::getLvglStyles().bg_modal);
#endif
	}

	void SideBar::enableHomeButton(bool enable)
	{
		ZoneScoped;
		m_homeBtn.setDisabled(!enable);
	}

	void SideBar::setSelectedView(LvObj* view)
	{
		ZoneScoped;
		auto& homeView = HomeView::instance();

		const bool isHomeSelected = (view == nullptr);
		const bool isControlSelected = (view == &homeView.getControlView());
		const bool isFilesSelected = (view == &homeView.getFileView());
#if SIDE_BAR_CONSOLE_BUTTON
		const bool isConsoleSelected = (view == &homeView.getConsoleView());
#endif
#if SIDE_BAR_SETTINGS_BUTTON
		const bool isSettingsSelected = (view == &homeView.getSettingsView());
#endif

		m_homeBtn.setChecked(isHomeSelected);
		m_controlBtn.setChecked(isControlSelected);
		m_filesBtn.setChecked(isFilesSelected);
#if SIDE_BAR_CONSOLE_BUTTON
		m_consoleBtn.setChecked(isConsoleSelected);
#endif
#if SIDE_BAR_SETTINGS_BUTTON
		m_settingsBtn.setChecked(isSettingsSelected);
#endif
	}

#if SIDE_BAR_BACK_BUTTON
	void SideBar::enableBackButton(bool enable)
	{
		ZoneScoped;
		m_backBtn.setDisabled(!enable);
		showAppDrawer(false);
	}

	void SideBar::backBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		UI::back();
	}
#endif

	void SideBar::animateResponse(const ResponseType /* type */)
	{
		ZoneScoped;
#if SIDE_BAR_CONSOLE_BUTTON
		LvAnim anim;
		anim.setDuration(500);
		anim.setValues(0, 100);
		anim.setExecCb(
			[](void* var, int32_t value)
			{
				ZoneScopedN("SideBar::animateResponse ExecCb");
				auto& sideBar = *static_cast<SideBar*>(var);
				sideBar.m_consoleBtn.setStyleRecolorOpa(static_cast<lv_opa_t>(value));
			});
		anim.setCompletedCb(
			[](lv_anim_t* anim)
			{
				ZoneScopedN("SideBar::animateResponse CompletedCb");
				auto& sideBar = *static_cast<SideBar*>(anim->var);
				sideBar.m_consoleBtn.setStyleRecolorOpa(LV_OPA_0);
				sideBar.m_consoleBtn.setState(LV_STATE_USER_1, false);
			});
		anim.setPathCb(lv_anim_path_ease_in_out);
		anim.setRepeatCount(1);
		anim.setVar(this);
		anim.setReverseDuration(200);
		m_consoleBtn.setState(LV_STATE_USER_1, true);
		anim.start();
#endif
	}

	void SideBar::homeBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		UI::home();
	}

	void SideBar::controlBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		LOG_INFO("Control button pressed");
		HomeView& homeView = HomeView::instance();
		openScreen(&homeView.getControlView(), true);
	}

	void SideBar::filesBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		LOG_INFO("Files button pressed");
		auto& view = HomeView::instance().getFileView();
		openScreen(&view, true);
	}

#if SIDE_BAR_APP_DRAWER
	void SideBar::menuBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		LOG_DBG("Menu button pressed");
		SideBar& sidebar = *static_cast<SideBar*>(lv_event_get_user_data(e));

		sidebar.showAppDrawer(!sidebar.m_appDrawer.hasState(LV_STATE_USER_1));
	}

	void SideBar::showAppDrawer(bool show, bool animate)
	{
		ZoneScoped;
		if (show == m_appDrawer.hasState(LV_STATE_USER_1))
		{
			LOG_DBG("App drawer is already {}", show ? "shown" : "hidden");
			return;
		}

		m_menuBtn.setIcon(show ? APP_DRAWER_CLOSE_ICON : APP_DRAWER_ICON);
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
#endif

#if SIDE_BAR_CONSOLE_BUTTON
	void SideBar::consoleBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		LOG_INFO("Console button pressed");
		HomeView& homeView = HomeView::instance();
		openScreen(&homeView.getConsoleView(), true);
	}
#endif

#if SIDE_BAR_SETTINGS_BUTTON
	void SideBar::settingsBtnEvent(lv_event_t*)
	{
		ZoneScoped;
		LOG_INFO("Settings button pressed");
		HomeView& homeView = HomeView::instance();
		openScreen(&homeView.getSettingsView(), true);
	}
#endif

	void SideBar::onShow()
	{
		ZoneScoped;
#if SIDE_BAR_APP_DRAWER
		m_appDrawer.init();
#endif
	}
} // namespace UI
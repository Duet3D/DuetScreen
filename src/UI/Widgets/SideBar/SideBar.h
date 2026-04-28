/*
 * Sidebar.h
 *
 *  Created on: 2025-01-16
 *      Author: Andy Everitt
 */

#pragma once

#include "EStop.h"
#include "SideBarPresenter.h"
#include "UI/Components/AppDrawer/AppDrawer.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Button/DraggableButton.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Core/View.h"

#define SIDE_BAR_BACK_BUTTON 0
#define SIDE_BAR_APP_DRAWER 0
#define SIDE_BAR_SETTINGS_BUTTON 1
#define SIDE_BAR_CONSOLE_BUTTON !CONSOLE_SIDE_PANEL

namespace UI
{
	class SideBar : public View<SideBarPresenter>
	{
	  public:
		SideBar(const std::string& name, LvObj& parent);

		void enableHomeButton(bool enable);
		void setSelectedView(LvObj* view);
#if SIDE_BAR_BACK_BUTTON
		void enableBackButton(bool enable);
#endif

#if SIDE_BAR_APP_DRAWER
		void showAppDrawer(bool show, bool animate = LV_ANIM_ON);
#endif

		void animateResponse(const ResponseType type);

	  private:
#if SIDE_BAR_BACK_BUTTON
		static void backBtnEvent(lv_event_t* e);
#endif
		static void homeBtnEvent(lv_event_t* e);
		static void filesBtnEvent(lv_event_t* e);
		static void controlBtnEvent(lv_event_t* e);
#if SIDE_BAR_APP_DRAWER
		static void menuBtnEvent(lv_event_t* e);
#endif
#if SIDE_BAR_CONSOLE_BUTTON
		static void consoleBtnEvent(lv_event_t* e);
#endif
#if SIDE_BAR_SETTINGS_BUTTON
		static void settingsBtnEvent(lv_event_t* e);
#endif

		void onShow() override;

		LvContainer m_btns{"buttons", getRoot()};
		Button m_homeBtn{"home", m_btns};
#if SIDE_BAR_BACK_BUTTON
		Button m_backBtn{"back", m_btns};
#endif
		Button m_controlBtn{"control", m_btns};
#if SIDE_BAR_APP_DRAWER
		Button m_menuBtn{"menu", m_btns};
#endif
		Button m_filesBtn{"macros", m_btns};
#if SIDE_BAR_CONSOLE_BUTTON
		Button m_consoleBtn{"console", m_btns};
#endif
#if SIDE_BAR_SETTINGS_BUTTON
		Button m_settingsBtn{"settings", m_btns};
#endif
		EStop m_eStopBtn{"estop", m_btns};
#if SIDE_BAR_APP_DRAWER
		AppDrawer m_appDrawer{"app_drawer", getRoot()};
		LvContainer m_appDrawerModalBg{"app_drawer_modal_bg", getRoot()};
#endif
	};
} // namespace UI

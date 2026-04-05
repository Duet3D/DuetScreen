/*
 * SideBarPresenter.cpp
 *
 *  Created on: 2025-02-21
 *      Author: Andy Everitt
 */

#include "SideBarPresenter.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "SideBar.h"
#include <string>
#include <thread>

namespace UI
{
#if SIDE_BAR_BACK_BUTTON
	void SideBarPresenter::enableBackButton(bool enable)
	{
		ZoneScoped;
		m_view->enableBackButton(enable);
	}
#endif

	void SideBarPresenter::enableHomeButton(bool enable)
	{
		ZoneScoped;
		m_view->enableHomeButton(enable);
	}

	void SideBarPresenter::setSelectedView(LvObj* view)
	{
		ZoneScoped;
		m_view->setSelectedView(view);
	}

	void SideBarPresenter::newResponse(const ResponseType type, const std::string& /* message */)
	{
		ZoneScoped;
		m_view->animateResponse(type);
	}

	void SideBarPresenter::onInit()
	{
		ZoneScoped;
#if SIDE_BAR_BACK_BUTTON
		registerEventListener<EventType::NavigationBackEnable>(this, &SideBarPresenter::enableBackButton);
#endif
#if 0
		registerEventListener<EventType::NavigationHomeEnable>(this, &SideBarPresenter::enableHomeButton);
#else
		enableHomeButton(true);
#endif
		registerEventListener<EventType::NavigationCurrentScreen>(this, &SideBarPresenter::setSelectedView);
		registerEventListener<EventType::Response>(this, &SideBarPresenter::newResponse);
	}

	void SideBarPresenter::onActivate() {}
} // namespace UI

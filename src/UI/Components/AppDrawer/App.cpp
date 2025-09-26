/*
 * App.cpp
 *
 *  Created on: 2025-08-05
 *      Author: Andy Everitt
 */

#include "App.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	App::App(size_t index, LvObj& parent)
		: ListItem(index, parent)
		, m_button("app_button", getRoot())
	{
		addStyle(Themes::getLvglStyles().pad_zero);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_button.setSize(LV_PCT(100), LV_PCT(100));

		m_button.addClickedCallback(appClickedEvent, this);
	}

	void App::appClickedEvent(lv_event_t* e)
	{
		UI_LOCK();
		auto* app = static_cast<App*>(lv_event_get_user_data(e));
		if (app == nullptr)
		{
			LOG_ERROR("App clicked event received with null user data");
			return;
		}

		LvContainer* screen = static_cast<LvContainer*>(app->getUserData());
		if (screen == nullptr)
		{
			LOG_ERROR("App clicked event received with null screen user data");
			return;
		}

		openScreen(screen, true);
	}
} // namespace UI

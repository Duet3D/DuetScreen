/*
 * AppDrawer.cpp
 *
 *  Created on: 2025-08-05
 *      Author: Andy Everitt
 */

#include "AppDrawer.h"
#include "Debug.h"
#include "UI/Screens/Home/HomeView.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	struct AppInfo
	{
		std::string_view name;
		LvContainer* screen;
	};

	AppDrawer::AppDrawer(const std::string& name, LvObj& parent)
		: Card(name, parent)
		, m_apps("apps", getRoot())
	{
		setMinWidth(LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_apps.setWidth(LV_SIZE_CONTENT);
		m_apps.setFlexGrow(1);
		m_apps.setTitle(_("select_view"));

		auto& list_cont = m_apps.getListContainer();
		list_cont.setWidth(LV_SIZE_CONTENT);
		// list_cont.setFlag(LV_OBJ_FLAG_SCROLLABLE, true);
		list_cont.setFlexGrow(1);
		list_cont.setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
		list_cont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	}

	void AppDrawer::init()
	{
		static const AppInfo apps[] = {
			{_("console"), &HomeView::instance().getConsoleView()},
			{_("move"), &HomeView::instance().getMoveView()},
			{_("temperature"), &HomeView::instance().getTemperatureView()},
			// {_("fan"), &HomeView::instance().getFanView()},
			{_("fine_tune"), &HomeView::instance().getFineTuneView()},
			// {_("macros"), &HomeView::instance().getMacroView()},
			{_("heightmap"), &HomeView::instance().getHeightmapView()},
			{_("settings"), &HomeView::instance().getSettingsView()},
			{_("status"), &HomeView::instance().getStatusView()},
		};

		m_apps.setItemCount(std::size(apps),
							[](size_t index, LvObj& parent)
							{
								auto app = std::make_shared<App>(index, parent);
								const auto& appInfo = apps[index];
								app->setName(appInfo.name);
								app->setSize(100, 100);
								app->setMinWidth(LV_SIZE_CONTENT);
								// app->setMinHeight(LV_SIZE_CONTENT);
								// app->setFlexGrow(1);
								app->setUserData((void*)appInfo.screen);
								return app;
							});
	}
} // namespace UI

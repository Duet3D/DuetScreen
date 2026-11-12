/*
 * AppDrawer.cpp
 *
 *  Created on: 2025-08-05
 *      Author: Andy Everitt
 */

#include "AppDrawer.h"
#include "Debug.h"
#include "UI/Widgets/SideBar/SideBar.h"
#include "i18n/i18n.h"

namespace UI
{
	struct AppInfo
	{
		std::string_view name;
		const char* screen; // Name of the screen to open (see Navigation.h)
		std::string_view icon;
	};

	AppDrawer::AppDrawer(const std::string& name, LvObj& parent)
		: Card(name, parent)
		, m_apps("apps", getRoot())
	{
		ZoneScoped;
		setMinWidth(LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_apps.setWidth(LV_SIZE_CONTENT);
		m_apps.setFlexGrow(1);
		m_apps.setTitle(_("app_drawer.select_view"));

		auto& list_cont = m_apps.getListContainer();
		list_cont.setWidth(LV_SIZE_CONTENT);
		// list_cont.setFlag(LV_OBJ_FLAG_SCROLLABLE, true);
		list_cont.setFlexGrow(1);
		list_cont.setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
		list_cont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	}

	void AppDrawer::init()
	{
		ZoneScoped;
		// Screens by the name they are registered under (see Navigation.h). Without the app drawer's own screens,
		// those apps open the control screen, which has the same pages as tabs.
#if SIDE_BAR_APP_DRAWER
		static constexpr bool ownScreens = true;
#else
		static constexpr bool ownScreens = false;
#endif
		static const AppInfo apps[] = {
			{.name = _("app_drawer.console"), .screen = "console", .icon = "console.png"},
			{.name = _("app_drawer.move"), .screen = ownScreens ? "move" : "control", .icon = "move.png"},
			{.name = _("app_drawer.temperature"),
			 .screen = ownScreens ? "temperature" : "control",
			 .icon = "temperature.png"},
			{.name = _("app_drawer.fine_tune"),
			 .screen = ownScreens ? "fine_tune" : "control",
			 .icon = "fine_tune.png"},
			{.name = _("app_drawer.heightmap"),
			 .screen = ownScreens ? "heightmap" : "control",
			 .icon = "heightmap.png"},
			{.name = _("app_drawer.object_cancel"),
			 .screen = ownScreens ? "object_cancel" : "control",
			 .icon = "cancel.png"},
			{.name = _("app_drawer.settings"), .screen = "settings", .icon = "settings.png"},
		};

		m_apps.setItemCount(std::size(apps),
							[](size_t index, LvObj& parent)
							{
								auto app = std::make_unique<App>(index, parent);
								const auto& appInfo = apps[index];
								app->setName(appInfo.name);
								app->setIcon(appInfo.icon);
								app->setSize(100, 100);
								app->setMinWidth(LV_SIZE_CONTENT);
								// app->setMinHeight(LV_SIZE_CONTENT);
								// app->setFlexGrow(1);
								app->setUserData(const_cast<char*>(appInfo.screen));
								return app;
							});
	}
} // namespace UI

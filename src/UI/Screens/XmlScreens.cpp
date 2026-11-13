/*
 * XmlScreens.cpp
 */

#include "XmlScreens.h"
#include "Console/ConsolePresenter.h"
#include "Fan/FanPresenter.h"
#include "File/FileBrowserPresenter.h"
#include "Heightmap/HeightmapPresenter.h"
#include "Modals/FactorsPresenter.h"
#include "Modals/NumberPadPresenter.h"
#include "Move/MovePresenter.h"
#include "Notifications/NotificationsPresenter.h"
#include "ObjectCancel/ObjectCancelPresenter.h"
#include "Settings/ConnectionSettingsPresenter.h"
#include "Settings/DeveloperSettingsPresenter.h"
#include "Settings/DisplaySettingsPresenter.h"
#include "Settings/GeneralSettingsPresenter.h"
#include "Settings/ThemePreviewPresenter.h"
#include "Settings/WifiPresenter.h"
#include "Status/JobStatusPresenter.h"
#include "StatusBar/StatusBarPresenter.h"
#include "Temperature/ExtruderPresenter.h"
#include "Temperature/HeaterActions.h"
#include "Temperature/TemperaturePresenter.h"
#include "UI/Widgets/BabyStep/BabyStepPresenter.h"
#include "UI/Widgets/EStop/EStop.h"
#include "UI/Xml/View/NativeViews.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	/** Make a file browser presenter with `settings` available as `name` */
	static void addFileBrowser(std::string name, FileBrowserPresenter::Settings settings)
	{
		Xml::Presenters::add(name,
							 [name, settings]() { return std::make_shared<FileBrowserPresenter>(name, settings); });
	}

	void registerXmlScreens()
	{
		/* Actions every view can raise */
		HeaterActions::registerAll();
		FactorsPresenter::registerActions();

		/* Presenters */
		Xml::Presenters::add<FanPresenter>("fans");
		Xml::Presenters::add<StatusBarPresenter>("status_bar");
		Xml::Presenters::add<JobStatusPresenter>("dashboard_job_status");
		Xml::Presenters::add<NotificationsPresenter>("notifications");
		Xml::Presenters::add<NumberPadPresenter>("numberpad");
		Xml::Presenters::add<FactorsPresenter>("factors");
		Xml::Presenters::add<ConsolePresenter>("console");
		Xml::Presenters::add<MovePresenter>("move");
		Xml::Presenters::add<BabyStepPresenter>("babystep");
		Xml::Presenters::add<TemperaturePresenter>("temperature");
		Xml::Presenters::add<ExtruderPresenter>("temperature_extrusion");
		Xml::Presenters::add<HeightmapPresenter>("heightmap");
		Xml::Presenters::add<ObjectCancelPresenter>("object_cancel");
		Xml::Presenters::add<GeneralSettingsPresenter>("settings_general");
		Xml::Presenters::add<ConnectionSettingsPresenter>("settings_connection");
		Xml::Presenters::add<WifiPresenter>("settings_wifi");
		Xml::Presenters::add<DisplaySettingsPresenter>("settings_display");
		Xml::Presenters::add<ThemePreviewPresenter>("settings_theme_preview");
		Xml::Presenters::add<DeveloperSettingsPresenter>("settings_developer");

		/* The file browsers (file_browser.xml): one per placement, each remembering its own order and display */
		using Browser = FileBrowserPresenter;
		using SortBy = Browser::SortBy;
		using Mode = Browser::DisplayMode;
		addFileBrowser("dashboard_jobs",
					   {.folder = Browser::BaseFolder::GCODES,
						.sortBy = {"ui:dashboard:file:jobs:sort_by", SortBy::DATE},
						.sortDescending = {"ui:dashboard:file:jobs:sort_descending", true},
						.displayMode = {"ui:dashboard:file:jobs:display_mode", Mode::List}});
		addFileBrowser("dashboard_macros",
					   {.folder = Browser::BaseFolder::MACROS,
						.sortBy = {"ui:dashboard:file:macros:sort_by", SortBy::NAME},
						.sortDescending = {"ui:dashboard:file:macros:sort_descending", false},
						.displayMode = {"ui:dashboard:file:macros:display_mode", Mode::List}});
		addFileBrowser("files_macros",
					   {.folder = Browser::BaseFolder::MACROS,
						.sortBy = {"ui:home:file:macros:sort_by", SortBy::NAME},
						.sortDescending = {"ui:home:file:macros:sort_descending", false},
						.displayMode = {"ui:home:file:macros:display_mode", Mode::List}});
		addFileBrowser("files_jobs",
					   {.folder = Browser::BaseFolder::GCODES,
						.sortBy = {"ui:home:file:jobs:sort_by", SortBy::DATE},
						.sortDescending = {"ui:home:file:jobs:sort_descending", true},
						.displayMode = {"ui:home:file:jobs:display_mode", Mode::Tile}});

		/* C++ views, until they are written in XML */
		Xml::NativeViews::add<LvContainer>("container");
		Xml::NativeViews::add<EStop>("estop");
	}
} // namespace UI

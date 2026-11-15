/*
 * Presenters.cpp
 */

#include "Presenters.h"
#include "Common/BabystepActions.h"
#include "Console/ConsoleActions.h"
#include "Console/GcodeReferencePresenter.h"
#include "Control/Fans/FanActions.h"
#include "Control/Heightmap/HeightmapPresenter.h"
#include "Control/Move/MoveActions.h"
#include "Control/ObjectCancel/ObjectCancelPresenter.h"
#include "Control/Temperature/ExtruderActions.h"
#include "Control/Temperature/FilamentActions.h"
#include "Control/Temperature/HeaterActions.h"
#include "Control/Temperature/SpindleActions.h"
#include "Control/Temperature/TemperaturePresenter.h"
#include "Control/Temperature/ToolActions.h"
#include "Customise/CustomiseActions.h"
#include "Customise/CustomisePresenter.h"
#include "Dashboard/JobActions.h"
#include "Dashboard/JobStatusPresenter.h"
#include "FileBrowser/FileBrowserPresenter.h"
#include "FileBrowser/FilesActions.h"
#include "HardwareTest/HardwareTestPresenter.h"
#include "Modals/FactorsActions.h"
#include "Notifications/NotificationsPresenter.h"
#include "Settings/ConnectionSettings.h"
#include "Settings/DeveloperSettings.h"
#include "Settings/DisplaySettings.h"
#include "Settings/GeneralSettings.h"
#include "Settings/NotificationSettings.h"
#include "Settings/SettingsActions.h"
#include "Settings/SystemActions.h"
#include "Settings/ThemePreviewPresenter.h"
#include "Settings/WifiPresenter.h"
#include "Sidebar/EStopActions.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	/** Make a file browser presenter with `settings` available as `name` */
	static void addFileBrowser(std::string name, FileBrowserPresenter::Settings settings)
	{
		Xml::Presenters::add(name,
							 [name, settings]() { return std::make_shared<FileBrowserPresenter>(name, settings); });
	}

	void registerPresenters()
	{
		/* The settings: source.settings.* */
		GeneralSettings::registerAll();
		DisplaySettings::registerAll();
		NotificationSettings::registerAll();
		ConnectionSettings::registerAll();
		DeveloperSettings::registerAll();

		/* Actions every view can raise */
		HeaterActions::registerAll();
		FanActions::registerAll();
		ConsoleActions::registerAll();
		ExtruderActions::registerAll();
		FilamentActions::registerAll();
		SpindleActions::registerAll();
		MoveActions::registerAll();
		SettingsActions::registerAll();
		SystemActions::registerAll();
		ToolActions::registerAll();
		FactorsActions::registerAll();
		FilesActions::registerAll();
		EStopActions::registerAll();
		JobActions::registerAll();
		BabystepActions::registerAll();
		CustomiseActions::registerAll();

		/* Presenters */
		Xml::Presenters::add<HardwareTestPresenter>("hardware_test");
		Xml::Presenters::add<NotificationsPresenter>("notifications");
		Xml::Presenters::add<CustomisePresenter>("customise");
		Xml::Presenters::add<GcodeReferencePresenter>("gcode_reference");
		Xml::Presenters::add<JobStatusPresenter>("job_status");
		Xml::Presenters::add<TemperaturePresenter>("temperature");
		Xml::Presenters::add<HeightmapPresenter>("heightmap");
		Xml::Presenters::add<ObjectCancelPresenter>("object_cancel");
		Xml::Presenters::add<WifiPresenter>("settings_wifi");
		Xml::Presenters::add<ThemePreviewPresenter>("settings_theme_preview");

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
	}
} // namespace UI

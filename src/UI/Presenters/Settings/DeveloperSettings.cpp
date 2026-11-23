/*
 * DeveloperSettings.cpp
 */

#include "DeveloperSettings.h"
#include "Comm/Connection.h"
#include "Configuration.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Styles/Styles.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include <string>
#include <vector>

namespace UI::DeveloperSettings
{
	namespace
	{
#if DEVELOPER_MODE
		/** The setting that turns the system's service `which`, called `name`, on and off */
		Xml::Setting service(const std::string& name, SystemHelper::Services which)
		{
			return {.name = name,
					.description = "1 if " + name + " is on (developer mode builds)",
					.read = [which] { return SystemHelper::isServiceEnabled(which) ? 1 : 0; },
					.write = [which](int32_t on)
					{ on != 0 ? SystemHelper::enableService(which) : SystemHelper::disableService(which); }};
		}
#else
		/** The setting for a service only developer mode builds have, called `name`: always off */
		Xml::Setting service(const std::string& name)
		{
			return {.name = name,
					.description = "1 if " + name + " is on (developer mode builds)",
					.read = [] { return 0; },
					.write = [name](int32_t) { LOG_WARN("settings.{:s}: only in developer mode builds", name); }};
		}
#endif
	} // namespace

	void registerAll()
	{
		Xml::Settings::add({.name = "debug_level",
							.description = "Index of the level logged from",
							.read = [] { return static_cast<int32_t>(Log::GetDebugLevel()); },
							.write = [](int32_t level) { Log::SetDebugLevel(static_cast<Log::DebugLevel>(level)); },
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto& level : Log::DebugLevelStrings)
								{
									names.push_back(_(level));
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add({.name = "advanced_settings",
							.description = "1 if advanced settings are shown",
							.read = [] { return StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS) ? 1 : 0; },
							.write = [](int32_t on) { StorageHelper::setData(ID_ENABLE_ADVANCED_SETTINGS, on != 0); }});
		Xml::Settings::add({.name = "second_usb_channel",
							.description = "1 if the second USB channel is used",
							.read = [] { return StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL) ? 1 : 0; },
							.write =
								[](int32_t on)
							{
								StorageHelper::setData(ID_ENABLE_SECOND_USB_CHANNEL, on != 0);
								// The USB port chooses its channels as it opens
								if (Comm::Connection::get().type() == Comm::CommunicationType::usb)
								{
									Comm::Connection::get().reconnect();
								}
							}});
		Xml::Settings::add({.name = "system_logging",
							.description = "1 if system messages are logged to the console",
							.read = [] { return StorageHelper::getData(ID_ENABLE_UI_LOGGING) ? 1 : 0; },
							.write =
								[](int32_t on)
							{
								StorageHelper::setData(ID_ENABLE_UI_LOGGING, on != 0);
								Log::EnableUiLogging(on != 0);
							}});
		// In every build, so the UI API is the same in all; source.system.* says whether each does anything
		Xml::Settings::add({.name = "debug_borders",
							.description = "1 if debug borders are shown (builds with debug borders)",
							.read =
								[]
							{
#if DEBUG_BORDERS
								return Themes::isdebugBorderVisible(lv_screen_active()) ? 1 : 0;
#else
								return 0;
#endif
							},
							.write =
								[]([[maybe_unused]] int32_t on)
							{
#if DEBUG_BORDERS
								StorageHelper::setData(ID_DEBUG_BORDERS, on != 0);
								Themes::showDebugBorders(lv_screen_active(), on != 0);
#else
								LOG_WARN("settings.debug_borders: this build has no debug borders");
#endif
							}});
#if DEVELOPER_MODE
		Xml::Settings::add(service("ssh", SystemHelper::Services::SSH));
		Xml::Settings::add(service("adb", SystemHelper::Services::ADB));
#else
		Xml::Settings::add(service("ssh"));
		Xml::Settings::add(service("adb"));
#endif
		Xml::Settings::add({.name = "system_monitor",
							.description = "1 if the system monitor is shown",
							.read = [] { return StorageHelper::getData(ID_SYSTEM_MONITOR_ENABLED) ? 1 : 0; },
							.write =
								[](int32_t on)
							{
								StorageHelper::setData(ID_SYSTEM_MONITOR_ENABLED, on != 0);
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
								on != 0 ? lv_sysmon_show_performance(NULL) : lv_sysmon_hide_performance(NULL);
#endif
#if LV_USE_SYSMON && LV_USE_MEM_MONITOR
								on != 0 ? lv_sysmon_show_memory(NULL) : lv_sysmon_hide_memory(NULL);
#endif
							}});
	}
} // namespace UI::DeveloperSettings

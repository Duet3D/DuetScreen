/*
 * Storage.cpp
 *
 *  Created on: 2026-02-04
 *      Author: Andy Everitt
 */

#include "Storage.h"
#include "Configuration.h"

/* For types */
#include "Comm/Usb.h"						 // Comm::UsbMode
#include "Debug.h"							 // Log::DebugLevel
#include "Hardware/Duet.h"					 // Comm::CommunicationType
#include "ObjectModel/Files.h"				 // OM::FileSystem::SortBy
#include "Subscribers/ResponseSubscribers.h" // ResponseType
#include "utils/SystemHelper.h"				 // SystemHelper::Services
#include "utils/UnitSystem.h"				 // Units::UnitSystem

/* Convertors */
namespace std::chrono
{
	void to_json(nlohmann::json& j, const std::chrono::seconds& c)
	{
		j = c.count();
	}
	void from_json(const nlohmann::json& j, std::chrono::seconds& c)
	{
		c = std::chrono::seconds(j.get<int>());
	}
	void to_json(nlohmann::json& j, const std::chrono::milliseconds& c)
	{
		j = c.count();
	}
	void from_json(const nlohmann::json& j, std::chrono::milliseconds& c)
	{
		c = std::chrono::milliseconds(j.get<int>());
	}
} // namespace std::chrono

/* Duet */
constexpr StorageKey<std::string_view> ID_DUET_IP_ADDRESS = {"duet:hostname", DEFAULT_IP_ADDRESS};
constexpr StorageKey<std::string_view> ID_DUET_PASSWORD = {"duet:password", ""};
constexpr StorageKey<Comm::CommunicationType> ID_DUET_COMMUNICATION_TYPE = {"duet:communication_type",
																			Comm::CommunicationType::usb};
constexpr StorageKey<std::chrono::milliseconds> ID_DUET_POLL_INTERVAL = {"duet:poll_interval",
																		 DEFAULT_PRINTER_POLL_INTERVAL};
constexpr StorageKey<speed_t> ID_DUET_BAUD_RATE = {"duet:baud_rate", DEFAULT_BAUD_RATE};

/* UI */
constexpr StorageKey<std::string_view> ID_THEME = {"ui:theme", "duetscreen"};
constexpr StorageKey<std::string_view> ID_FONT = {"ui:font", "OpenSans"};
constexpr StorageKey<std::string_view> ID_ICON_FOLDER = {"ui:icon_folder", DEFAULT_ICON_SET};
constexpr StorageKey<std::string_view> ID_KEYBOARD_LAYOUT = {"ui:keyboard_layout", "us"};
constexpr StorageKey<bool> ID_UI_ANIMATIONS_ENABLED = {"ui:animations_enabled", true};

constexpr StorageKey<bool> ID_SCREENSAVER_ENABLE = {"ui:screensaver_enable", true};
constexpr StorageKey<std::chrono::seconds> ID_SCREENSAVER_TIMEOUT = {"ui:screensaver_timeout", DEFAULT_SCREEN_TIMEOUT};
constexpr StorageKey<std::chrono::milliseconds> ID_NOTIFICATION_TIMEOUT = {"ui:info_timeout", std::chrono::seconds(5)};
constexpr StorageKey<ResponseType> ID_NOTIFICATION_LEVEL = {"ui:notification_level", ResponseType::INFO};
constexpr StorageKey<bool> ID_NOTIFICATION_AUTO_CLOSE_ERROR = {"ui:notification_auto_close_error", false};

constexpr StorageKey<bool> ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED = {"ui:console_command_list_collapsed", false};

constexpr StorageKey<std::vector<float>, std::vector<float> (*)()> ID_BABYSTEP_AMOUNT = {
	"ui:baby_step_amount", +[]() -> std::vector<float> { return {0.01f, 0.05f}; }};
constexpr StorageKey<std::vector<float>, std::vector<float> (*)()> ID_MOVE_DISTANCES = {
	"ui:move:distances", +[]() -> std::vector<float> { return {0.1f, 0.5f, 1, 5, 10, 25, 50}; }};
constexpr StorageKey<std::vector<uint32_t>, std::vector<uint32_t> (*)()> ID_MOVE_FEEDRATES = {
	"ui:move:feedrates", +[]() -> std::vector<uint32_t> { return {5, 10, 25, 50, 100, 200, 300}; }};

constexpr StorageKey<bool> ID_SHOW_CONFIRMATION_DIALOGS = {"ui:show_confirmation_dialogs", true};

/* Multi value selectors */
// these will have the following sub keys {"values", "selected"}
constexpr std::string_view ID_MVS_EXTRUSION_FEEDRATES = "ui:extrusion:feedrates";
constexpr std::string_view ID_MVS_EXTRUSION_DISTANCES = "ui:extrusion:distances";

constexpr StorageKey<Units::UnitSystem> ID_UNIT_SYSTEM = {"ui:units", Units::UnitSystem::Metric};

/* System */
constexpr StorageKey<time_t> ID_UPGRADE_FILE_LAST_MODIFIED = {"sys:upgrade_file_last_modified", 0};
constexpr StorageKey<std::string_view> ID_SYS_LANG_CODE_KEY = {"sys:lang_code", DEFAULT_LANGUAGE_CODE};
constexpr StorageKey<unsigned int> ID_SYS_BRIGHTNESS_KEY = {"sys:brightness", 100u};
constexpr StorageKey<Comm::UsbMode> ID_USB_MODE = {"sys:usb_mode", Comm::UsbMode::Host};
constexpr StorageKey<bool> ID_DISPLAY_CONNECTED_MESSAGE = {"sys:display_connected_message", true};
constexpr StorageKey<bool> ID_ENABLE_ADVANCED_SETTINGS = {"sys:enable_advanced_settings", false};

/* Debug */
constexpr StorageKey<Log::DebugLevel> ID_DEBUG_LEVEL = {"debug:level", Log::DebugLevel::Info};
constexpr StorageKey<std::string_view> ID_LOG_FILE = {"debug:file", DEFAULT_LOG_FILE};
constexpr StorageKey<bool> ID_ENABLE_UI_LOGGING = {"debug:ui_logging", false};
constexpr StorageKey<std::chrono::milliseconds> ID_BURNIN_FREQUENCY = {"debug:burnin_frequency",
																	   std::chrono::milliseconds(2000)};
constexpr StorageKey<bool> ID_SYSTEM_MONITOR_ENABLED = {"debug:system_monitor_enabled", false};
constexpr StorageKey<bool> ID_ENABLE_SECOND_USB_CHANNEL = {"developer:enable_second_usb_channel", false};

#if DEBUG_BORDERS
constexpr StorageKey<bool> ID_DEBUG_BORDERS = {"debug:borders", false};
#endif

#if DEVELOPER_MODE
/* Developer */
#  if 0
constexpr StorageKey<bool, bool (*)()> ID_SSH_ENABLED = {
	"developer:ssh_enabled", +[]() -> bool { return SystemHelper::isServiceEnabled(SystemHelper::Services::SSH); }};
constexpr StorageKey<bool, bool (*)()> ID_ADB_ENABLED = {
	"developer:adb_enabled", +[]() -> bool { return SystemHelper::isServiceEnabled(SystemHelper::Services::ADB); }};
#  endif
#endif

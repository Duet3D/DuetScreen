/*
 * DeveloperSettingsPresenter.cpp
 */

#include "DeveloperSettingsPresenter.h"
#include "Comm/FileInfo.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Directories.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/HardwareTest/HardwareTest.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include "utils/UnitSystem.h"
#include "utils/UpgradeHelper.h"
#include <filesystem>

namespace UI
{
	DeveloperSettingsPresenter::DeveloperSettingsPresenter()
		: XmlPresenter("settings_developer")
	{
		std::string levels;
		for (const auto& level : Log::DebugLevelStrings)
		{
			levels += levels.empty() ? "" : "\n";
			levels += _(level);
		}
		m_debugLevels.set(levels);
		m_debugBordersShown.set(DEBUG_BORDERS ? 1 : 0);
		m_developerMode.set(DEVELOPER_MODE ? 1 : 0);
		m_systemMonitorShown.set(LV_USE_SYSMON ? 1 : 0);

		action("settings.debug_level",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   m_debugLevel.set(static_cast<int32_t>(*value));
					   Log::SetDebugLevel(static_cast<Log::DebugLevel>(static_cast<int>(*value)));
				   }
			   });
		toggle("settings.advanced_settings",
			   m_advancedSettings,
			   [](bool on) { StorageHelper::setData(ID_ENABLE_ADVANCED_SETTINGS, on); });
		toggle("settings.second_usb_channel",
			   m_secondUsbChannel,
			   [](bool on)
			   {
				   StorageHelper::setData(ID_ENABLE_SECOND_USB_CHANNEL, on);
				   if (on && Comm::DUET.GetCommunicationType() == Comm::CommunicationType::usb &&
					   Comm::DUET.IsConnected())
				   {
					   Comm::DUET.SendGcode("M575 P1 S0\n", true);
				   }
			   });
		toggle("settings.system_logging",
			   m_systemLogging,
			   [](bool on)
			   {
				   StorageHelper::setData(ID_ENABLE_UI_LOGGING, on);
				   Log::EnableUiLogging(on);
			   });
#if DEBUG_BORDERS
		toggle("settings.debug_borders",
			   m_debugBorders,
			   [](bool on)
			   {
				   StorageHelper::setData(ID_DEBUG_BORDERS, on);
				   Themes::showDebugBorders(lv_screen_active(), on);
			   });
#endif
#if DEVELOPER_MODE
		toggle("settings.ssh",
			   m_ssh,
			   [](bool on)
			   {
				   on ? SystemHelper::enableService(SystemHelper::Services::SSH)
					  : SystemHelper::disableService(SystemHelper::Services::SSH);
			   });
		toggle("settings.adb",
			   m_adb,
			   [](bool on)
			   {
				   on ? SystemHelper::enableService(SystemHelper::Services::ADB)
					  : SystemHelper::disableService(SystemHelper::Services::ADB);
			   });
		action("settings.buildroot_setup",
			   [](const Xml::ActionArgs&) { SystemHelper::restartService(SystemHelper::Services::SETUP); });
#endif
		toggle("settings.system_monitor",
			   m_systemMonitor,
			   [](bool on)
			   {
				   StorageHelper::setData(ID_SYSTEM_MONITOR_ENABLED, on);
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
				   on ? lv_sysmon_show_performance(NULL) : lv_sysmon_hide_performance(NULL);
#endif
#if LV_USE_SYSMON && LV_USE_MEM_MONITOR
				   on ? lv_sysmon_show_memory(NULL) : lv_sysmon_hide_memory(NULL);
#endif
			   });

		action("settings.restart", [](const Xml::ActionArgs&) { Restart(); });
		action("settings.erase_and_restart", [](const Xml::ActionArgs&) { EraseAndRestart(); });
		action("settings.reboot", [](const Xml::ActionArgs&) { Reboot(); });
		action("settings.hardware_test", [this](const Xml::ActionArgs&) { startHardwareTest(); });
		action("settings.clear_cache",
			   [this](const Xml::ActionArgs&)
			   {
				   if (std::filesystem::exists(NVS_FOLDER))
				   {
					   std::filesystem::remove_all(NVS_FOLDER);
				   }
				   FILEINFO_CACHE->ClearCache();
				   showStorage();
			   });
		action("settings.upgrade_from_github",
			   [](const Xml::ActionArgs&)
			   {
				   LOG_INFO("User requested upgrade from GitHub latest release");
				   if (!UpgradeHelper::upgradeFromGithubLatest())
				   {
					   LOG_ERROR("Upgrade from GitHub latest failed to start");
					   Model::get().post<EventType::Response>(ResponseType::ERROR,
															  _("settings.upgrade_from_github_failed"));
				   }
			   });
		action("settings.send_config",
			   [](const Xml::ActionArgs&)
			   {
				   Comm::DUET.UploadFile(fmt::format("{:s}/duetscreen.json", OM::Directories::GetSystemDirectory()),
										 StorageHelper::dump());
			   });
	}

	DeveloperSettingsPresenter::~DeveloperSettingsPresenter() = default;

	void DeveloperSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("debug_levels", m_debugLevels, "Log levels offered, one per line");
		f.field("debug_level", m_debugLevel, "Index of the one in use");
		f.field("advanced_settings", m_advancedSettings, "1 if advanced settings are shown");
		f.field("second_usb_channel", m_secondUsbChannel, "1 if the second USB channel is used");
		f.field("system_logging", m_systemLogging, "1 if system messages are logged to the console");
		f.field("debug_borders_shown", m_debugBordersShown, "1 if the build has debug borders");
		f.field("debug_borders", m_debugBorders, "1 if debug borders are on");
		f.field("developer_mode", m_developerMode, "1 in developer mode (SSH, ADB, setup)");
		f.field("ssh", m_ssh, "1 if SSH is on");
		f.field("adb", m_adb, "1 if ADB is on");
		f.field("system_monitor_shown", m_systemMonitorShown, "1 if the build has the system monitor");
		f.field("system_monitor", m_systemMonitor, "1 if the system monitor is on");
		f.field("storage_info", m_storageInfo, "Storage used");
		f.field("cache_size", m_cacheSize, "Size of the cache");
	}

	void DeveloperSettingsPresenter::onActivate()
	{
		m_debugLevel.set(static_cast<int32_t>(Log::GetDebugLevel()));
		m_advancedSettings.set(StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS) ? 1 : 0);
		m_secondUsbChannel.set(StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL) ? 1 : 0);
		m_systemLogging.set(StorageHelper::getData(ID_ENABLE_UI_LOGGING) ? 1 : 0);
#if DEBUG_BORDERS
		m_debugBorders.set(Themes::isdebugBorderVisible(lv_screen_active()) ? 1 : 0);
#endif
#if DEVELOPER_MODE
		m_ssh.set(SystemHelper::isServiceEnabled(SystemHelper::Services::SSH) ? 1 : 0);
		m_adb.set(SystemHelper::isServiceEnabled(SystemHelper::Services::ADB) ? 1 : 0);
#endif
		m_systemMonitor.set(StorageHelper::getData(ID_SYSTEM_MONITOR_ENABLED) ? 1 : 0);
		showStorage();
	}

	void DeveloperSettingsPresenter::toggle(std::string name,
											Xml::IntSubject& shown,
											std::function<void(bool on)> apply)
	{
		action(std::move(name),
			   [&shown, apply = std::move(apply)](const Xml::ActionArgs& args)
			   {
				   const bool on = args.value().value_or(0.0f) != 0.0f;
				   shown.set(on ? 1 : 0);
				   apply(on);
			   });
	}

	void DeveloperSettingsPresenter::showStorage()
	{
		ZoneScoped;
		std::uintmax_t total = 0;
		std::uintmax_t used = 0;
		std::uintmax_t cacheSize = 0;
		size_t cacheFiles = 0;
#if USE_FIXED_TEST_STRINGS
		total = 16uz * 1024uz * 1024uz * 1024uz; // 16 GB
		used = 256uz * 1024uz * 1024uz;			 // 256 MB
		cacheSize = 10uz * 1024uz;				 // 10 KB
		cacheFiles = 100u;
#else
		const auto space = std::filesystem::space("/");
		total = space.capacity;
		used = space.capacity - space.available;
		if (std::filesystem::exists(NVS_FOLDER))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(NVS_FOLDER))
			{
				if (entry.is_regular_file())
				{
					cacheSize += entry.file_size();
					cacheFiles++;
				}
			}
		}
#endif
		m_storageInfo.set(_("settings.storage_info_format",
							Units::formatBytes(used),
							Units::formatBytes(total),
							total == 0 ? 0 : used * 100 / total));
		m_cacheSize.set(_("settings.duetscreen_cache_size_format", Units::formatBytes(cacheSize), cacheFiles));
	}

	void DeveloperSettingsPresenter::startHardwareTest()
	{
		ZoneScoped;
		if (m_hardwareTest == nullptr)
		{
			m_hardwareTest = std::make_unique<HardwareTest>();
		}
		m_hardwareTest->show(true);

		// Skip the serial number entry screen since the end user won't know this
		auto presenter = m_hardwareTest->getPresenter();
		presenter->m_serialNumber = "unknown";
		presenter->createLogFile();
		presenter->nextTest();
	}
} // namespace UI

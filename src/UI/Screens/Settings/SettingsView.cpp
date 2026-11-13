#include "SettingsView.h"
#include "BuildDate.h"
#include "Comm/Usb.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Job.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "i18n/i18n.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include "utils/UnitSystem.h"
#include "utils/UpgradeHelper.h"
#include "version.h"
#include <ranges>

namespace UI
{
	SettingsTab::SettingsTab(const std::string& name, LvObj& parent)
		: LvContainer(name, parent, layout_t(0, 0, 100, 100))
		, m_colDsc({LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST})
	{
		ZoneScoped;
		addStyle(Themes::getLvglStyles().card);

		m_rowDsc.at(1) = LV_GRID_TEMPLATE_LAST;
		setGridDsc(m_colDsc, m_rowDsc);
	}

	void SettingsTab::createHeader(std::string_view text)
	{
		ZoneScoped;
		if (m_rowCount >= m_maxRowCount)
		{
			LOG_FATAL_THROW("Maximum row count exceeded");
		}

		m_rowDsc.at(m_rowCount) = LV_GRID_CONTENT;
		auto lv_label = std::make_unique<LvLabel>(fmt::format("row_label_{:d}", m_rowCount), getRoot());
		lv_label->setText(text);
		lv_label->addStyle(Themes::getLvglStyles().text_emphasis);
		m_labels.at(m_rowCount) = std::move(lv_label);

		setGridCell(*m_labels.at(m_rowCount),
					LV_GRID_ALIGN_START,
					0,
					1,
					LV_GRID_ALIGN_CENTER,
					static_cast<int32_t>(m_rowCount),
					1);

		m_rowCount++;
		m_rowDsc.at(m_rowCount) = LV_GRID_TEMPLATE_LAST;

		updateLayout();
	}

	void SettingsTab::createRow(std::string_view label, LvObj& obj)
	{
		ZoneScoped;
		if (m_rowCount >= m_maxRowCount)
		{
			LOG_FATAL_THROW("Maximum row count exceeded");
		}

		m_rowDsc.at(m_rowCount) = LV_GRID_CONTENT;
		auto lv_label = std::make_unique<LvLabel>(fmt::format("row_label_{:d}", m_rowCount), getRoot());
		lv_label->setText(label);
		m_labels.at(m_rowCount) = std::move(lv_label);

		setGridCell(*m_labels.at(m_rowCount),
					LV_GRID_ALIGN_END,
					0,
					1,
					LV_GRID_ALIGN_CENTER,
					static_cast<int32_t>(m_rowCount),
					1);
		setGridCell(obj, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, static_cast<int32_t>(m_rowCount), 1);

		m_rowCount++;
		m_rowDsc.at(m_rowCount) = LV_GRID_TEMPLATE_LAST;

		updateLayout();
	}

	void SettingsTab::createSpanRow(LvObj& obj)
	{
		ZoneScoped;
		if (m_rowCount >= m_maxRowCount)
		{
			LOG_FATAL_THROW("Maximum row count exceeded");
		}

		m_rowDsc.at(m_rowCount) = LV_GRID_CONTENT;
		setGridCell(obj, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, static_cast<int32_t>(m_rowCount), 1);

		m_rowCount++;
		m_rowDsc.at(m_rowCount) = LV_GRID_TEMPLATE_LAST;

		updateLayout();
	}

	void SettingsTab::setRowVisibility(LvObj& obj, bool show)
	{
		ZoneScoped;
		UI_LOCK();
		int32_t row = lv_obj_get_style_grid_cell_row_pos(obj.getRootPtr(), LV_PART_MAIN);
		if (row < 0 || row >= static_cast<int32_t>(m_rowCount))
		{
			LOG_ERROR("Invalid obj");
			return;
		}

		if (auto& label = m_labels.at(row))
		{
			label->setVisible(show);
		}
		obj.setVisible(show);
	}

	DeveloperSettings::DeveloperSettings(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		/* Debug level */
		createRow(_("settings.debug_level"), m_debugLevel);
		m_debugLevel.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		for (const auto& level : Log::DebugLevelStrings)
		{
			m_debugLevel.addOption(_(level));
		}
		m_debugLevel.setSelectedCallback([this](uint32_t index, std::string_view /* option */)
										 { Log::SetDebugLevel(static_cast<Log::DebugLevel>(index)); });

		/* Advanced settings */
		createRow(_("settings.enable_advanced_settings"), m_enableAdvancedSettings);
		m_enableAdvancedSettings.setCheckedCallback([](bool checked)
													{ StorageHelper::setData(ID_ENABLE_ADVANCED_SETTINGS, checked); });

		/* USB second channel */
		createRow(_("settings.enable_second_usb_channel"), m_enableSecondUsbChannel);
		m_enableSecondUsbChannel.setCheckedCallback(
			[](bool checked)
			{
				StorageHelper::setData(ID_ENABLE_SECOND_USB_CHANNEL, checked);
				if (checked && Comm::DUET.GetCommunicationType() == Comm::CommunicationType::usb &&
					Comm::DUET.IsConnected())
				{
					Comm::DUET.SendGcode("M575 P1 S0\n", true);
				}
			});

#if DEBUG_BORDERS
		/* Debug borders */
		createRow(_("settings.debug_borders"), m_debugBorders);
		m_debugBorders.setCheckedCallback(
			[](bool checked)
			{
				StorageHelper::setData(ID_DEBUG_BORDERS, checked);
				Themes::showDebugBorders(lv_screen_active(), checked);
			});
#endif

#if DEVELOPER_MODE
		/* Enable SSH */
		createRow(_("settings.enable_ssh"), m_enableSSH);
		m_enableSSH.setCheckedCallback(
			[](bool checked)
			{
				if (checked)
				{
					SystemHelper::enableService(SystemHelper::Services::SSH);
				}
				else
				{
					SystemHelper::disableService(SystemHelper::Services::SSH);
				}
			});

		/* Enable ADB */
		createRow(_("settings.enable_adb"), m_enableADB);
		m_enableADB.setCheckedCallback(
			[](bool checked)
			{
				if (checked)
				{
					SystemHelper::enableService(SystemHelper::Services::ADB);
				}
				else
				{
					SystemHelper::disableService(SystemHelper::Services::ADB);
				}
			});
#endif

#if LV_USE_SYSMON
		createRow(_("settings.enable_system_monitor"), m_enableSystemMonitor);
		m_enableSystemMonitor.setCheckedCallback(
			[](bool checked)
			{
				StorageHelper::setData(ID_SYSTEM_MONITOR_ENABLED, checked);

#  if LV_USE_PERF_MONITOR
				if (checked)
					lv_sysmon_show_performance(NULL);
				else
					lv_sysmon_hide_performance(NULL);
#  endif
#  if LV_USE_MEM_MONITOR
				if (checked)
					lv_sysmon_show_memory(NULL);
				else
					lv_sysmon_hide_memory(NULL);
#  endif
			});
#endif

		/* System Logging */
		createRow(_("settings.system_logging"), m_systemLogging);
		m_systemLogging.setCheckedCallback(
			[](bool checked)
			{
				StorageHelper::setData(ID_ENABLE_UI_LOGGING, checked);
				Log::EnableUiLogging(checked);
			});

		createSpanRow(m_controls);
		m_controls.setSize(LV_PCT(50), LV_SIZE_CONTENT);
		m_controls.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_controls.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		for (auto& btn : {
				 &m_restart,
				 &m_eraseAndRestart,
				 &m_reboot,
				 &m_startHardwareTest,
#if DEVELOPER_MODE
				 &m_runBuildrootSetup,
#endif
				 &m_sendConfigJson,
				 &m_clearCache,
				 &m_upgradeFromGithub,
			 })
		{
			btn->setSize(150, 100);
		}

		m_restart.setText(_("settings.restart"));
		m_eraseAndRestart.setText(_("settings.erase_and_restart"));
		m_reboot.setText(_("settings.reboot"));
		m_startHardwareTest.setText(_("settings.start_hardware_test"));
		m_sendConfigJson.setText(_("settings.send_config_to_duet"));
		m_sendConfigJson.addClickedCallback(
			[this](lv_event_t*)
			{
				std::string config = StorageHelper::dump();
				Comm::DUET.UploadFile(fmt::format("{:s}/duetscreen.json", OM::Directories::GetSystemDirectory()),
									  config);
			});

		m_upgradeFromGithub.setText(_("settings.upgrade_from_github"));
		m_upgradeFromGithub.addClickedCallback([this](lv_event_t*) { getPresenter()->upgradeFromGithubLatest(); });

		m_restart.addClickedCallback([](lv_event_t*) { Restart(); });
		m_eraseAndRestart.addClickedCallback([](lv_event_t*) { EraseAndRestart(); });
		m_reboot.addClickedCallback([](lv_event_t*) { Reboot(); });
		m_startHardwareTest.addClickedCallback([this](lv_event_t*) { getPresenter()->startHardwareTest(); });

#if DEVELOPER_MODE
		m_runBuildrootSetup.setText(_("settings.run_buildroot_setup"));
		m_runBuildrootSetup.addClickedCallback([](lv_event_t*)
											   { SystemHelper::restartService(SystemHelper::Services::SETUP); });
#endif
		m_clearCache.setText(_("settings.clear_duetscreen_cache"));
		m_clearCache.addClickedCallback(
			[this](lv_event_t*)
			{
				if (std::filesystem::exists(NVS_FOLDER))
					std::filesystem::remove_all(NVS_FOLDER);
				FILEINFO_CACHE->ClearCache();
				getPresenter()->refreshCacheInfo();
			});

		/* Storage */
		createRow(_("settings.storage_info"), m_storageInfo);
		createRow(_("settings.duetscreen_cache_size"), m_CacheSize);

		/* Hardware test */
		m_hardwareTest.hide();
	}

	void DeveloperSettings::setStorageInfo(std::uintmax_t totalStorage, std::uintmax_t usedStorage)
	{
		ZoneScoped;
#if USE_FIXED_TEST_STRINGS
		totalStorage = 16uz * 1024uz * 1024uz * 1024uz; // 16 GB
		// totalStorage *= 1024u;					// 16 GB
		usedStorage = 256uz * 1024uz * 1024uz; // 256 MB
#endif
		m_storageInfo.setText(_("settings.storage_info_format",
								Units::formatBytes(usedStorage),
								Units::formatBytes(totalStorage),
								usedStorage * 100 / totalStorage));
	}

	void DeveloperSettings::setDuetScreenCacheSize(std::uintmax_t cacheSize, size_t fileCount)
	{
		ZoneScoped;
#if USE_FIXED_TEST_STRINGS
		cacheSize = 10uz * 1024uz; // 10 KB
		fileCount = 100u;
#endif
		m_CacheSize.setText(_("settings.duetscreen_cache_size_format", Units::formatBytes(cacheSize), fileCount));
	}

	void DeveloperSettings::onInit() {}

	void DeveloperSettings::onShow()
	{
		ZoneScoped;
		m_debugLevel.setSelected(static_cast<uint32_t>(Log::GetDebugLevel()));
		m_enableAdvancedSettings.setChecked(StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS));
		m_enableSecondUsbChannel.setChecked(StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL));
#if DEBUG_BORDERS
		m_debugBorders.setChecked(Themes::isdebugBorderVisible(lv_screen_active()));
#endif
#if DEVELOPER_MODE
		m_enableSSH.setChecked(SystemHelper::isServiceEnabled(SystemHelper::Services::SSH));
		m_enableADB.setChecked(SystemHelper::isServiceEnabled(SystemHelper::Services::ADB));
#endif
#if LV_USE_SYSMON
		m_enableSystemMonitor.setChecked(StorageHelper::getData(ID_SYSTEM_MONITOR_ENABLED));
#endif
		m_systemLogging.setChecked(StorageHelper::getData(ID_ENABLE_UI_LOGGING));
	}
} // namespace UI

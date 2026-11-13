#pragma once

#include "Hardware/Duet.h"
#include "SettingsPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Components/Input/DropdownMenu.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/Input/TextBox.h"
#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvSwitch.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Components/Theme/ThemePreview.h"
#include "UI/Core/View.h"
#include "UI/Widgets/HardwareTest/HardwareTest.h"
#include "UI/Widgets/Network/WifiSelector.h"
#include "i18n/i18n.h"

namespace UI
{
	using LvSettingsToggle = UI_SETTINGS_TOGGLE_WIDGET;

	class SettingsTab : public LvContainer
	{
	  public:
		SettingsTab(const std::string& name, LvObj& parent);

	  protected:
		void createHeader(std::string_view text);
		void createRow(std::string_view label, LvObj& obj);
		void createSpanRow(LvObj& obj);
		void setRowVisibility(LvObj& obj, bool show);

		static constexpr int32_t m_maxRowCount = 15;
		size_t m_rowCount = 0;

		std::array<std::unique_ptr<LvLabel>, m_maxRowCount> m_labels;
		std::array<int32_t, 3> m_colDsc;
		std::array<int32_t, m_maxRowCount + 1> m_rowDsc;
	};

	class ConnectionSettings : public View<ConnectionSettingsPresenter, SettingsTab>
	{
	  public:
		ConnectionSettings(const std::string& name, LvObj& parent);

		void setKeyboard(LvKeyboard* keyboard);

	  private:
		void showConnectionMethodSettings(const Comm::CommunicationType method);

		void onInit() override;
		void onShow() override;

		DropdownMenu m_connectionMethod{"connection_method", getRoot()};
		DropdownMenu m_usbMode{"usb_mode", getRoot()};
		Slider m_pollInterval{"poll_interval", getRoot()};

		/* USB Settings */
		// None

		/* Wifi Settings */
		TextBox m_duetIpAddress{"duet_ip_address", getRoot()};
		TextBox m_duetPassword{"duet_password", getRoot()};

		/* UART Settings */
		// None

		/* Network */
		WifiSelector m_wifiSelector{"wifi_selector", getRoot()};

		LvKeyboard* m_keyboard;
	};

	class DisplaySettings : public View<DisplaySettingsPresenter, SettingsTab>
	{
	  public:
		DisplaySettings(const std::string& name, LvObj& parent);

	  private:
		void updateThemePreview();

		void onInit() override;
		void onShow() override;

		DropdownMenu m_theme{"theme", getRoot()};
		DropdownMenu m_font{"font", getRoot()};
		DropdownMenu m_icons{"icons", getRoot()};
		ThemePreview m_themePreview{"theme_preview", getRoot()};
		LvSettingsToggle m_enableAnimations{"enable_animations", getRoot()};
		DropdownMenu m_screenRotation{"screen_rotation", getRoot()};
	};

	class DeveloperSettings : public View<DeveloperSettingsPresenter, SettingsTab>
	{
	  public:
		DeveloperSettings(const std::string& name, LvObj& parent);

		HardwareTest& getHardwareTest() { return m_hardwareTest; }

		void setStorageInfo(std::uintmax_t totalStorage, std::uintmax_t usedStorage);
		void setDuetScreenCacheSize(std::uintmax_t cacheSize, size_t fileCount);

	  private:
		static void onDebugLevelEvent(lv_event_t* e);
#if DEBUG_BORDERS
		static void onDebugBordersEvent(lv_event_t* e);
#endif
		static void onEnableSSHEvent(lv_event_t* e);
		static void onRestartEvent(lv_event_t* e);
		static void onEraseAndRestartEvent(lv_event_t* e);
		static void onRebootEvent(lv_event_t* e);

		void onInit() override;
		void onShow() override;

		DropdownMenu m_debugLevel{"debug_level", getRoot()};
		LvSettingsToggle m_enableAdvancedSettings{"enable_advanced_settings", getRoot()};
		LvSettingsToggle m_enableSecondUsbChannel{"enable_second_usb_channel", getRoot()};
#if DEBUG_BORDERS
		LvSettingsToggle m_debugBorders{"debug_borders", getRoot()};
#endif
#if DEVELOPER_MODE
		LvSettingsToggle m_enableSSH{"enable_ssh", getRoot()};
		LvSettingsToggle m_enableADB{"enable_adb", getRoot()};
#endif
#if LV_USE_SYSMON
		LvSettingsToggle m_enableSystemMonitor{"enable_system_monitor", getRoot()};
#endif
		LvSettingsToggle m_systemLogging{"system_logging", getRoot()};

		/* Controls */
		LvContainer m_controls{"controls", getRoot()};
		Button m_restart{"restart", m_controls};
		Button m_eraseAndRestart{"erase_and_restart", m_controls};
		Button m_reboot{"reboot", m_controls};
		Button m_startHardwareTest{"start_hardware_test", m_controls};
#if DEVELOPER_MODE
		Button m_runBuildrootSetup{"run_buildroot_setup", m_controls};
#endif
		Button m_clearCache{"clear_duetscreen_cache", m_controls};
		Button m_upgradeFromGithub{"upgrade_from_github", m_controls};
		Button m_sendConfigJson{"send_config_json", m_controls};

		/* Storage */
		LvLabel m_storageInfo{"storage_info", getRoot()};
		LvLabel m_CacheSize{"duetscreen_cache_size", getRoot()};

		HardwareTest m_hardwareTest;
	};

} // namespace UI
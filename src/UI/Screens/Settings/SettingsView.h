#pragma once

#include "Hardware/Duet.h"
#include "SettingsPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/DropdownMenu.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/Input/TextBox.h"
#include "UI/Components/Modal/Modal.h"
#include "UI/Components/Theme/ThemePreview.h"
#include "UI/Core/View.h"
#include "UI/Widgets/HardwareTest/HardwareTest.h"

namespace UI
{
	class SettingsView;

	class SettingsSubView : public Card
	{
	  public:
		SettingsSubView(const std::string& name, LvObj& parent, SettingsView& mainSettingsView);

		SettingsView& getMainSettingsView() const { return m_mainSettingsView; }
		std::shared_ptr<SettingsPresenter> getMainSettingsPresenter() const;

	  protected:
		static void onTextAreaEvent(lv_event_t* e);
		SettingsView& m_mainSettingsView;
	};

	class DuetSettingsView : public SettingsSubView
	{
	  public:
		DuetSettingsView(LvObj& parent, SettingsView& mainSettingsView);

		class UsbSettings : public LvContainer
		{
		  public:
			UsbSettings(DuetSettingsView& parent);

		  private:
		};

		class WifiSettings : public LvContainer
		{
		  public:
			WifiSettings(DuetSettingsView& parent);

		  private:
			TextBox m_hostname;
			TextBox m_password;
		};

		class UartSettings : public LvContainer
		{
		  public:
			UartSettings(DuetSettingsView& parent);

		  private:
		};

	  private:
		static void onConnectionMethodEvent(lv_event_t* e);

		void showConnectionMethodSettings(const Comm::CommunicationType method);
		virtual void onShow() override;

		DropdownMenu m_connectionMethod;
		UsbSettings m_usbSettings;
		WifiSettings m_wifiSettings;
		UartSettings m_uartSettings;
		Slider m_pollInterval;
		Slider m_infoTimeout;
	};

	class ScreenSettingsView : public SettingsSubView
	{
	  public:
		ScreenSettingsView(LvObj& parent, SettingsView& mainSettingsView);

	  private:
		void onShow() override;

		lv_obj_t* m_firmwareVersion;
		lv_obj_t* m_buildTime;
		DropdownMenu m_language;
		DropdownMenu m_usbMode;
		Slider m_brightness;
		Slider m_screensaverTimeout;
		lv_obj_t* m_systemLogging;
		lv_obj_t* m_displayConnectedMessage;
	};

	class ThemeSettingsView : public SettingsSubView
	{
	  public:
		ThemeSettingsView(LvObj& parent, SettingsView& mainSettingsView);

	  private:
		DropdownMenu m_theme;
		ThemePreview m_themePreview;
	};

	class NetworkSettingsView : public View<NetworkSettingsPresenter, SettingsSubView>
	{
	  public:
		NetworkSettingsView(LvObj& parent, SettingsView& mainSettingsView);

		void setIpAddress(const std::string& ipAddress);
		void setEnabled(bool enabled);
		void setNetworkCount(size_t count);
		void setNetworkDetails(size_t index, const std::string& ssid, int32_t signalLevel, bool known, bool connected);

	  private:
		static void onNetworkSelectionEvent(lv_event_t* e);
		static void onRefreshEvent(lv_event_t* e);
		void onPasswordCloseEvent();
		void onPasswordConfirmEvent();

		void onShow() override;
		void onHide() override;

		LvContainer m_topBar;
		LvLabel m_ipAddress;
		Button m_refresh;

		lv_obj_t* m_networkList;
		Modal<MessageBox> m_passwordWindow;
		TextBox m_passwordInput;
	};

	class DeveloperSettingsView : public SettingsSubView
	{
	  public:
		DeveloperSettingsView(LvObj& parent, SettingsView& mainSettingsView);

	  private:
		static void onDebugLevelEvent(lv_event_t* e);
#if DEBUG_BORDERS
		static void onDebugBordersEvent(lv_event_t* e);
#endif
		static void onEnableSSHEvent(lv_event_t* e);
		static void onRestartEvent(lv_event_t* e);
		static void onEraseAndRestartEvent(lv_event_t* e);
		static void onRebootEvent(lv_event_t* e);

		lv_obj_t* m_debugLevelCont;
		lv_obj_t* m_debugLevelLabel;
		lv_obj_t* m_debugLevel;

#if DEBUG_BORDERS
		lv_obj_t* m_debugBorders;
#endif
		lv_obj_t* m_enableSSH;

		Button m_restart;
		Button m_eraseAndRestart;
		Button m_reboot;
		Button m_startHardwareTest;
	};

	/**
	 * @brief View to configure the screen settings
	 *
	 * This class provides a user interface for configuring various screen settings.
	 *
	 * @note The following subviews are defined:
	 * @note - DuetSettingsView
	 * @note - DisplaySettingsView
	 * @note - LanguageSettingsView
	 * @note - NetworkSettingsView
	 * @note - DeveloperSettingsView.
	 * @note - ThemeSettingsView.
	 *
	 * @param parent The parent LVGL object.
	 */
	class SettingsView : public View<SettingsPresenter>
	{
		friend class SettingsSubView;
		friend class DuetSettingsView;
		friend class ScreenSettingsView;
		friend class ThemeSettingsView;

	  public:
		SettingsView(LvObj& parent);

		void showKeyboard(bool show,
						  lv_keyboard_mode_t mode = LV_KEYBOARD_MODE_TEXT_LOWER,
						  lv_obj_t* textArea = nullptr);
		void setKeyboardTextArea(lv_obj_t* textArea);

		DuetSettingsView& getDuetSettingsView() { return m_duetSettingsView; }
		NetworkSettingsView& getNetworkSettingsView() { return m_networkSettingsView; }
		DeveloperSettingsView& getDeveloperSettingsView() { return m_developerSettingsView; }

		HardwareTest& getHardwareTest() { return m_hardwareTest; }

		virtual bool back() override;

	  protected:
		static void onWindowSelectEvent(lv_event_t* e);

		lv_obj_t* getKeyboard() const { return m_keyboard; }

		virtual void onShow() override;
		virtual void onHide() override;

		int32_t m_layoutColDsc[3] = {LV_GRID_CONTENT, LV_GRID_FR(4), LV_GRID_TEMPLATE_LAST};
		int32_t m_layoutRowDsc[3] = {LV_GRID_FR(2), 0, LV_GRID_TEMPLATE_LAST};

		lv_obj_t* m_settingsList;
		LvContainer m_subWindow;
		LvKeyboard m_keyboard;

		lv_obj_t* m_screenHeader;
		lv_obj_t* m_screenSettings;
		lv_obj_t* m_themeSettings;
		lv_obj_t* m_connectivityHeader;
		lv_obj_t* m_duetSettings;
		lv_obj_t* m_networkSettings;
		lv_obj_t* m_devHeader;
		lv_obj_t* m_developerSettings;

		DuetSettingsView m_duetSettingsView;
		ScreenSettingsView m_deviceSettingsView;
		ThemeSettingsView m_themeSettingsView;
		NetworkSettingsView m_networkSettingsView;
		DeveloperSettingsView m_developerSettingsView;

		HardwareTest m_hardwareTest;

		SettingsSubView* m_currentSubView;
	};
} // namespace UI
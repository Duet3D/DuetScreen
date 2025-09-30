#include "SettingsView.h"
#include "BuildDate.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include "version.h"

namespace UI
{
	SettingsView::SettingsView(LvObj& parent)
		: View("settings_view", parent, layout_t(0, 0, 100, 100))
		, m_settingsList(lv_list_create(getRoot()))
		, m_subWindow("sub_window", getRoot())
		, m_keyboard("keyboard", getRoot())
		, m_screenHeader(lv_list_add_text(m_settingsList, _("settings_screen_header")))
		, m_screenSettings(lv_list_add_button(m_settingsList, NULL, _("settings_screen")))
		, m_themeSettings(lv_list_add_button(m_settingsList, NULL, _("settings_theme")))
		, m_connectivityHeader(lv_list_add_text(m_settingsList, _("settings_connectivity_header")))
		, m_duetSettings(lv_list_add_button(m_settingsList, NULL, _("settings_duet")))
		, m_networkSettings(lv_list_add_button(m_settingsList, LV_SYMBOL_WIFI, _("settings_network")))
		, m_devHeader(lv_list_add_text(m_settingsList, _("settings_dev_header")))
		, m_developerSettings(lv_list_add_button(m_settingsList, LV_SYMBOL_SETTINGS, _("settings_developer")))
		, m_duetSettingsView(m_subWindow, *this)
		, m_deviceSettingsView(m_subWindow, *this)
		, m_themeSettingsView(m_subWindow, *this)
		, m_networkSettingsView(m_subWindow, *this)
		, m_developerSettingsView(m_subWindow, *this)
		, m_currentSubView(&m_deviceSettingsView)
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);

		// Layout
		lv_obj_set_layout(getRoot(), LV_LAYOUT_GRID);
		lv_obj_set_grid_dsc_array(getRoot(), m_layoutColDsc, m_layoutRowDsc);
		lv_obj_set_grid_cell(m_settingsList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_subWindow, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_keyboard, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 1, 1);

		// List
		lv_obj_set_user_data(m_duetSettings, &m_duetSettingsView);
		lv_obj_set_user_data(m_screenSettings, &m_deviceSettingsView);
		lv_obj_set_user_data(m_themeSettings, &m_themeSettingsView);
		lv_obj_set_user_data(m_networkSettings, &m_networkSettingsView);
		lv_obj_set_user_data(m_developerSettings, &m_developerSettingsView);

		lv_obj_add_event_cb(m_duetSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_screenSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_themeSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_networkSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_developerSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);

		m_hardwareTest.hide();

		// Sub window
		lv_obj_set_style_pad_all(m_subWindow, 0, LV_PART_MAIN);
	}

	void SettingsView::onWindowSelectEvent(lv_event_t* e)
	{
		UI_LOCK();
		SettingsView* view = (SettingsView*)lv_event_get_user_data(e);
		SettingsSubView* subView = (SettingsSubView*)lv_obj_get_user_data(lv_event_get_target_obj(e));
		SettingsSubView* currentSubView = view->m_currentSubView;

		if (currentSubView == subView)
		{
			return;
		}

		if (currentSubView != nullptr)
		{
			currentSubView->hide();
		}

		view->showKeyboard(false);
		subView->show(true);
		view->m_currentSubView = subView;
	}

	void SettingsView::showKeyboard(bool show, lv_keyboard_mode_t mode, lv_obj_t* textArea)
	{
		UI_LOCK();
		if (show)
		{
			m_layoutRowDsc[1] = LV_GRID_FR(1);
			lv_keyboard_set_mode(m_keyboard, mode);
			lv_obj_remove_flag(m_keyboard, LV_OBJ_FLAG_HIDDEN);
			setKeyboardTextArea(textArea);
		}
		else
		{
			m_layoutRowDsc[1] = 0;
			setKeyboardTextArea(NULL);
			lv_obj_add_flag(m_keyboard, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void SettingsView::setKeyboardTextArea(lv_obj_t* textArea)
	{
		UI_LOCK();
		lv_keyboard_set_textarea(m_keyboard, textArea);
	}

	bool SettingsView::back()
	{
		UI_LOCK();
		if (!lv_obj_has_flag(m_keyboard, LV_OBJ_FLAG_HIDDEN))
		{
			showKeyboard(false);
			return true;
		}

		return false;
	}

	void SettingsView::onHide()
	{
		m_currentSubView->hide();
	}

	void SettingsView::onShow()
	{
		showKeyboard(false);
		m_currentSubView->show(true);
	}

	SettingsSubView::SettingsSubView(const std::string& name, LvObj& parent, SettingsView& mainSettingsView)
		: Card(name, parent, layout_t(0, 0, 100, 100))
		, m_mainSettingsView(mainSettingsView)
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
	}

	std::shared_ptr<SettingsPresenter> SettingsSubView::getMainSettingsPresenter() const
	{
		return m_mainSettingsView.getPresenter();
	}

	void SettingsSubView::onTextAreaEvent(lv_event_t* e)
	{
		UI_LOCK();
		lv_event_code_t code = lv_event_get_code(e);
		if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
		{
			return;
		}

		lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
		SettingsSubView* view = (SettingsSubView*)lv_event_get_user_data(e);
		const char* acceptedChars = lv_textarea_get_accepted_chars(ta);
		lv_keyboard_mode_t mode = (!acceptedChars || strpbrk(acceptedChars, "abcdefghijklmnopqrstuvwxyz") != nullptr)
									  ? LV_KEYBOARD_MODE_TEXT_LOWER
									  : LV_KEYBOARD_MODE_NUMBER;
		if (code == LV_EVENT_FOCUSED)
		{
			view->getMainSettingsView().showKeyboard(true, mode, ta);
		}

		if (code == LV_EVENT_DEFOCUSED)
		{
			view->getMainSettingsView().showKeyboard(false);
		}
	}

	DuetSettingsView::DuetSettingsView(LvObj& parent, SettingsView& mainSettingsView)
		: SettingsSubView("duet_settings_view", parent, mainSettingsView)
		, m_connectionMethod("duet_settings_connection_method", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_usbSettings(*this)
		, m_wifiSettings(*this)
		, m_uartSettings(*this)
		, m_pollInterval("duet_settings_poll_interval", getRoot())
		, m_infoTimeout("duet_settings_info_timeout", getRoot())
	{
		UI_LOCK();

		// Connection Method
		std::vector<std::string> options;
		for (const auto& method : Comm::duetCommunicationTypeNames)
		{
			options.push_back(_(method.data()));
		}
		m_connectionMethod.setLabel(_("settings_duet_connection_method"));
		m_connectionMethod.setOptions(options);
		m_connectionMethod.addEventCallback(onConnectionMethodEvent, LV_EVENT_VALUE_CHANGED, this);
		m_connectionMethod.setSelected((uint32_t)Comm::DUET.GetCommunicationType());

		// Poll Interval
		m_pollInterval.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_pollInterval.setLabel(_("settings_duet_poll_interval"));
		m_pollInterval.setOutOfRangeMode(Slider::OutOfRange::UPPER);
		m_pollInterval.setRange(MIN_PRINTER_POLL_INTERVAL.count(), 2000);
		m_pollInterval.setValue(Comm::DUET.GetPollInterval().count());
		m_pollInterval.setValueChangedCallback([](int32_t value)
											   { Comm::DUET.SetPollInterval(std::chrono::milliseconds(value)); });
		m_pollInterval.setKeyboard(getMainSettingsView().getKeyboard());
		m_pollInterval.setFocusedCallback(
			[this](bool focused)
			{ getMainSettingsView().showKeyboard(focused, LV_KEYBOARD_MODE_NUMBER, m_pollInterval.getInput()); });

		// Info Timeout
		m_infoTimeout.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_infoTimeout.setLabel(_("settings_duet_info_timeout"));
		m_infoTimeout.setOutOfRangeMode(Slider::OutOfRange::UPPER);
		m_infoTimeout.setRange(0, 5000);
		m_infoTimeout.setValue(StorageHelper::getData(ID_INFO_TIMEOUT, DEFAULT_POPUP_TIMEOUT));
		m_infoTimeout.setValueChangedCallback([](int32_t value)
											  { StorageHelper::setData(ID_INFO_TIMEOUT, (uint32_t)value); });
		m_infoTimeout.setKeyboard(getMainSettingsView().getKeyboard());
		m_infoTimeout.setFocusedCallback(
			[this](bool focused)
			{ getMainSettingsView().showKeyboard(focused, LV_KEYBOARD_MODE_NUMBER, m_infoTimeout.getInput()); });
	}

	DuetSettingsView::UsbSettings::UsbSettings(DuetSettingsView& parent)
		: LvContainer("duet_settings_usb", parent, layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
	{
		UI_LOCK();
	}

	DuetSettingsView::WifiSettings::WifiSettings(DuetSettingsView& parent)
		: LvContainer("duet_settings_wifi", parent, layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_hostname("duet_settings_hostname", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_password("duet_settings_password", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_hostname.setOneLine(true);
		m_hostname.setLabel(_("settings_duet_hostname"));
		m_hostname.setPlaceholderText(_("settings_duet_hostname_prompt"));
		m_hostname.setAcceptedChars("0123456789.");
		m_hostname.setText(Comm::DUET.GetHostname());
		m_hostname.getTextArea().addEventCallback(onTextAreaEvent, LV_EVENT_ALL, &parent);
		m_hostname.addConfirmEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
				Comm::DUET.SetHostname(lv_textarea_get_text(ta));
			},
			nullptr);

		m_password.setOneLine(true);
		m_password.setLabel(_("settings_duet_password"));
		m_password.setPlaceholderText(_("settings_duet_password_prompt"));
		m_password.setPasswordMode(true);
		m_password.setText(Comm::DUET.GetPassword());
		m_password.getTextArea().addEventCallback(onTextAreaEvent, LV_EVENT_ALL, &parent);
		m_password.addConfirmEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
				Comm::DUET.SetPassword(lv_textarea_get_text(ta));
			},
			nullptr);
	}

	DuetSettingsView::UartSettings::UartSettings(DuetSettingsView& parent)
		: LvContainer("duet_settings_uart", parent, layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
	{
		UI_LOCK();
	}

	void DuetSettingsView::onConnectionMethodEvent(lv_event_t* e)
	{
		UI_LOCK();
		DuetSettingsView* view = (DuetSettingsView*)lv_event_get_user_data(e);
		auto comm_type = (Comm::CommunicationType)(view->m_connectionMethod.getSelected());
		Comm::DUET.SetCommunicationType(comm_type);
		switch (comm_type)
		{
		case Comm::CommunicationType::uart:
		case Comm::CommunicationType::usb:
			view->getMainSettingsPresenter()->setUsbMode(UsbMode::Host);
			break;
		case Comm::CommunicationType::network:
			view->getMainSettingsPresenter()->setUsbMode(UsbMode::InternalWiFi);
			break;
		default:
			break;
		}

		view->showConnectionMethodSettings(Comm::DUET.GetCommunicationType());
	}

	void DuetSettingsView::showConnectionMethodSettings(const Comm::CommunicationType method)
	{
		UI_LOCK();

		m_usbSettings.setVisible(method == Comm::CommunicationType::usb);
		m_wifiSettings.setVisible(method == Comm::CommunicationType::network);
		m_uartSettings.setVisible(method == Comm::CommunicationType::uart);
	}

	void DuetSettingsView::onShow()
	{
		UI_LOCK();
		m_pollInterval.setValue(Comm::DUET.GetPollInterval().count());
		m_infoTimeout.setValue(StorageHelper::getData(ID_INFO_TIMEOUT, DEFAULT_POPUP_TIMEOUT));
		showConnectionMethodSettings(Comm::DUET.GetCommunicationType());
	}

	ScreenSettingsView::ScreenSettingsView(LvObj& parent, SettingsView& mainSettingsView)
		: SettingsSubView("screen", parent, mainSettingsView)
		, m_firmwareVersion(lv_label_create(getRoot()))
		, m_buildTime(lv_label_create(getRoot()))
		, m_language("language", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_usbMode("usb_mode", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_brightness("brightness", getRoot())
		, m_screensaverTimeout("screensaver_timeout", getRoot())
		, m_systemLogging(lv_checkbox_create(getRoot()))
		, m_displayConnectedMessage(lv_checkbox_create(getRoot()))
	{
		UI_LOCK();

		lv_label_set_text(m_firmwareVersion, utils::format(_("settings_firmware_version"), FIRMWARE_VERSION).c_str());
		lv_label_set_text(m_buildTime, utils::format(_("settings_build_time"), BuildDateText, BuildTimeSuffix).c_str());

		m_language.setLabel(_("settings_language"));
		m_language.setOptions(_("settings_language_en"));

		m_usbMode.setLabel(_("settings_usb_mode"));
		m_usbMode.setOptions(
			{_("settings_usb_mode_host"), _("settings_usb_mode_device"), _("settings_usb_mode_internal_wifi")});
		m_usbMode.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
				ScreenSettingsView* view = (ScreenSettingsView*)lv_event_get_user_data(e);
				int32_t selected = view->m_usbMode.getSelected();
				view->getMainSettingsPresenter()->setUsbMode((UsbMode(selected)));
			},
			LV_EVENT_VALUE_CHANGED,
			this);
		m_usbMode.setSelected(StorageHelper::getData(ID_USB_MODE, 0));

		// Brightness
		m_brightness.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_brightness.setRange(0, 100);
		m_brightness.setLabel(_("settings_brightness"));
		m_brightness.setValue(DisplayHelper::getBrightness());
		m_brightness.setValueChangedCallback([](uint32_t value) { DisplayHelper::setBrightness(value); });
		m_brightness.setSendMode(Slider::SendMode::VALUE_CHANGED);
		m_brightness.setKeyboard(getMainSettingsView().getKeyboard());
		m_brightness.setFocusedCallback(
			[this](bool focused)
			{ getMainSettingsView().showKeyboard(focused, LV_KEYBOARD_MODE_NUMBER, m_brightness.getInput()); });

		// Screensaver Timeout
		m_screensaverTimeout.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_screensaverTimeout.setLabel(_("settings_screensaver_timeout"));
		m_screensaverTimeout.setRange(0, 5 * 60); // seconds
		m_screensaverTimeout.setValueChangedCallback([](uint32_t value)
													 { StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, value * 1000); });
		m_screensaverTimeout.setOutOfRangeMode(Slider::OutOfRange::UPPER);
		m_screensaverTimeout.setKeyboard(getMainSettingsView().getKeyboard());
		m_screensaverTimeout.setFocusedCallback(
			[this](bool focused)
			{ getMainSettingsView().showKeyboard(focused, LV_KEYBOARD_MODE_NUMBER, m_screensaverTimeout.getInput()); });

		// System Logging
		lv_checkbox_set_text(m_systemLogging, _("settings_system_logging"));
		lv_obj_set_state(m_systemLogging, LV_STATE_CHECKED, StorageHelper::getData(ID_ENABLE_UI_LOGGING, false));
		lv_obj_add_event_cb(
			m_systemLogging,
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* checkbox = (lv_obj_t*)lv_event_get_target(e);
				bool checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
				StorageHelper::setData(ID_ENABLE_UI_LOGGING, checked);
				Log::EnableUiLogging(checked);
			},
			LV_EVENT_VALUE_CHANGED,
			this);

		// Display Connected Message
		lv_checkbox_set_text(m_displayConnectedMessage, _("settings_display_connected_message"));
		lv_obj_set_state(
			m_displayConnectedMessage, LV_STATE_CHECKED, StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE, true));
		lv_obj_add_event_cb(
			m_displayConnectedMessage,
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* checkbox = (lv_obj_t*)lv_event_get_target(e);
				bool checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
				StorageHelper::setData(ID_DISPLAY_CONNECTED_MESSAGE, checked);
			},
			LV_EVENT_VALUE_CHANGED,
			this);
	}

	void ScreenSettingsView::onShow()
	{
		UI_LOCK();
		m_usbMode.setSelected(StorageHelper::getData(ID_USB_MODE, 0));
		m_brightness.setValue(DisplayHelper::getBrightness());
		m_screensaverTimeout.setValue(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT, DEFAULT_SCREEN_TIMEOUT) / 1000);
	}

	ThemeSettingsView::ThemeSettingsView(LvObj& parent, SettingsView& mainSettingsView)
		: SettingsSubView("screen", parent, mainSettingsView)
		, m_theme("theme", getRoot(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_themePreview("theme_demo", getRoot())
	{
		UI_LOCK();

		m_theme.setLabel(_("settings_theme"));
		for (auto& theme : Themes::getThemes())
		{
			m_theme.addOption(_(theme->getName().data()));
		}
		m_theme.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
				auto view = (ThemeSettingsView*)lv_event_get_user_data(e);
				int32_t selected = view->m_theme.getSelected();
				auto theme = Themes::getTheme(selected);
				if (theme == nullptr)
				{
					return;
				}
				theme->setThemeActive();
				view->m_themePreview.updateSwatches();
				StorageHelper::setData(ID_THEME, selected);
				// view->getMainSettingsPresenter()->setTheme(selected);
			},
			LV_EVENT_VALUE_CHANGED,
			this);
		m_theme.setSelected(StorageHelper::getData(ID_THEME, 0));
		m_themePreview.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	}

	NetworkSettingsView::NetworkSettingsView(LvObj& parent, SettingsView& mainSettingsView)
		: View("network_settings_view", parent, mainSettingsView)
		, m_topBar("top_bar", getRoot())
		, m_ipAddress("ip_address", m_topBar)
		, m_refresh("refresh", m_topBar, _("refresh"), layout_t{0, 0, 0, LV_SIZE_CONTENT})
		, m_networkList(lv_table_create(getRoot()))
		, m_passwordWindow("password_msgbox", getRoot(), layout_t{0, 0, 80, LV_SIZE_CONTENT})
		, m_passwordInput("password_input", m_passwordWindow.getBody(), layout_t(0, 0, 80, LV_SIZE_CONTENT))
	{
		UI_LOCK();

		lv_obj_set_flex_flow(getRoot(), LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(getRoot(), LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		lv_obj_set_style_pad_all(m_topBar, 2, 0);
		lv_obj_set_flex_flow(m_topBar, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(m_topBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		lv_obj_set_size(m_topBar, LV_PCT(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_grow(m_ipAddress, 3);
		lv_obj_set_flex_grow(m_refresh.getRoot(), 1);
		lv_obj_set_height(m_ipAddress, LV_SIZE_CONTENT);
		lv_label_set_text(m_ipAddress, utils::format(_("settings_network_ip_address"), "").c_str());

		// Network List
		lv_obj_set_flex_flow(m_networkList, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(m_networkList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		lv_obj_set_size(m_networkList, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_pad_column(m_networkList, 5, 0);
		lv_obj_set_style_pad_row(m_networkList, 5, 0);
		lv_obj_set_style_pad_all(m_networkList, 5, 0);

		// Network Selection
		lv_obj_set_flex_grow(m_networkList, 1);
		lv_obj_set_width(m_networkList, LV_PCT(100));
		lv_table_set_column_count(m_networkList, 5);
		lv_table_set_cell_value(m_networkList, 0, 0, _("settings_network_ssid"));
		lv_table_set_cell_value(m_networkList, 0, 1, _("settings_network_signal"));
		lv_table_set_cell_value(m_networkList, 0, 2, _("settings_network_known"));
		lv_table_set_cell_value(m_networkList, 0, 3, _("settings_network_forget"));
		lv_table_set_cell_value(m_networkList, 0, 4, _("settings_network_connected"));

		// Password Window
		lv_obj_add_flag(m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(m_passwordWindow, LV_OBJ_FLAG_FLOATING);
		lv_obj_align(m_passwordWindow, LV_ALIGN_CENTER, 0, 0);
		lv_obj_set_size(m_passwordWindow, LV_PCT(80), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(m_passwordWindow, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(m_passwordWindow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_passwordWindow.setTitle(_("settings_network_password_title"));
		m_passwordWindow.setText("");
		m_passwordWindow.okVisible(true);
		m_passwordWindow.cancelVisible(true);
		m_passwordWindow.setOkCallback([this]() { onPasswordConfirmEvent(); });
		m_passwordWindow.setCloseCallback([this]() { onPasswordCloseEvent(); });
		m_passwordInput.setPlaceholderText(_("settings_network_enter_password"));
		m_passwordInput.setPasswordMode(true);
		m_passwordInput.setOneLine(true);

		// Refresh
		m_refresh.addClickedCallback(onRefreshEvent, this);

		// Callbacks
		lv_obj_add_event_cb(m_networkList, onNetworkSelectionEvent, LV_EVENT_VALUE_CHANGED, this);
	}

	void NetworkSettingsView::setIpAddress(const std::string& ipAddress)
	{
		UI_LOCK();
		lv_label_set_text(m_ipAddress, utils::format(_("settings_network_ip_address"), ipAddress.c_str()).c_str());
	}

	void NetworkSettingsView::setEnabled(bool enabled)
	{
		UI_LOCK();
	}

	void NetworkSettingsView::setNetworkCount(size_t count)
	{
		UI_LOCK();
		lv_table_set_row_count(m_networkList, count + 1);
	}

	void NetworkSettingsView::setNetworkDetails(
		size_t index, const std::string& ssid, int32_t signalLevel, bool known, bool connected)
	{
		UI_LOCK();
		void* knownPtr = lv_malloc(sizeof(bool));
		*(bool*)knownPtr = known;
		lv_table_set_cell_user_data(m_networkList, index + 1, 2, knownPtr);
		lv_table_set_cell_value(m_networkList, index + 1, 0, ssid.c_str());
		lv_table_set_cell_value(m_networkList, index + 1, 1, utils::format("%d dBm", signalLevel).c_str());
		lv_table_set_cell_value(m_networkList, index + 1, 2, known ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
		lv_table_set_cell_value(m_networkList, index + 1, 3, known ? LV_SYMBOL_TRASH : "");
		lv_table_set_cell_value(m_networkList, index + 1, 4, connected ? LV_SYMBOL_WIFI : "");
	}

	void NetworkSettingsView::onNetworkSelectionEvent(lv_event_t* e)
	{
		UI_LOCK();

		NetworkSettingsView* view = (NetworkSettingsView*)lv_event_get_user_data(e);
		lv_obj_t* table = (lv_obj_t*)lv_event_get_target(e);
		uint32_t row;
		uint32_t col;
		lv_table_get_selected_cell(view->m_networkList, &row, &col);

		view->getMainSettingsView().showKeyboard(false);

		if (row == 0)
		{
			return;
		}

		if (col == 3)
		{
			view->getPresenter()->forgetNetwork(lv_table_get_cell_value(table, row, 0));
			return;
		}

		const char* ssid = lv_table_get_cell_value(table, row, 0);
		bool known = *(bool*)lv_table_get_cell_user_data(table, row, 2);
		if (!known)
		{
			view->m_passwordInput.setText("");
			view->m_passwordInput.showPassword(false);
			view->m_passwordWindow.setText(ssid);
			view->getMainSettingsView().showKeyboard(
				true, LV_KEYBOARD_MODE_TEXT_LOWER, view->m_passwordInput.getTextArea());
			lv_obj_remove_flag(view->m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
			return;
		}

		view->getPresenter()->connectToNetwork(ssid);
	}

	void NetworkSettingsView::onPasswordCloseEvent()
	{
		UI_LOCK();
		getMainSettingsView().showKeyboard(false);
		m_passwordWindow.setFlag(LV_OBJ_FLAG_HIDDEN, true);
	}

	void NetworkSettingsView::onPasswordConfirmEvent()
	{
		UI_LOCK();
		getPresenter()->connectToNetwork(m_passwordWindow.getText().getText(), m_passwordInput.getText());
	}

	void NetworkSettingsView::onRefreshEvent(lv_event_t* e)
	{
		UI_LOCK();
		NetworkSettingsView* view = (NetworkSettingsView*)lv_event_get_user_data(e);
		lv_obj_add_flag(view->m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
		view->getPresenter()->scanWifi();
	}

	void NetworkSettingsView::onShow()
	{
		UI_LOCK();
		lv_obj_add_flag(m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
		getPresenter()->scanWifi();
	}

	void NetworkSettingsView::onHide()
	{
		UI_LOCK();
		lv_obj_add_flag(m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
	}

	DeveloperSettingsView::DeveloperSettingsView(LvObj& parent, SettingsView& mainSettingsView)
		: SettingsSubView("developer_settings_view", parent, mainSettingsView)
		, m_debugLevelCont(lv_obj_create(getRoot()))
		, m_debugLevelLabel(lv_label_create(m_debugLevelCont))
		, m_debugLevel(lv_dropdown_create(m_debugLevelCont))
#if DEBUG_BORDERS
		, m_debugBorders(lv_checkbox_create(getRoot()))
#endif
		, m_enableSSH(lv_checkbox_create(getRoot()))
		, m_restart("developer_settings_restart", getRoot(), _("settings_restart"))
		, m_eraseAndRestart("developer_settings_erase_and_restart", getRoot(), _("settings_erase_and_restart"))
		, m_reboot("developer_settings_reboot", getRoot(), _("settings_reboot"))
		, m_startHardwareTest("start_hardware_test", getRoot())
	{
		UI_LOCK();

		// Debug Level
		lv_obj_set_flex_flow(m_debugLevelCont, LV_FLEX_FLOW_ROW);
		lv_obj_set_size(m_debugLevelCont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_pad_column(m_debugLevelCont, 5, 0);
		lv_label_set_text(m_debugLevelLabel, _("settings_debug_level"));
		std::string options;
		for (const auto& level : Log::DebugLevelStrings)
		{
			options += level;
			options += "\n";
		}
		lv_dropdown_set_options(m_debugLevel, options.c_str());
		lv_dropdown_set_selected(m_debugLevel, static_cast<uint32_t>(Log::GetDebugLevel()));
		lv_dropdown_set_selected_highlight(m_debugLevel, true);
		lv_obj_add_event_cb(m_debugLevel, onDebugLevelEvent, LV_EVENT_VALUE_CHANGED, NULL);

#if DEBUG_BORDERS
		lv_checkbox_set_text(m_debugBorders, _("settings_debug_borders"));
		lv_obj_set_state(m_debugBorders, LV_STATE_CHECKED, Themes::isdebugBorderVisible(lv_screen_active()));
		lv_obj_add_event_cb(m_debugBorders, onDebugBordersEvent, LV_EVENT_VALUE_CHANGED, this);
#endif

		lv_checkbox_set_text(m_enableSSH, _("settings_enable_ssh"));
		lv_obj_set_state(m_enableSSH, LV_STATE_CHECKED, StorageHelper::getData<bool>(ID_SSH_ENABLED, false));
		lv_obj_add_event_cb(m_enableSSH, onEnableSSHEvent, LV_EVENT_VALUE_CHANGED, this);

		// Power
		lv_obj_set_height(m_restart.getRoot(), LV_SIZE_CONTENT);
		lv_obj_set_height(m_eraseAndRestart.getRoot(), LV_SIZE_CONTENT);
		lv_obj_set_height(m_reboot.getRoot(), LV_SIZE_CONTENT);
		m_restart.addClickedCallback(onRestartEvent, this);
		m_eraseAndRestart.addClickedCallback(onEraseAndRestartEvent, this);
		m_reboot.addClickedCallback(onRebootEvent, this);

		/* Hardware Test */
		m_startHardwareTest.setText(_("settings_start_hardware_test"));
		m_startHardwareTest.addClickedCallback(
			[](lv_event_t* e)
			{
				auto& view = *static_cast<DeveloperSettingsView*>(lv_event_get_user_data(e));
				view.getMainSettingsPresenter()->startHardwareTest();
			},
			this);
	}

	void DeveloperSettingsView::onDebugLevelEvent(lv_event_t* e)
	{
		UI_LOCK();
		lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
		size_t lvl = lv_dropdown_get_selected(dropdown);
		Log::SetDebugLevel(static_cast<Log::DebugLevel>(lvl));
	}

#if DEBUG_BORDERS
	void DeveloperSettingsView::onDebugBordersEvent(lv_event_t* e)
	{
		UI_LOCK();
		lv_obj_t* cb = (lv_obj_t*)lv_event_get_target(e);
		bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
		StorageHelper::setData<bool>(ID_DEBUG_BORDERS, checked);
		Themes::showDebugBorders(lv_screen_active(), checked);
	}
#endif

	void DeveloperSettingsView::onEnableSSHEvent(lv_event_t* e)
	{
		UI_LOCK();
		lv_obj_t* cb = (lv_obj_t*)lv_event_get_target(e);
		bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);
		LOG_INFO("{:s} SSH", checked ? "Enabling" : "Disabling");
		StorageHelper::setData<bool>(ID_SSH_ENABLED, checked);
		if (checked)
		{
#if !SIMULATION
			system("mv /etc/init.d/50dropbear /etc/init.d/S50dropbear;"
				   "/etc/init.d/S50dropbear start");
#endif
		}
		else
		{
#if !SIMULATION
			system("/etc/init.d/S50dropbear stop;"
				   "mv /etc/init.d/S50dropbear /etc/init.d/50dropbear");
#endif
		}
	}

	void DeveloperSettingsView::onRestartEvent(lv_event_t* e)
	{
		UI_LOCK();
		Restart();
	}

	void DeveloperSettingsView::onEraseAndRestartEvent(lv_event_t* e)
	{
		UI_LOCK();
		EraseAndRestart();
	}

	void DeveloperSettingsView::onRebootEvent(lv_event_t* e)
	{
		UI_LOCK();
		Reboot();
	}
} // namespace UI

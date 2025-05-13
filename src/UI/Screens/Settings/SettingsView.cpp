#include "SettingsView.h"
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
	SettingsView::SettingsView(lv_obj_t* parent)
		: View("settings_view", parent, layout_t(0, 0, 100, 100))
		, m_settingsList(lv_list_create(getCont()))
		, m_subWindow(lv_obj_create(getCont()))
		, m_keyboard(lv_keyboard_create(getCont()))
		, m_connectivityHeader(lv_list_add_text(m_settingsList, _("settings_connectivity_header")))
		, m_duetSettings(lv_list_add_button(m_settingsList, NULL, _("settings_duet")))
		, m_deviceSettings(lv_list_add_button(m_settingsList, NULL, _("settings_device")))
		, m_networkSettings(lv_list_add_button(m_settingsList, LV_SYMBOL_WIFI, _("settings_network")))
		, m_devHeader(lv_list_add_text(m_settingsList, _("settings_dev_header")))
		, m_developerSettings(lv_list_add_button(m_settingsList, LV_SYMBOL_SETTINGS, _("settings_developer")))
		, m_duetSettingsView(m_subWindow, *this)
		, m_deviceSettingsView(m_subWindow, *this)
		, m_networkSettingsView(m_subWindow, *this)
		, m_developerSettingsView(m_subWindow, *this)
		, m_currentSubView(&m_duetSettingsView)
	{
		UI_LOCK();
		// Layout
		lv_obj_set_layout(getCont(), LV_LAYOUT_GRID);
		lv_obj_set_grid_dsc_array(getCont(), m_layoutColDsc, m_layoutRowDsc);
		lv_obj_set_grid_cell(m_settingsList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_subWindow, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_keyboard, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 1, 1);

		// List
		lv_obj_set_user_data(m_duetSettings, &m_duetSettingsView);
		lv_obj_set_user_data(m_deviceSettings, &m_deviceSettingsView);
		lv_obj_set_user_data(m_networkSettings, &m_networkSettingsView);
		lv_obj_set_user_data(m_developerSettings, &m_developerSettingsView);

		lv_obj_add_event_cb(m_duetSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_deviceSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_networkSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_developerSettings, onWindowSelectEvent, LV_EVENT_CLICKED, this);

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
		subView->show();
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
		m_currentSubView->show();
	}

	SettingsSubView::SettingsSubView(const std::string& name, lv_obj_t* parent, SettingsView& mainSettingsView)
		: BaseView(name, parent, layout_t(0, 0, 100, 100))
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

	DuetSettingsView::DuetSettingsView(lv_obj_t* parent, SettingsView& mainSettingsView)
		: SettingsSubView("duet_settings_view", parent, mainSettingsView)
		, m_connectionMethod("duet_settings_connection_method", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_usbSettings(*this)
		, m_wifiSettings(*this)
		, m_uartSettings(*this)
		, m_pollInterval("duet_settings_poll_interval", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_infoTimeout("duet_settings_info_timeout", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
	{
		UI_LOCK();

		// Connection Method
		std::vector<std::string> options;
		for (const auto& method : Comm::duetCommunicationTypeNames)
		{
			options.push_back(_(method));
		}
		m_connectionMethod.setLabel(_("settings_duet_connection_method"));
		m_connectionMethod.setOptions(options);
		m_connectionMethod.addEventCallback(onConnectionMethodEvent, LV_EVENT_VALUE_CHANGED, this);
		m_connectionMethod.setSelected((uint32_t)Comm::DUET.GetCommunicationType(), LV_ANIM_OFF);

		// Poll Interval
		m_pollInterval.setLabel(_("settings_duet_poll_interval"));
		m_pollInterval.setOutOfRangeMode(Slider::OutOfRange::UPPER);
		m_pollInterval.setRange(MIN_PRINTER_POLL_INTERVAL, 2000);
		m_pollInterval.setValue(Comm::DUET.GetPollInterval());
		m_pollInterval.setValueChangedCallback([](int32_t value) { Comm::DUET.SetPollInterval((uint32_t)value); });
		m_pollInterval.setKeyboard(getMainSettingsView().getKeyboard());
		m_pollInterval.setFocusedCallback(
			[this](bool focused)
			{ getMainSettingsView().showKeyboard(focused, LV_KEYBOARD_MODE_NUMBER, m_pollInterval.getInput()); });

		// Info Timeout
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
		: BaseView("duet_settings_usb", parent, layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
	{
		UI_LOCK();
	}

	DuetSettingsView::WifiSettings::WifiSettings(DuetSettingsView& parent)
		: BaseView("duet_settings_wifi", parent, layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_hostname("duet_settings_hostname", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_password("duet_settings_password", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_hostname.setOneLine(true);
		m_hostname.setLabel(_("settings_duet_hostname"));
		m_hostname.setPlaceholderText(_("settings_duet_hostname_prompt"));
		m_hostname.setAcceptedChars("0123456789.");
		m_hostname.setText(Comm::DUET.GetHostname());
		m_hostname.addEventCallback(onTextAreaEvent, LV_EVENT_ALL, &parent);
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
		m_password.addEventCallback(onTextAreaEvent, LV_EVENT_ALL, &parent);
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
		: BaseView("duet_settings_uart", parent, layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
	{
		UI_LOCK();
	}

	void DuetSettingsView::onConnectionMethodEvent(lv_event_t* e)
	{
		UI_LOCK();
		DuetSettingsView* view = (DuetSettingsView*)lv_event_get_user_data(e);
		Comm::DUET.SetCommunicationType((Comm::CommunicationType)(view->m_connectionMethod.getSelected()));
		view->showConnectionMethodSettings(Comm::DUET.GetCommunicationType());
	}

	void DuetSettingsView::showConnectionMethodSettings(const Comm::CommunicationType method)
	{
		UI_LOCK();

		m_usbSettings.show(method == Comm::CommunicationType::usb);
		m_wifiSettings.show(method == Comm::CommunicationType::network);
		m_uartSettings.show(method == Comm::CommunicationType::uart);
		lv_obj_move_foreground(m_usbSettings);
		lv_obj_move_foreground(m_wifiSettings);
		lv_obj_move_foreground(m_uartSettings);
		lv_obj_move_to_index(m_usbSettings, 1);
		lv_obj_move_to_index(m_wifiSettings, 1);
		lv_obj_move_to_index(m_uartSettings, 1);
	}

	void DuetSettingsView::onShow()
	{
		UI_LOCK();
		m_pollInterval.setValue(Comm::DUET.GetPollInterval());
		m_infoTimeout.setValue(StorageHelper::getData(ID_INFO_TIMEOUT, DEFAULT_POPUP_TIMEOUT));
		showConnectionMethodSettings(Comm::DUET.GetCommunicationType());
	}

	DeviceSettingsView::DeviceSettingsView(lv_obj_t* parent, SettingsView& mainSettingsView)
		: SettingsSubView("device_settings_view", parent, mainSettingsView)
		, m_firmwareVersion(lv_label_create(getCont()))
		, m_buildTime(lv_label_create(getCont()))
		, m_language("device_settings_language", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_theme("device_settings_theme", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_usbMode("device_settings_usb_mode", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_brightness("settings_brightness", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_screensaverTimeout("settings_screensaver_timeout", getCont(), layout_t(0, 0, 100, LV_SIZE_CONTENT))
		, m_systemLogging(lv_checkbox_create(getCont()))
	{
		UI_LOCK();

		lv_label_set_text(m_firmwareVersion, utils::format(_("settings_firmware_version"), FIRMWARE_VERSION).c_str());
		lv_label_set_text(m_buildTime, utils::format(_("settings_build_time"), __DATE__, __TIME__).c_str());

		m_language.setLabel(_("settings_language"));
		m_language.setOptions(_("settings_language_en"));

		m_theme.setLabel(_("settings_theme"));
		m_theme.setOptions(_("settings_theme_light"));

		m_usbMode.setLabel(_("settings_usb_mode"));
		m_usbMode.setOptions({_("settings_usb_mode_auto_detect"),
							  _("settings_usb_mode_host"),
							  _("settings_usb_mode_device"),
							  _("settings_usb_mode_internal_wifi")});
		m_usbMode.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
				DeviceSettingsView* view = (DeviceSettingsView*)lv_event_get_user_data(e);
				int32_t selected = view->m_usbMode.getSelected();
				view->getMainSettingsPresenter()->setUsbMode((UsbMode(selected)));
			},
			LV_EVENT_VALUE_CHANGED,
			this);
		m_usbMode.setSelected(StorageHelper::getData(ID_USB_MODE, 0));

		// Brightness
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
		m_screensaverTimeout.setLabel(_("settings_screensaver_timeout"));
		m_screensaverTimeout.setRange(0, 30 * 60); // seconds
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
	}

	void DeviceSettingsView::onShow()
	{
		UI_LOCK();
		m_brightness.setValue(DisplayHelper::getBrightness());
		m_screensaverTimeout.setValue(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT, DEFAULT_SCREEN_TIMEOUT) / 1000);
	}

	NetworkSettingsView::NetworkSettingsView(lv_obj_t* parent, SettingsView& mainSettingsView)
		: View("network_settings_view", parent, mainSettingsView)
		, m_topBar(lv_obj_create(getCont()))
		, m_ipAddress(lv_label_create(m_topBar))
		, m_enable(lv_checkbox_create(m_topBar))
		, m_refresh("network_settings_refresh", m_topBar, _("refresh"), layout_t{0, 0, 0, LV_SIZE_CONTENT})
		, m_networkList(lv_table_create(getCont()))
		, m_passwordWindow(lv_msgbox_create(getCont()))
		, m_passwordInput(lv_textarea_create(m_passwordWindow))
		, m_passwordSsid(nullptr)
	{
		UI_LOCK();

		lv_obj_set_flex_flow(getCont(), LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(getCont(), LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		lv_obj_set_style_pad_all(m_topBar, 2, 0);
		lv_obj_set_flex_flow(m_topBar, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(m_topBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		lv_obj_set_size(m_topBar, LV_PCT(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_grow(m_ipAddress, 3);
		lv_obj_set_flex_grow(m_refresh.getCont(), 1);
		lv_obj_set_height(m_ipAddress, LV_SIZE_CONTENT);
		lv_obj_set_size(m_enable, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_label_set_text(m_ipAddress, utils::format(_("settings_network_ip_address"), "").c_str());
		lv_checkbox_set_text(m_enable, _("settings_network_enable"));

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
		lv_obj_set_width(m_passwordInput, LV_PCT(80));

		lv_msgbox_add_title(m_passwordWindow, _("settings_network_password_title"));
		m_passwordSsid = lv_msgbox_add_text(m_passwordWindow, "");
		lv_obj_t* closeBtn = lv_msgbox_add_header_button(m_passwordWindow, LV_SYMBOL_CLOSE);
		lv_obj_t* confirmBtn = lv_msgbox_add_footer_button(m_passwordWindow, LV_SYMBOL_OK);
		lv_textarea_set_placeholder_text(m_passwordInput, _("settings_network_enter_password"));
		lv_textarea_set_one_line(m_passwordInput, true);
		lv_textarea_set_password_mode(m_passwordInput, true);

		// Refresh
		m_refresh.setCallback(onRefreshEvent, LV_EVENT_CLICKED, this);

		// Callbacks
		lv_obj_add_event_cb(m_enable, onEnableEvent, LV_EVENT_VALUE_CHANGED, this);
		lv_obj_add_event_cb(m_networkList, onNetworkSelectionEvent, LV_EVENT_VALUE_CHANGED, this);
		lv_obj_add_event_cb(closeBtn, onPasswordCloseEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(confirmBtn, onPasswordConfirmEvent, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(m_passwordWindow, onPasswordCloseEvent, LV_EVENT_DEFOCUSED, this);
	}

	void NetworkSettingsView::setIpAddress(const std::string& ipAddress)
	{
		UI_LOCK();
		lv_label_set_text(m_ipAddress, utils::format(_("settings_network_ip_address"), ipAddress.c_str()).c_str());
	}

	void NetworkSettingsView::setEnabled(bool enabled)
	{
		UI_LOCK();
		lv_obj_set_state(m_enable, LV_STATE_CHECKED, enabled);
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

	void NetworkSettingsView::onEnableEvent(lv_event_t* e)
	{
		UI_LOCK();
		NetworkSettingsView* view = (NetworkSettingsView*)lv_event_get_user_data(e);
		view->getPresenter()->setWifiEnabled(lv_obj_has_state(view->m_enable, LV_STATE_CHECKED));
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
			lv_textarea_set_text(view->m_passwordInput, "");
			lv_label_set_text(view->m_passwordSsid, ssid);
			view->getMainSettingsView().showKeyboard(true, LV_KEYBOARD_MODE_TEXT_LOWER, view->m_passwordInput);
			lv_obj_remove_flag(view->m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
			return;
		}

		view->getPresenter()->connectToNetwork(ssid);
	}

	void NetworkSettingsView::onPasswordCloseEvent(lv_event_t* e)
	{
		UI_LOCK();
		NetworkSettingsView* view = (NetworkSettingsView*)lv_event_get_user_data(e);
		view->getMainSettingsView().showKeyboard(false);
		lv_obj_add_flag(view->m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
	}

	void NetworkSettingsView::onPasswordConfirmEvent(lv_event_t* e)
	{
		UI_LOCK();
		NetworkSettingsView* view = (NetworkSettingsView*)lv_event_get_user_data(e);
		view->getMainSettingsView().showKeyboard(false);
		lv_obj_add_flag(view->m_passwordWindow, LV_OBJ_FLAG_HIDDEN);
		view->getPresenter()->connectToNetwork(lv_label_get_text(view->m_passwordSsid),
											   lv_textarea_get_text(view->m_passwordInput));
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

	DeveloperSettingsView::DeveloperSettingsView(lv_obj_t* parent, SettingsView& mainSettingsView)
		: SettingsSubView("developer_settings_view", parent, mainSettingsView)
		, m_debugLevelCont(lv_obj_create(getCont()))
		, m_debugLevelLabel(lv_label_create(m_debugLevelCont))
		, m_debugLevel(lv_dropdown_create(m_debugLevelCont))
#if DEBUG_BORDERS
		, m_debugBorders(lv_checkbox_create(getCont()))
#endif
		, m_enableSSH(lv_checkbox_create(getCont()))
		, m_restart("developer_settings_restart", getCont(), _("settings_restart"))
		, m_eraseAndRestart("developer_settings_erase_and_restart", getCont(), _("settings_erase_and_restart"))
		, m_reboot("developer_settings_reboot", getCont(), _("settings_reboot"))
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
		lv_dropdown_set_selected(m_debugLevel, static_cast<uint32_t>(Log::GetDebugLevel()), false);
		lv_dropdown_set_selected_highlight(m_debugLevel, true);
		lv_obj_add_event_cb(m_debugLevel, onDebugLevelEvent, LV_EVENT_VALUE_CHANGED, NULL);

#if DEBUG_BORDERS
		lv_checkbox_set_text(m_debugBorders, _("settings_debug_borders"));
		lv_obj_set_state(m_debugBorders,
						 LV_STATE_CHECKED,
						 Styles::instance().hasStyle(lv_screen_active(), &Styles::instance().debugBorders.style));
		lv_obj_add_event_cb(m_debugBorders, onDebugBordersEvent, LV_EVENT_VALUE_CHANGED, this);
#endif

		lv_checkbox_set_text(m_enableSSH, _("settings_enable_ssh"));
		lv_obj_set_state(m_enableSSH, LV_STATE_CHECKED, StorageHelper::getData<bool>(ID_SSH_ENABLED, false));
		lv_obj_add_event_cb(m_enableSSH, onEnableSSHEvent, LV_EVENT_VALUE_CHANGED, this);

		// Power
		lv_obj_set_height(m_restart.getCont(), LV_SIZE_CONTENT);
		lv_obj_set_height(m_eraseAndRestart.getCont(), LV_SIZE_CONTENT);
		lv_obj_set_height(m_reboot.getCont(), LV_SIZE_CONTENT);
		m_restart.setCallback(onRestartEvent, LV_EVENT_CLICKED, this);
		m_eraseAndRestart.setCallback(onEraseAndRestartEvent, LV_EVENT_CLICKED, this);
		m_reboot.setCallback(onRebootEvent, LV_EVENT_CLICKED, this);
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
		Styles::instance().showDebugBorders(lv_screen_active(), checked);
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

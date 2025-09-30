#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class SettingsView;
	class NetworkSettingsView;

	enum class UsbMode
	{
		Host,
		Device,
		InternalWiFi
	};

	class SettingsPresenter : public Presenter<SettingsView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(SettingsPresenter, SettingsView)

		// Actions
		void setUsbMode(UsbMode mode);
		void startHardwareTest();

	  private:
		virtual void onInit() override;

		void setUsbHost(bool host);
		void setUsbMux(bool usbc);
		void setUsbState(bool state);
	};

	class NetworkSettingsPresenter : public Presenter<NetworkSettingsView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(NetworkSettingsPresenter, NetworkSettingsView)

		// Actions
		void setWifiEnabled(bool enabled);
		void scanWifi();
		void connectToNetwork(std::string_view ssid);
		void connectToNetwork(std::string_view ssid, std::string_view password);
		void forgetNetwork(std::string_view ssid);
		void refresh();

	  private:
		void onInit() override;
		void onActivate() override;
		void onDeactivate() override;

		lv_timer_t* m_scanTimer = nullptr;
	};
} // namespace UI

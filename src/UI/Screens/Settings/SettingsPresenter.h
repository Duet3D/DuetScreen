#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class SettingsView;
	class NetworkSettingsView;

	enum class UsbMode
	{
		UsbAuto,
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

	  private:
		virtual void onInit() override;

		void setUsbHost(bool host);
		void setUsbMux(bool usbc);
	};

	class NetworkSettingsPresenter : public Presenter<NetworkSettingsView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(NetworkSettingsPresenter, NetworkSettingsView)

		// Actions
		void setWifiEnabled(bool enabled);
		void scanWifi();
		void connectToNetwork(const std::string& ssid);
		void connectToNetwork(const std::string& ssid, const std::string& password);
		void forgetNetwork(const std::string& ssid);

	  private:
		void onActivate() override;
	};
} // namespace UI

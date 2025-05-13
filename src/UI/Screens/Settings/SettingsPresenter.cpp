#include "SettingsPresenter.h"
#include "Pins.h"
#include "SettingsView.h"
#include "utils/GpioHelper.h"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"

namespace UI
{
	void SettingsPresenter::setUsbMode(UsbMode mode)
	{
		LOG_DBG("Setting USB mode to {:d}", static_cast<int>(mode));
		GpioHelper::stopMonitoring(GPIO_USB_DEVICE_DETECT);
		switch (mode)
		{
		case UsbMode::UsbAuto:
			setUsbHost(GpioHelper::getPinValue(GPIO_USB_DEVICE_DETECT) == 1);
			setUsbMux(true);
			GpioHelper::monitorPin(GPIO_USB_DEVICE_DETECT,
								   [this](int pin, int value)
								   {
									   LOG_DBG("USB device detect changed to {:d}", value);
									   setUsbHost(value == 1);
								   });
			break;
		case UsbMode::Host:
			setUsbHost(true);
			setUsbMux(true);
			break;
		case UsbMode::Device:
			setUsbHost(false);
			setUsbMux(true);
			break;
		case UsbMode::InternalWiFi:
			setUsbHost(true);
			setUsbMux(false);
			break;
		}
		LOG_INFO("USB mode set to {:d}", static_cast<int>(mode));
		StorageHelper::setData(ID_USB_MODE, static_cast<int>(mode));
	}

	void SettingsPresenter::setUsbHost(bool host)
	{
		LOG_DBG("Setting USB host to {:d}", host);
		GpioHelper::setPinValue(GPIO_USB_STATE, host ? 0 : 1);
	}

	void SettingsPresenter::setUsbMux(bool usbc)
	{
		LOG_DBG("Setting USB mux to {:d}", usbc);
		GpioHelper::setPinValue(GPIO_USB_SELECT, usbc ? 1 : 0);
	}

	void SettingsPresenter::onInit()
	{
		// Set the USB mode based on the stored value
		int usbMode = StorageHelper::getData(ID_USB_MODE, 0);
		setUsbMode(static_cast<UsbMode>(usbMode));
	}

	void NetworkSettingsPresenter::setWifiEnabled(bool enabled)
	{
		NetworkHelper::enable(enabled);
		scanWifi();
	}

	void NetworkSettingsPresenter::connectToNetwork(const std::string& ssid)
	{
		NetworkHelper::connect(ssid);
		scanWifi();
	}

	void NetworkSettingsPresenter::connectToNetwork(const std::string& ssid, const std::string& password)
	{
		NetworkHelper::connect(ssid, password);
		scanWifi();
	}

	void NetworkSettingsPresenter::forgetNetwork(const std::string& ssid)
	{
		NetworkHelper::forgetNetwork(ssid);
		scanWifi();
	}

	void NetworkSettingsPresenter::scanWifi()
	{
#if SIMULATION
		std::vector<WiFiNetwork> networks = {
			{"Network 1", 100, 1}, {"Network 2", 75, 2}, {"Network 3", 50, 3}, {"Network 4", 25, -1}};
#else
		std::vector<WiFiNetwork> networks = NetworkHelper::scanWiFiNetworks();
#endif

		m_view->setNetworkCount(networks.size());
		for (size_t i = 0; i < networks.size(); ++i)
		{
			m_view->setNetworkDetails(
				i, networks[i].ssid, networks[i].signal_level, networks[i].id != -1, networks[i].connected);
		}
		m_view->setIpAddress(NetworkHelper::getIpAddress());
	}

	void NetworkSettingsPresenter::onActivate()
	{
		m_view->setEnabled(NetworkHelper::isEnabled());
		m_view->setIpAddress(NetworkHelper::getIpAddress());
	}
} // namespace UI

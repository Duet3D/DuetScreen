/*
 * UsbMode.cpp
 */

#include "UsbMode.h"
#include "Debug.h"
#include "Pins.h"
#include "Storage.h"
#include "nameof.hpp"
#include "tracy/Tracy.hpp"
#include "utils/GpioHelper.h"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"
#include <fstream>
#include <string>
#include <string_view>

namespace Comm
{
	/** Make the USB-C port's controller the host (true) or a device (false) */
	static void setUsbHost(bool host);

	/** Route the USB-C port to the connector (true) or the internal WiFi module (false) */
	static void setUsbMux(bool usbc);

	/** Set the USB state pin */
	static void setUsbState(bool state);

	static libusb_context* s_context = nullptr;	 // Made by usbInit()
	static UsbMode s_usbMode = UsbMode::Unknown; // What the port is used for

	void usbInit()
	{
		ZoneScoped;
		setUsbMode(StorageHelper::getData(ID_USB_MODE));
		if (s_context == nullptr && libusb_init(&s_context) != LIBUSB_SUCCESS)
		{
			LOG_ERROR("Couldn't start libusb");
			s_context = nullptr;
		}
	}

	libusb_context* usbContext()
	{
		return s_context;
	}

	void setUsbMode(UsbMode mode)
	{
		ZoneScoped;
		LOG_DBG("Setting USB mode to {:s}", nameof::nameof_enum(mode));
		switch (mode)
		{
		case UsbMode::Host:
			setUsbHost(true);
			setUsbMux(true);
			setUsbState(true);
			break;
		case UsbMode::Device:
			setUsbHost(false);
			setUsbMux(true);
			setUsbState(false);
			break;
		case UsbMode::InternalWiFi:
			setUsbHost(true);
			setUsbMux(false);
			setUsbState(false); /* probably `false` because smart USB chargers reset the screen in wifi mode if the
								   state pin doesn't signal device mode */
			NetworkHelper::enable(true);
			break;
		default:
			LOG_FATAL_THROW("Unknown USB mode");
		}
		LOG_INFO("USB mode set to {:s}", nameof::nameof_enum(mode));
		StorageHelper::setData(ID_USB_MODE, mode);
		s_usbMode = mode;
	}

	UsbMode getUsbMode()
	{
		ZoneScoped;
		return s_usbMode;
	}

	static void setUsbHost(bool host)
	{
		ZoneScoped;
		const std::string_view desired = host ? "usb_host" : "usb_device";

		// Read current role to avoid unnecessary writes; opening the sysfs file
		// with ofstream truncates it, which can momentarily clear the USB role
		// and cause an unwanted device/host transition.
		std::ifstream ifs(USB_OTG_ROLE_PATH);
		if (ifs.is_open())
		{
			std::string current;
			std::getline(ifs, current);
			ifs.close();
			if (current == desired)
			{
				return;
			}
		}

		std::ofstream ofs(USB_OTG_ROLE_PATH);
		ofs << desired;
		ofs.close();
	}

	static void setUsbMux(bool usbc)
	{
		ZoneScoped;
		GpioHelper::setPinValue(GPIO_USB_SELECT, usbc ? 1 : 0);
	}

	static void setUsbState(bool state)
	{
		ZoneScoped;
		GpioHelper::setPinValue(GPIO_USB_STATE, state ? 1 : 0);
	}
} // namespace Comm

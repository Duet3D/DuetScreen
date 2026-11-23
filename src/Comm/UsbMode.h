/*
 * UsbMode.h
 *
 * The screen's USB-C port: what it is for (host to a Duet, a device to a computer, or the internal WiFi module), set
 * through its role, multiplexer and state pins, and the libusb context the USB port to a Duet (Ports/UsbPort.h) uses.
 */

#pragma once

#include <libusb-1.0/libusb.h>

namespace Comm
{
	/** What the USB-C port is for; the value is kept with the settings */
	enum class UsbMode
	{
		Host,		  // The screen is the host of a Duet
		Device,		  // The screen is a device of a computer
		InternalWiFi, // The port goes to the internal WiFi module
		Unknown,
	};

	/** Set the port as the settings say, and make the libusb context (once) */
	void usbInit();

	/** Use the port for `mode`, and keep it with the settings */
	void setUsbMode(UsbMode mode);

	/** What the port is used for */
	UsbMode getUsbMode();

	/** The libusb context the USB port to a Duet uses, or nullptr if usbInit() hasn't made it */
	libusb_context* usbContext();
} // namespace Comm

/*
 * UsbActions.h
 *
 * The global actions of the shared USB file dialog (src/UI/Xml/Data/UsbSource.h), which the file browser opens for a
 * file on a USB drive: `actions.usb.destination` chooses the printer's folder it is uploaded to, `actions.usb.upload`
 * uploads it, `actions.usb.cancel_upload` cancels that, and `actions.usb.install` updates the screen with it. What
 * each does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::UsbActions
{
	/** Register the USB file dialog actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::UsbActions

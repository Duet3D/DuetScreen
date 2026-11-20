/*
 * UsbPort.h
 *
 * The Duet on the screen's USB host port, as a SerialPort: one data channel, or two on a Duet 3 (see Usb.h).
 */

#pragma once

#include "SerialPort.h"

namespace Comm
{
	class UsbPort : public SerialPort
	{
	  public:
		/** Close the device if it is open */
		~UsbPort() override;

		bool open(Receiver receiver) override;
		void close() override;
		bool write(size_t channel, std::string_view bytes) override;
		size_t channels() const override;
		bool needsChecksums() const override { return false; }
	};
} // namespace Comm

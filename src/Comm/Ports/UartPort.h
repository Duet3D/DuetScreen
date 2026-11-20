/*
 * UartPort.h
 *
 * A Duet wired to the screen's UART (its PanelDue port), as a SerialPort: one channel, lines checksummed.
 */

#pragma once

#include "Hardware/UartController.h"
#include "SerialPort.h"
#include <string>
#include <termios.h>

namespace Comm
{
	class UartPort : public SerialPort
	{
	  public:
		/**
		 * @param device The UART's device, e.g. "/dev/ttyS3"
		 * @param baudRate Its speed, as termios codes it (e.g. B115200)
		 */
		UartPort(std::string device, speed_t baudRate);

		bool open(Receiver receiver) override;
		void close() override;
		bool write(size_t channel, std::string_view bytes) override;
		size_t channels() const override { return 1; }
		bool needsChecksums() const override { return true; }

	  private:
		std::string m_device;  // The UART's device
		speed_t m_baudRate;	   // Its speed
		UartController m_uart; // The UART itself
	};
} // namespace Comm

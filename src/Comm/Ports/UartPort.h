/*
 * UartPort.h
 *
 * A Duet wired to the screen's UART (its PanelDue port), as a SerialPort: one channel, raw 8N1, every line
 * checksummed (bytes can be lost on a wire). A thread waits for what arrives and hands it on; a read or write error
 * marks the port failed(), so the connection starts again.
 */

#pragma once

#include "SerialPort.h"
#include <atomic>
#include <string>
#include <termios.h>
#include <thread>

namespace Comm
{
	class UartPort : public SerialPort
	{
	  public:
		/**
		 * @param device The UART's device, e.g. "/dev/ttyS5"
		 * @param baudRate Its speed, as termios codes it (e.g. B115200)
		 */
		UartPort(std::string device, speed_t baudRate);

		/** Close the UART if it is open */
		~UartPort() override;

		UartPort(const UartPort&) = delete;
		UartPort& operator=(const UartPort&) = delete;

		bool open(Receiver receiver) override;
		void close() override;
		bool write(size_t channel, std::string_view bytes) override;
		size_t channels() const override { return 1; }
		bool needsChecksums() const override { return true; }
		bool failed() const override { return m_failed.load(); }
		std::chrono::milliseconds replyTimeout() const override;
		size_t maxUploadSize() const override;

		/** Bits per second for the termios code `code` (e.g. 115200 for B115200), or 0 if it isn't one */
		static unsigned int bitsPerSecond(speed_t code);

	  private:
		/** Set the UART raw, 8N1, at the baud rate */
		bool configure();

		/** The read thread: wait for bytes and hand them on, until closed */
		void readLoop();

		std::string m_device;				// The UART's device
		speed_t m_baudRate;					// Its speed
		Receiver m_receiver;				// Given what is read
		int m_fd = -1;						// The open UART, or -1
		std::atomic<bool> m_reading{false}; // The read thread carries on
		std::atomic<bool> m_failed{false};	// Reading or writing failed
		std::thread m_reader;				// Waits for what arrives
	};
} // namespace Comm

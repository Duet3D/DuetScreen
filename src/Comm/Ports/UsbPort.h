/*
 * UsbPort.h
 *
 * A Duet on the screen's USB host port, as a SerialPort. A Duet is a CDC-ACM device: one data channel, or two on a
 * Duet 3 with RepRapFirmware 3.7 on (the second is used for the printer's state and emergency commands, unless the
 * settings say not to).
 *
 *   - open(): find the Duet, choose its channels, claim them, set the line coding, drop DTR briefly (so the Duet
 *     throws away any line it only partly received before) and raise it, and keep reads waiting on every channel
 *   - reads complete on the port's one thread (libusb's events), which hands the bytes on and reads again
 *   - write(): one transfer per line, waited for, so a Duet that has gone is noticed at once
 *   - a transfer that fails because the device has gone, or with an error, marks the port failed()
 */

#pragma once

#include "SerialPort.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace Comm
{
	class UsbPort : public SerialPort
	{
	  public:
		/** The most data channels a Duet has */
		static constexpr size_t maxChannels = 2;

		/** @param useSecondChannel Whether a second channel, if the Duet has one, is used */
		explicit UsbPort(bool useSecondChannel);

		/** Close the device if it is open */
		~UsbPort() override;

		UsbPort(const UsbPort&) = delete;
		UsbPort& operator=(const UsbPort&) = delete;

		bool open(Receiver receiver) override;
		void close() override;
		bool write(size_t channel, std::string_view bytes) override;
		size_t channels() const override { return m_channelCount; }
		bool needsChecksums() const override { return false; }
		bool failed() const override { return m_failed.load(); }
		std::vector<std::string> openingGcode() const override;

	  private:
		/** One data channel: a CDC data interface's bulk endpoints, and its control interface */
		struct Channel
		{
			uint8_t dataInterface = 0xFF;	 // Its data interface
			uint8_t controlInterface = 0xFF; // Its control interface (line coding, DTR)
			uint8_t inEndpoint = 0;			 // Bulk IN
			uint8_t outEndpoint = 0;		 // Bulk OUT
		};

		/** How big each read is */
		static constexpr size_t readSize = 4096;

		/** How many reads are kept waiting on each channel, so the Duet always has one to fill */
		static constexpr size_t readsPerChannel = 4;

		/** One read kept waiting on a channel */
		struct Read
		{
			UsbPort* port = nullptr;					  // The port it reads for
			size_t channel = 0;							  // The channel it reads
			libusb_transfer* transfer = nullptr;		  // The transfer, while it is waiting
			std::array<unsigned char, readSize> buffer{}; // What it reads into
		};

		/** Find a Duet among the USB devices; nullptr if there is none (the caller unrefs what it returns) */
		static libusb_device* findDuet();

		/** Choose up to maxChannels data channels from the device's descriptors */
		bool chooseChannels(libusb_device* device);

		/** Claim the channels' interfaces, detaching the kernel's driver from them */
		bool claimInterfaces();

		/** Send a CDC class request (SET_LINE_CODING, SET_CONTROL_LINE_STATE) to each channel's control interface */
		bool controlRequest(uint8_t request, uint16_t value, std::span<unsigned char> data);

		/** Set the lines 115200 8N1 */
		bool setLineCoding();

		/** Raise (true) or drop (false) DTR: the Duet sees the port open or closed */
		bool setDtr(bool on);

		/** Start the event thread and the reads */
		bool startReading();

		/** Cancel the reads and wait for them, then stop the event thread */
		void stopReading();

		/** Submit `read`; false if it couldn't be */
		bool submit(Read& read);

		/** A read completed (libusb's event thread) */
		static void LIBUSB_CALL readDone(libusb_transfer* transfer);

		/** The event thread: libusb's events until stopped */
		void handleEvents();

		bool m_useSecondChannel;											  // Whether a second channel is used
		Receiver m_receiver;												  // Given what is read
		libusb_device_handle* m_handle = nullptr;							  // The open device
		std::array<Channel, maxChannels> m_channels{};						  // The channels used
		size_t m_channelCount = 0;											  // How many of m_channels are used
		std::array<std::array<Read, readsPerChannel>, maxChannels> m_reads{}; // The reads kept waiting
		std::atomic<size_t> m_readsWaiting{0};								  // Reads submitted and not yet completed
		std::atomic<bool> m_reading{false};									  // Reads that complete are submitted again
		std::atomic<bool> m_handlingEvents{false};							  // The event thread carries on
		std::atomic<bool> m_failed{false};									  // A transfer failed: the Duet has gone
		std::thread m_events;												  // Handles libusb's events
	};
} // namespace Comm

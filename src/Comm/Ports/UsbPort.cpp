/*
 * UsbPort.cpp
 */

#include "UsbPort.h"
#include "Comm/UsbMode.h"
#include "Debug.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <chrono>
#include <map>
#include <string_view>

namespace Comm
{
	namespace
	{
		/** The Duets (and PanelDue-like CDC devices) a UsbPort talks to, by vendor and product id */
		struct KnownDevice
		{
			uint16_t vendor;	   // USB vendor id
			uint16_t product;	   // USB product id
			std::string_view name; // What it is, for the log
		};

		/** Every device a UsbPort talks to */
		constexpr std::array<KnownDevice, 5> knownDevices = {{
			{0x1d50, 0x60ec, "Duet 2"},
			{0x1d50, 0x60ed, "Duet 2 Maestro"},
			{0x1d50, 0x60ee, "Duet 3"},
			{0x1d50, 0x60ef, "Duet"},
			{0x16c0, 0x27dd, "CDC-ACM Device"},
		}};

		/** How long the port is held closed (DTR low) on opening, so the Duet notices it closed */
		constexpr std::chrono::milliseconds portClosedTime{100};

		/** How long a write or control request may take, ms */
		constexpr unsigned int transferTimeoutMs = 1000;

		/** How long closing waits for the reads to be cancelled */
		constexpr std::chrono::milliseconds cancelWait{2000};

		/** CDC class requests */
		constexpr uint8_t setLineCodingRequest = 0x20;
		constexpr uint8_t setControlLineStateRequest = 0x22;

		/** A transfer's status, for the log */
		std::string_view statusName(libusb_transfer_status status)
		{
			switch (status)
			{
			case LIBUSB_TRANSFER_COMPLETED:
				return "completed";
			case LIBUSB_TRANSFER_ERROR:
				return "error";
			case LIBUSB_TRANSFER_TIMED_OUT:
				return "timed out";
			case LIBUSB_TRANSFER_CANCELLED:
				return "cancelled";
			case LIBUSB_TRANSFER_STALL:
				return "stalled";
			case LIBUSB_TRANSFER_NO_DEVICE:
				return "no device";
			case LIBUSB_TRANSFER_OVERFLOW:
				return "overflow";
			}
			return "unknown";
		}
	} // namespace

	UsbPort::UsbPort(bool useSecondChannel)
		: m_useSecondChannel(useSecondChannel)
	{
	}

	UsbPort::~UsbPort()
	{
		close();
	}

	std::vector<std::string> UsbPort::openingGcode() const
	{
		// JSON replies, without checksums (USB has its own error checking), on each channel used
		std::vector<std::string> gcode = {"M575 P0 S0"};
		if (m_channelCount > 1)
		{
			gcode.emplace_back("M575 P1 S0");
		}
		return gcode;
	}

	bool UsbPort::open(Receiver receiver)
	{
		ZoneScoped;
		close();
		if (usbContext() == nullptr)
		{
			LOG_ERROR("libusb isn't running");
			return false;
		}
		libusb_device* device = findDuet();
		if (device == nullptr)
		{
			LOG_WARN("No Duet on the USB port");
			return false;
		}
		const bool chosen = chooseChannels(device);
		const int opened = chosen ? libusb_open(device, &m_handle) : LIBUSB_ERROR_NOT_FOUND;
		libusb_unref_device(device);
		if (!chosen || opened != LIBUSB_SUCCESS)
		{
			LOG_ERROR("Couldn't open the Duet: {:s}", libusb_error_name(opened));
			m_handle = nullptr;
			return false;
		}

		m_receiver = std::move(receiver);
		m_failed = false;
		// The port may not have been closed last time (the screen restarted): close it first, so the Duet throws away
		// any line it has only partly received rather than joining the first line sent now onto it. RepRapFirmware
		// notices the port closed when it next reads it, so it stays closed for a while
		if (!claimInterfaces() || !setLineCoding() || !setDtr(false))
		{
			close();
			return false;
		}
		std::this_thread::sleep_for(portClosedTime);
		if (!setDtr(true) || !startReading())
		{
			close();
			return false;
		}
		LOG_INFO("Opened the Duet on USB: {:d} channel{:s}", m_channelCount, m_channelCount > 1 ? "s" : "");
		return true;
	}

	void UsbPort::close()
	{
		ZoneScoped;
		if (m_handle == nullptr)
		{
			return;
		}
		stopReading();
		setDtr(false); // closing the port: the Duet throws away any line it has only partly received
		for (size_t i = 0; i < m_channelCount; i++)
		{
			for (const uint8_t interface : {m_channels[i].dataInterface, m_channels[i].controlInterface})
			{
				if (interface != 0xFF)
				{
					libusb_release_interface(m_handle, interface);
				}
			}
		}
		libusb_close(m_handle);
		m_handle = nullptr;
		m_channelCount = 0;
		m_receiver = nullptr;
	}

	bool UsbPort::write(size_t channel, std::string_view bytes)
	{
		ZoneScoped;
		ZoneValue(bytes.size());
		if (m_handle == nullptr || channel >= m_channelCount)
		{
			return false;
		}
		int sent = 0;
		// libusb takes a non-const buffer for both directions; it only reads it for an OUT transfer
		std::string copy(bytes);
		const int result = libusb_bulk_transfer(m_handle,
												m_channels[channel].outEndpoint,
												reinterpret_cast<unsigned char*>(copy.data()),
												static_cast<int>(copy.size()),
												&sent,
												transferTimeoutMs);
		if (result != LIBUSB_SUCCESS || sent != static_cast<int>(copy.size()))
		{
			LOG_ERROR(
				"Writing to the Duet failed: {:s} ({:d} of {:d} bytes)", libusb_error_name(result), sent, copy.size());
			m_failed = true;
			return false;
		}
		return true;
	}

	libusb_device* UsbPort::findDuet()
	{
		ZoneScoped;
		libusb_device** list = nullptr;
		const ssize_t count = libusb_get_device_list(usbContext(), &list);
		if (count < 0)
		{
			LOG_ERROR("Couldn't list the USB devices: {:s}", libusb_error_name(static_cast<int>(count)));
			return nullptr;
		}
		libusb_device* found = nullptr;
		for (ssize_t i = 0; i < count && found == nullptr; i++)
		{
			libusb_device_descriptor descriptor;
			if (libusb_get_device_descriptor(list[i], &descriptor) != LIBUSB_SUCCESS)
			{
				continue;
			}
			for (const KnownDevice& known : knownDevices)
			{
				if (descriptor.idVendor == known.vendor && descriptor.idProduct == known.product)
				{
					LOG_INFO("Found a {:s} on USB ({:04x}:{:04x})", known.name, known.vendor, known.product);
					found = libusb_ref_device(list[i]);
					break;
				}
			}
		}
		libusb_free_device_list(list, 1);
		return found;
	}

	bool UsbPort::chooseChannels(libusb_device* device)
	{
		ZoneScoped;
		libusb_config_descriptor* config = nullptr;
		if (const int result = libusb_get_active_config_descriptor(device, &config); result != LIBUSB_SUCCESS)
		{
			LOG_ERROR("Couldn't read the Duet's configuration: {:s}", libusb_error_name(result));
			return false;
		}

		// A CDC control interface names its data interfaces in its union functional descriptor (0x24, 0x06)
		std::map<uint8_t, uint8_t> controlOf;
		for (int i = 0; i < config->bNumInterfaces; i++)
		{
			for (int j = 0; j < config->interface[i].num_altsetting; j++)
			{
				const libusb_interface_descriptor& setting = config->interface[i].altsetting[j];
				if (setting.bInterfaceClass != LIBUSB_CLASS_COMM)
				{
					continue;
				}
				const std::span<const unsigned char> extra(setting.extra, static_cast<size_t>(setting.extra_length));
				for (size_t k = 0; k + 2 < extra.size() && extra[k] >= 3 && k + extra[k] <= extra.size(); k += extra[k])
				{
					if (extra[k + 1] == 0x24 && extra[k + 2] == 0x06)
					{
						for (size_t member = k + 4; member < k + extra[k]; member++)
						{
							controlOf.emplace(extra[member], setting.bInterfaceNumber);
						}
					}
				}
			}
		}

		// Data channels: interfaces with a bulk IN and a bulk OUT endpoint, best first (CDC data class, with a
		// control interface, bigger packets, lower interface numbers)
		struct Candidate
		{
			Channel channel; // The channel it would be
			int score = 0;	 // How good a choice it is
		};
		std::vector<Candidate> candidates;
		for (int i = 0; i < config->bNumInterfaces; i++)
		{
			for (int j = 0; j < config->interface[i].num_altsetting; j++)
			{
				const libusb_interface_descriptor& setting = config->interface[i].altsetting[j];
				Candidate candidate;
				uint16_t packetSize = 0;
				for (int k = 0; k < setting.bNumEndpoints; k++)
				{
					const libusb_endpoint_descriptor& endpoint = setting.endpoint[k];
					if ((endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK)
					{
						continue;
					}
					if (endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN)
					{
						candidate.channel.inEndpoint = endpoint.bEndpointAddress;
						packetSize = endpoint.wMaxPacketSize;
					}
					else
					{
						candidate.channel.outEndpoint = endpoint.bEndpointAddress;
					}
				}
				if (candidate.channel.inEndpoint == 0 || candidate.channel.outEndpoint == 0)
				{
					continue;
				}
				candidate.channel.dataInterface = setting.bInterfaceNumber;
				const auto control = controlOf.find(setting.bInterfaceNumber);
				candidate.channel.controlInterface =
					control != controlOf.end() ? control->second
											   : static_cast<uint8_t>(std::max(0, setting.bInterfaceNumber - 1));
				candidate.score = (setting.bInterfaceClass == LIBUSB_CLASS_DATA ? 1000 : 0) +
								  (control != controlOf.end() ? 500 : 0) + packetSize - setting.bInterfaceNumber;
				candidates.push_back(candidate);
			}
		}
		libusb_free_config_descriptor(config);

		std::ranges::stable_sort(candidates, std::greater<>(), &Candidate::score);
		const size_t wanted = m_useSecondChannel ? maxChannels : 1;
		m_channelCount = 0;
		for (const Candidate& candidate : candidates)
		{
			const bool taken = std::ranges::any_of(
				std::span(m_channels.data(), m_channelCount),
				[&](const Channel& channel) { return channel.dataInterface == candidate.channel.dataInterface; });
			if (!taken && m_channelCount < wanted)
			{
				m_channels[m_channelCount++] = candidate.channel;
			}
		}
		if (m_channelCount == 0)
		{
			LOG_ERROR("The Duet has no data channel");
			return false;
		}
		return true;
	}

	bool UsbPort::claimInterfaces()
	{
		ZoneScoped;
		for (size_t i = 0; i < m_channelCount; i++)
		{
			for (const uint8_t interface : {m_channels[i].dataInterface, m_channels[i].controlInterface})
			{
				if (libusb_kernel_driver_active(m_handle, interface) == 1)
				{
					libusb_detach_kernel_driver(m_handle, interface);
				}
				// Claiming an interface twice (two channels sharing a control interface) succeeds
				if (const int result = libusb_claim_interface(m_handle, interface); result != LIBUSB_SUCCESS)
				{
					LOG_ERROR("Couldn't claim the Duet's interface {:d}: {:s}", interface, libusb_error_name(result));
					return false;
				}
			}
		}
		return true;
	}

	bool UsbPort::controlRequest(uint8_t request, uint16_t value, std::span<unsigned char> data)
	{
		ZoneScoped;
		const uint8_t type = static_cast<uint8_t>(LIBUSB_ENDPOINT_OUT) |
							 static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
							 static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE);
		bool ok = true;
		for (size_t i = 0; i < m_channelCount; i++)
		{
			const uint8_t interface = m_channels[i].controlInterface;
			// Once per control interface: channels may share one
			if (std::ranges::any_of(std::span(m_channels.data(), i),
									[interface](const Channel& channel)
									{ return channel.controlInterface == interface; }))
			{
				continue;
			}
			const int result = libusb_control_transfer(m_handle,
													   type,
													   request,
													   value,
													   interface,
													   data.data(),
													   static_cast<uint16_t>(data.size()),
													   transferTimeoutMs);
			if (result < 0)
			{
				LOG_ERROR("A request to the Duet's interface {:d} failed: {:s}", interface, libusb_error_name(result));
				ok = false;
			}
		}
		return ok;
	}

	bool UsbPort::setLineCoding()
	{
		// 115200 baud (little endian), 1 stop bit, no parity, 8 data bits
		std::array<unsigned char, 7> coding = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8};
		return controlRequest(setLineCodingRequest, 0, coding);
	}

	bool UsbPort::setDtr(bool on)
	{
		return controlRequest(setControlLineStateRequest, on ? 1 : 0, {});
	}

	bool UsbPort::startReading()
	{
		ZoneScoped;
		m_handlingEvents = true;
		m_events = std::thread(&UsbPort::handleEvents, this);
		m_reading = true;
		for (size_t channel = 0; channel < m_channelCount; channel++)
		{
			for (Read& read : m_reads[channel])
			{
				read.port = this;
				read.channel = channel;
				if (!submit(read))
				{
					return false;
				}
			}
		}
		return true;
	}

	void UsbPort::stopReading()
	{
		ZoneScoped;
		m_reading = false;
		for (auto& channel : m_reads)
		{
			for (Read& read : channel)
			{
				if (read.transfer != nullptr)
				{
					libusb_cancel_transfer(read.transfer);
				}
			}
		}
		// The event thread completes the cancelled reads; a read can't be freed while it is waiting
		const auto giveUp = std::chrono::steady_clock::now() + cancelWait;
		while (m_readsWaiting > 0 && m_handlingEvents && std::chrono::steady_clock::now() < giveUp)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		if (m_readsWaiting > 0)
		{
			LOG_ERROR("{:d} USB reads weren't cancelled; they are left", m_readsWaiting.load());
		}
		m_handlingEvents = false;
		if (m_events.joinable())
		{
			m_events.join();
		}
		m_readsWaiting = 0;
	}

	bool UsbPort::submit(Read& read)
	{
		if (read.transfer == nullptr)
		{
			read.transfer = libusb_alloc_transfer(0);
			if (read.transfer == nullptr)
			{
				LOG_ERROR("Couldn't make a USB transfer");
				return false;
			}
		}
		libusb_fill_bulk_transfer(read.transfer,
								  m_handle,
								  m_channels[read.channel].inEndpoint,
								  read.buffer.data(),
								  static_cast<int>(read.buffer.size()),
								  readDone,
								  &read,
								  0);
		if (const int result = libusb_submit_transfer(read.transfer); result != LIBUSB_SUCCESS)
		{
			LOG_ERROR("Couldn't read from the Duet: {:s}", libusb_error_name(result));
			libusb_free_transfer(read.transfer);
			read.transfer = nullptr;
			if (result == LIBUSB_ERROR_NO_DEVICE)
			{
				m_failed = true;
			}
			return false;
		}
		m_readsWaiting++;
		return true;
	}

	void LIBUSB_CALL UsbPort::readDone(libusb_transfer* transfer)
	{
		ZoneScoped;
		Read& read = *static_cast<Read*>(transfer->user_data);
		UsbPort& port = *read.port;
		port.m_readsWaiting--;
		switch (transfer->status)
		{
		case LIBUSB_TRANSFER_COMPLETED:
			if (transfer->actual_length > 0 && port.m_receiver)
			{
				ZoneValue(transfer->actual_length);
				port.m_receiver(read.channel,
								std::string_view(reinterpret_cast<const char*>(transfer->buffer),
												 static_cast<size_t>(transfer->actual_length)));
			}
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
		case LIBUSB_TRANSFER_CANCELLED:
			break;
		default:
			// The Duet has gone (unplugged, restarted) or the line is broken: the connection starts again
			LOG_WARN("Reading from the Duet: {:s}", statusName(transfer->status));
			port.m_failed = true;
			break;
		}
		const bool again = port.m_reading && !port.m_failed && transfer->status != LIBUSB_TRANSFER_CANCELLED;
		if (!again || !port.submit(read))
		{
			libusb_free_transfer(read.transfer);
			read.transfer = nullptr;
		}
	}

	void UsbPort::handleEvents()
	{
		tracy::SetThreadName("USB");
		timeval timeout = {0, 100'000};
		while (m_handlingEvents)
		{
			libusb_handle_events_timeout_completed(usbContext(), &timeout, nullptr);
		}
	}
} // namespace Comm

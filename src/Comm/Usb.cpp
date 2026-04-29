#include "Usb.h"
#include "Comm/JsonDecoder.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Pins.h"
#include "Storage.h"
#include "nameof.hpp"
#include "tracy/Tracy.hpp"
#include "utils/GpioHelper.h"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

namespace Comm
{
	static void setUsbHost(bool host);
	static void setUsbMux(bool usbc);
	static void setUsbState(bool state);

	static UsbDevice s_currentUsbDevice;
	static libusb_context* s_context = nullptr;
	static UsbMode s_usbMode = UsbMode::Unknown;

	using vendor_id_t = uint16_t;
	using product_id_t = uint16_t;

	static const std::unordered_map<vendor_id_t, std::unordered_map<product_id_t, std::string_view>> s_devices = {
		{0x1d50, {{0x60ec, "Duet 2"}, {0x60ed, "Duet 2 Maestro"}, {0x60ee, "Duet 3"}, {0x60ef, "Duet"}}},
		{0x16c0, {{0x27dd, "CDC-ACM Device"}}}};

	static TracyLockable(std::recursive_mutex, s_usbMutex);
	static TracyLockable(std::mutex, s_transferMutex);
	static std::condition_variable s_completionCondition;

	static constexpr int32_t s_usbTimeoutMs = 1000;

	UsbDevice::UsbDevice()
		: m_name("")
		, m_device(nullptr)
		, m_handle(nullptr)
		, m_channels{}
		, m_channelCount(0)
		, m_claimedInterfaceCount(0)
		, m_claimedInterfaces{}
		, m_receiveContexts{}
	{
		ZoneScoped;
		m_claimedInterfaces.fill(0xFF);
	}

	UsbDevice::~UsbDevice()
	{
		ZoneScoped;
		reset();
	}

	bool UsbDevice::init(const char* name, libusb_device* device, receive_cb_t callback)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		m_name = name;
		m_device = device;
		m_receiveCallback = callback;
		if (!getDeviceInterface())
		{
			LOG_ERROR("Failed to get device interface");
			return false;
		}
		return true;
	}

	void UsbDevice::reset()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		LOG_DBG("Resetting USB device {:s}", m_name);
		if (m_handle)
		{
			// Stop event handling thread before releasing resources
			m_eventThreadRunning = false;
			if (m_eventLoopThread.joinable())
			{
				m_eventLoopThread.join();
			}

			for (std::size_t i = 0; i < m_claimedInterfaceCount; ++i)
			{
				if (m_claimedInterfaces[i] == 0xFF)
				{
					continue;
				}
				LOG_DBG("Releasing interface {}", m_claimedInterfaces[i]);
				libusb_release_interface(m_handle, m_claimedInterfaces[i]);
			}
			libusb_close(m_handle);
			m_handle = nullptr;
		}
		if (m_device)
		{
			LOG_DBG("Unref device");
			// libusb_unref_device(m_device);
			m_device = nullptr;
		}
		m_name = "";
		for (auto& channel : m_channels)
		{
			channel = ChannelConfig{};
		}
		m_channelCount = 0;
		m_claimedInterfaceCount = 0;
		m_claimedInterfaces.fill(0xFF);
	}

	bool UsbDevice::connect()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		int r;

		if (!m_device)
		{
			LOG_ERROR("No USB device");
			return false;
		}

		r = libusb_open(m_device, &m_handle);

		if (!m_handle)
		{
			LOG_ERROR("Cannot open device: {:s} [{:s}]", libusb_strerror(r), libusb_error_name(r));
			return false;
		}

		auto detachAndClaimInterface = [&](uint8_t interfaceNumber) -> bool
		{
			if (interfaceNumber == 0xFF)
			{
				return true;
			}

			for (std::size_t i = 0; i < m_claimedInterfaceCount; ++i)
			{
				if (m_claimedInterfaces[i] == interfaceNumber)
				{
					return true;
				}
			}

			if (m_claimedInterfaceCount >= std::size(m_claimedInterfaces))
			{
				LOG_ERROR("Too many USB interfaces to claim");
				return false;
			}

			if (libusb_kernel_driver_active(m_handle, interfaceNumber) == 1)
			{
				r = libusb_detach_kernel_driver(m_handle, interfaceNumber);
				if (r < 0)
				{
					LOG_ERROR(
						"Cannot detach kernel driver for interface {}: {:s}", interfaceNumber, libusb_error_name(r));
					return false;
				}
			}

			r = libusb_claim_interface(m_handle, interfaceNumber);
			if (r < 0)
			{
				LOG_ERROR("Cannot claim interface {}: {:s}", interfaceNumber, libusb_error_name(r));
				return false;
			}

			m_claimedInterfaces[m_claimedInterfaceCount++] = interfaceNumber;
			return true;
		};

		for (std::size_t channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
		{
			if (!detachAndClaimInterface(m_channels[channelIndex].dataInterfaceNumber))
			{
				goto close_handle;
			}

			if (!detachAndClaimInterface(m_channels[channelIndex].controlInterfaceNumber))
			{
				goto close_handle;
			}
		}

		if (!setBaud(115200))
		{
			LOG_ERROR("Failed to set CDC line coding");
			goto close_handle;
		}

		r = setDtr(true);
		if (r < 0)
		{
			LOG_ERROR("Failed to set DTR on all channels");
			goto close_handle;
		}

		m_eventThreadRunning = true;
		m_eventLoopThread = std::thread(&UsbDevice::eventLoop, this);
		for (std::size_t channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
		{
			if (receive(channelIndex, s_usbTimeoutMs) != receive_err_t::NONE)
			{
				LOG_ERROR("Failed to start receive transfer for channel {}", channelIndex);
				goto close_handle;
			}
		}

		return true;

	close_handle:
		LOG_DBG("Closing device");
		libusb_close(m_handle);
		m_handle = nullptr;
		return false;
	}

	bool UsbDevice::send(std::string_view data, std::size_t channelIndex, unsigned int timeoutMs)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		if (!m_handle)
		{
			LOG_WARN("No USB device handle");
			return false;
		}

		if (m_channelCount == 0)
		{
			LOG_WARN("No USB data channel available");
			return false;
		}

		const std::size_t resolvedChannelIndex = m_channelCount == 1 ? 0 : channelIndex;
		if (resolvedChannelIndex >= m_channelCount)
		{
			LOG_WARN("Invalid USB channel {} requested (available: {})", resolvedChannelIndex, m_channelCount);
			return false;
		}

		struct libusb_transfer* transfer = libusb_alloc_transfer(0);
		if (!transfer)
		{
			LOG_ERROR("Failed to allocate transfer");
			return false;
		}

		LOG_DBG("Submitting USB transfer on channel {:d}, {:d} bytes: '{:s}'", resolvedChannelIndex, data.size(), data);

		TransferData* transferData = new TransferData();
		transferData->buffer.assign(data.begin(), data.end());

		// Fill bulk transfer structure
		libusb_fill_bulk_transfer(transfer,
								  m_handle,
								  m_channels[resolvedChannelIndex].outEndpoint,
								  transferData->buffer.data(),
								  static_cast<int>(transferData->buffer.size()),
								  sendTransferCallback,
								  transferData,
								  timeoutMs);

		int r = libusb_submit_transfer(transfer);
		if (r < 0)
		{
			LOG_ERROR("Failed to submit transfer: {:s}", libusb_error_name(r));
			libusb_free_transfer(transfer);
			delete transferData;
			if (r == LIBUSB_ERROR_NO_DEVICE)
			{
				Reconnect();
			}
			return false;
		}

		return true;
	}

	// Convenience wrapper: set baud as 8N1 on CDC-ACM
	bool UsbDevice::setBaud(uint32_t baud)
	{
		ZoneScoped;
		return setLineCoding(baud, /*stopBits*/ 0, /*parity*/ 0, /*dataBits*/ 8);
	}

	// CDC-ACM SET_LINE_CODING to configure baud/format
	bool UsbDevice::setLineCoding(uint32_t baud, uint8_t stopBits, uint8_t parity, uint8_t dataBits)
	{
		ZoneScoped;
		static constexpr uint8_t s_invalidInterface = 0xFF;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		if (!m_handle)
		{
			LOG_WARN("No USB device handle");
			return false;
		}

#pragma pack(push, 1)
		struct LineCoding
		{
			uint32_t dwDTERate;	 // Baud rate in bps (LE)
			uint8_t bCharFormat; // Stop bits: 0=1,1=1.5,2=2
			uint8_t bParityType; // 0=None,1=Odd,2=Even,3=Mark,4=Space
			uint8_t bDataBits;	 // Data bits: typically 5..8
		};
#pragma pack(pop)

		LineCoding lc{baud, stopBits, parity, dataBits};

		uint8_t bmRequestType = static_cast<uint8_t>(LIBUSB_ENDPOINT_OUT) |
								static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
								static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE);
		uint8_t bRequest = 0x20; // SET_LINE_CODING
		uint16_t wValue = 0;
		if (m_channelCount == 0)
		{
			LOG_WARN("No USB channel available for line coding");
			return false;
		}

		std::array<uint16_t, s_maxChannelCount> configuredInterfaces{};
		configuredInterfaces.fill(s_invalidInterface);
		std::size_t configuredCount = 0;
		bool success = true;
		unsigned int timeoutMs = 1000;

		for (std::size_t channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
		{
			const ChannelConfig& channel = m_channels[channelIndex];
			const uint16_t wIndex = channel.controlInterfaceNumber != s_invalidInterface
										? channel.controlInterfaceNumber
										: channel.dataInterfaceNumber;

			// Multiple data channels can reference the same control interface.
			// Send class-control requests once per unique interface index.
			bool alreadyConfigured = false;
			for (std::size_t i = 0; i < configuredCount; ++i)
			{
				if (configuredInterfaces[i] == wIndex)
				{
					alreadyConfigured = true;
					break;
				}
			}
			if (alreadyConfigured)
			{
				continue;
			}

			const int err = libusb_control_transfer(m_handle,
													bmRequestType,
													bRequest,
													wValue,
													wIndex,
													reinterpret_cast<unsigned char*>(&lc),
													static_cast<uint16_t>(sizeof(lc)),
													timeoutMs);
			if (err < 0)
			{
				LOG_ERROR("Failed to set line coding for channel {} (if={}): {:s}",
						  channelIndex,
						  wIndex,
						  libusb_error_name(err));
				success = false;
				continue;
			}

			configuredInterfaces[configuredCount++] = wIndex;
			LOG_DBG("CDC line coding set for channel {} (if={}): {} bps, {} stop, parity {}, {} bits",
					channelIndex,
					wIndex,
					baud,
					stopBits,
					parity,
					dataBits);
		}

		return success;
	}

	UsbDevice::receive_err_t UsbDevice::receive(std::size_t channelIndex, unsigned int timeoutMs)
	{
		ZoneScoped;
		if (!m_handle)
		{
			LOG_WARN("No USB device handle");
			return receive_err_t::NO_DEVICE;
		}

		struct libusb_transfer* transfer = libusb_alloc_transfer(0);
		if (!transfer)
		{
			LOG_ERROR("Failed to allocate transfer");
			return receive_err_t::FAILED_TO_ALLOCATE_TRANSFER;
		}

		if (m_channelCount == 0 || channelIndex >= m_channelCount)
		{
			libusb_free_transfer(transfer);
			LOG_ERROR("Invalid USB receive channel {}", channelIndex);
			return receive_err_t::OTHER_ERROR;
		}

		ReceiveTransferContext* receiveContext = &m_receiveContexts[channelIndex];
		receiveContext->device = this;
		receiveContext->channelIndex = channelIndex;

		libusb_fill_bulk_transfer(transfer,
								  m_handle,
								  m_channels[channelIndex].inEndpoint,
								  m_receiveBuffers[channelIndex],
								  s_receiveBufferSize,
								  receiveTransferCallback,
								  receiveContext,
								  timeoutMs);

		int r = libusb_submit_transfer(transfer);
		if (r < 0)
		{
			LOG_ERROR("Failed to submit transfer: {:s}", libusb_error_name(r));
			libusb_free_transfer(transfer);
			return receive_err_t::FAILED_TO_SUBMIT_TRANSFER;
		}

		return receive_err_t::NONE;
	}

	int UsbDevice::setDtr(bool state)
	{
		ZoneScoped;
		static constexpr uint8_t s_invalidInterface = 0xFF;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		if (!m_handle)
		{
			LOG_WARN("No USB device handle");
			return -1;
		}
		uint8_t request_type =
			static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) | static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE);
		uint8_t request = 0x22;				  // SET_CONTROL_LINE_STATE (commonly used for DTR/RTS)
		uint16_t value = state ? 0x01 : 0x00; // DTR set high (bit 0)
		if (m_channelCount == 0)
		{
			LOG_WARN("No USB channel available for DTR control");
			return -1;
		}

		std::array<uint16_t, s_maxChannelCount> configuredInterfaces{};
		configuredInterfaces.fill(s_invalidInterface);
		std::size_t configuredCount = 0;
		int firstError = 0;

		for (std::size_t channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
		{
			const ChannelConfig& channel = m_channels[channelIndex];
			const uint16_t index = channel.controlInterfaceNumber != s_invalidInterface ? channel.controlInterfaceNumber
																						: channel.dataInterfaceNumber;

			// Multiple data channels can reference the same control interface.
			// Send class-control requests once per unique interface index.
			bool alreadyConfigured = false;
			for (std::size_t i = 0; i < configuredCount; ++i)
			{
				if (configuredInterfaces[i] == index)
				{
					alreadyConfigured = true;
					break;
				}
			}
			if (alreadyConfigured)
			{
				continue;
			}

			const int err = libusb_control_transfer(m_handle, request_type, request, value, index, nullptr, 0, 1000);
			if (err < 0)
			{
				LOG_ERROR(
					"Failed to set DTR for channel {} (if={}): {:s}", channelIndex, index, libusb_error_name(err));
				if (firstError == 0)
				{
					firstError = err;
				}
				continue;
			}

			configuredInterfaces[configuredCount++] = index;
			LOG_DBG("DTR set for channel {} (if={})", channelIndex, index);
		}

		return firstError;
	}

	bool UsbDevice::getDeviceInterface()
	{
		ZoneScoped;
		static constexpr uint8_t s_invalidInterface = 0xFF;
		static constexpr int s_dataClassScore = 1000;
		static constexpr int s_controlInterfaceScore = 500;

		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		libusb_config_descriptor* config_desc;
		int configResult = libusb_get_active_config_descriptor(m_device, &config_desc);
		if (configResult < 0)
		{
			LOG_ERROR("Failed to get active config descriptor: {:s}", libusb_error_name(configResult));
			return false;
		}

		struct Candidate
		{
			uint8_t dataInterface = s_invalidInterface;
			uint8_t controlInterface = s_invalidInterface;
			uint8_t inEndpoint = 0;
			uint8_t outEndpoint = 0;
			uint16_t packetSize = 0;
			uint8_t interfaceClass = 0;
		};

		std::unordered_map<uint8_t, uint8_t> controlInterfaceByDataInterface;
		std::vector<Candidate> candidates;
		candidates.reserve(static_cast<std::size_t>(config_desc->bNumInterfaces));

		for (int i = 0; i < config_desc->bNumInterfaces; i++)
		{
			const libusb_interface& interface = config_desc->interface[i];
			for (int j = 0; j < interface.num_altsetting; j++)
			{
				const libusb_interface_descriptor& altsetting = interface.altsetting[j];
				if (altsetting.bInterfaceClass != LIBUSB_CLASS_COMM)
				{
					continue;
				}
				for (int k = 0; k < altsetting.extra_length - 4; ++k)
				{
					const uint8_t* extra = reinterpret_cast<const uint8_t*>(altsetting.extra + k);
					if (extra[0] < 5 || (k + extra[0]) > altsetting.extra_length)
					{
						continue;
					}
					if (extra[1] == 0x24 && extra[2] == 0x06)
					{
						const uint8_t controlInterface = altsetting.bInterfaceNumber;
						for (int slaveIndex = 4; slaveIndex < extra[0]; ++slaveIndex)
						{
							controlInterfaceByDataInterface.emplace(extra[slaveIndex], controlInterface);
						}
					}
					k += static_cast<int>(extra[0]) - 1;
				}
			}
		}

		for (int i = 0; i < config_desc->bNumInterfaces; i++)
		{
			const libusb_interface& interface = config_desc->interface[i];
			for (int j = 0; j < interface.num_altsetting; j++)
			{
				const libusb_interface_descriptor& altsetting = interface.altsetting[j];

				uint8_t inEndpoint = 0;
				uint8_t outEndpoint = 0;
				uint16_t packetSize = 0;
				for (int k = 0; k < altsetting.bNumEndpoints; k++)
				{
					const libusb_endpoint_descriptor& ep_desc = altsetting.endpoint[k];
					if ((ep_desc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
					{
						if (ep_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN)
						{
							inEndpoint = ep_desc.bEndpointAddress;
							packetSize = ep_desc.wMaxPacketSize;
						}
						else
						{
							outEndpoint = ep_desc.bEndpointAddress;
						}
					}
				}
				if (inEndpoint != 0 && outEndpoint != 0)
				{
					Candidate candidate{};
					candidate.dataInterface = altsetting.bInterfaceNumber;
					candidate.inEndpoint = inEndpoint;
					candidate.outEndpoint = outEndpoint;
					candidate.packetSize = packetSize;
					candidate.interfaceClass = altsetting.bInterfaceClass;

					auto controlInterfaceIt = controlInterfaceByDataInterface.find(candidate.dataInterface);
					if (controlInterfaceIt != controlInterfaceByDataInterface.end())
					{
						candidate.controlInterface = controlInterfaceIt->second;
					}

					candidates.push_back(std::move(candidate));
				}
			}
		}

		if (candidates.empty())
		{
			libusb_free_config_descriptor(config_desc);
			LOG_ERROR("Failed to find a valid USB bulk IN/OUT endpoint pair");
			return false;
		}

		auto scoreCandidate = [](const Candidate& candidate) -> int
		{
			int score = 0;
			if (candidate.interfaceClass == LIBUSB_CLASS_DATA)
			{
				score += s_dataClassScore;
			}
			if (candidate.controlInterface != s_invalidInterface)
			{
				score += s_controlInterfaceScore;
			}
			score += candidate.packetSize;
			score -= candidate.dataInterface;
			return score;
		};

		std::sort(candidates.begin(),
				  candidates.end(),
				  [&](const Candidate& lhs, const Candidate& rhs)
				  {
					  const int lhsScore = scoreCandidate(lhs);
					  const int rhsScore = scoreCandidate(rhs);
					  if (lhsScore != rhsScore)
					  {
						  return lhsScore > rhsScore;
					  }
					  return lhs.dataInterface < rhs.dataInterface;
				  });

		m_channelCount = 0;
		for (auto& channel : m_channels)
		{
			channel = ChannelConfig{};
		}

		for (const Candidate& candidate : candidates)
		{
			if (m_channelCount >= s_maxChannelCount)
			{
				break;
			}

			// Multiple altsettings can describe the same data interface. Keep only
			// the highest-ranked candidate for each data interface number.
			bool alreadySelected = false;
			for (std::size_t channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
			{
				if (m_channels[channelIndex].dataInterfaceNumber == candidate.dataInterface)
				{
					alreadySelected = true;
					break;
				}
			}

			if (alreadySelected)
			{
				continue;
			}

			ChannelConfig& channel = m_channels[m_channelCount];
			channel.dataInterfaceNumber = candidate.dataInterface;
			channel.controlInterfaceNumber = candidate.controlInterface;
			channel.inEndpoint = candidate.inEndpoint;
			channel.outEndpoint = candidate.outEndpoint;
			channel.packetSize = candidate.packetSize;

			if (channel.controlInterfaceNumber == s_invalidInterface)
			{
				channel.controlInterfaceNumber = channel.dataInterfaceNumber > 0
													 ? static_cast<uint8_t>(channel.dataInterfaceNumber - 1)
													 : channel.dataInterfaceNumber;
			}

			LOG_DBG("Selected USB channel {} on interface {} (score {}, class {})",
					m_channelCount,
					channel.dataInterfaceNumber,
					scoreCandidate(candidate),
					candidate.interfaceClass);
			LOG_DBG("Using channel {} data interface {}, IN endpoint {:#x}, OUT endpoint {:#x}, packet {}",
					m_channelCount,
					channel.dataInterfaceNumber,
					channel.inEndpoint,
					channel.outEndpoint,
					channel.packetSize);
			LOG_DBG("Using channel {} control interface {}", m_channelCount, channel.controlInterfaceNumber);

			++m_channelCount;
		}

		if (m_channelCount == 0)
		{
			libusb_free_config_descriptor(config_desc);
			LOG_ERROR("Failed to select any USB data channels");
			return false;
		}

		LOG_INFO("Configured {} USB data channel{:s}", m_channelCount, m_channelCount > 1 ? "s" : "");
		libusb_free_config_descriptor(config_desc);
		return true;
	}

	void LIBUSB_CALL UsbDevice::sendTransferCallback(struct libusb_transfer* transfer)
	{
		ZoneScoped;
		TransferData* transferData = static_cast<TransferData*>(transfer->user_data);

		// Notify completion for any pending transfers
		std::unique_lock<LockableBase(std::mutex)> lock(s_transferMutex);
		transferData->completed = true;

		if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
		{
			// Data successfully transferred, invoke the callback
			if (transferData->callback)
			{
				transferData->callback(transferData->buffer);
			}
		}
		else
		{
			LOG_ERROR("Transfer failed: {}", libusb_error_name(transfer->status));
			// Handle error, optionally invoke callback with an empty buffer or error code
		}

		libusb_free_transfer(transfer);		// Free the transfer after processing
		delete transferData;				// Clean up user data
		s_completionCondition.notify_all(); // Notify event loop about completion
	}

	void LIBUSB_CALL UsbDevice::receiveTransferCallback(struct libusb_transfer* transfer)
	{
		ZoneScoped;
		auto* receiveContext = static_cast<ReceiveTransferContext*>(transfer->user_data);
		auto* device = receiveContext->device;
		const std::size_t channelIndex = receiveContext->channelIndex;

		// Notify completion for any pending transfers
		std::unique_lock<LockableBase(std::mutex)> lock(s_transferMutex);

		if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
		{
			// Data successfully transferred, invoke the callback
			if (device->m_receiveCallback)
			{
				device->m_receiveCallback(transfer->buffer, transfer->actual_length, channelIndex);
			}
		}
		else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
		{
			LOG_DBG("Transfer timed out");
		}
		else if (transfer->status == LIBUSB_TRANSFER_OVERFLOW)
		{
			LOG_ERROR("Transfer overflow, buffer too small");
		}
		else
		{
			LOG_ERROR("Transfer failed: {}", libusb_error_name(transfer->status));
		}

		if (device->m_eventThreadRunning.load(std::memory_order_relaxed) && device->m_handle)
		{
			device->receive(channelIndex, s_usbTimeoutMs);
		}

		libusb_free_transfer(transfer);		// Free the transfer after processing
		s_completionCondition.notify_all(); // Notify event loop about completion
	}

	void UsbDevice::eventLoop()
	{
		tracy::SetThreadName("USB Event Loop");
		timeval tv = {0, 50'000}; // 50 ms
		while (m_eventThreadRunning)
		{
			{
				ZoneScoped;
				int r = libusb_handle_events_timeout(s_context, &tv); // blocking call
				if (r == LIBUSB_ERROR_INTERRUPTED)
				{
					continue;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent busy-waiting
		}
	}

	UsbDevice& getCurrentUsbDevice()
	{
		ZoneScoped;
		return s_currentUsbDevice;
	}

	static bool findDuetUsbDevice(libusb_device** device_list,
								  ssize_t device_count,
								  const char** found_device_name,
								  libusb_device** found_device)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		for (ssize_t i = 0; i < device_count; ++i)
		{
			libusb_device* device = device_list[i];
			libusb_device_descriptor desc;
			if (libusb_get_device_descriptor(device, &desc) != 0)
			{
				continue;
			}

			auto vendorDevicesIt = s_devices.find(desc.idVendor);
			if (vendorDevicesIt == s_devices.end())
			{
				continue;
			}

			auto& productDevices = vendorDevicesIt->second;
			auto productDeviceIt = productDevices.find(desc.idProduct);
			if (productDeviceIt == productDevices.end())
			{
				continue;
			}

			std::string_view device_name = productDeviceIt->second;

			LOG_INFO("{:s} target device (Vendor ID: {:#x}, Product ID: {:#x}) found.",
					 device_name,
					 desc.idVendor,
					 desc.idProduct);
			*found_device_name = device_name.data();
			*found_device = device;
			return true;
		}

		*found_device_name = nullptr;
		*found_device = nullptr;
		return false;
	}

	int usbInit()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		setUsbMode(StorageHelper::getData(ID_USB_MODE));
		return libusb_init(&s_context);
	}

	bool connectUsbDevice()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		// Reset any existing connection first
		s_currentUsbDevice.reset();

		// Small delay to allow USB reset to complete
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		LOG_VERBOSE("Getting usb device list");
		libusb_device** device_list;
		ssize_t device_count = libusb_get_device_list(s_context, &device_list);

		if (device_count < 0)
		{
			LOG_ERROR("Failed to get device list: {:s}", libusb_error_name(static_cast<int>(device_count)));
			return false;
		}

		bool ret = true;
		const char* device_name = nullptr;
		libusb_device* device = nullptr;
		if (!findDuetUsbDevice(device_list, device_count, &device_name, &device))
		{
			LOG_ERROR("Target device not found");
			ret = false;
			goto finish;
		}

		s_currentUsbDevice.init(device_name,
								device,
								[](unsigned char* buf, size_t len, size_t channelIndex)
								{
									ZoneScopedN("USB Receive Callback");
									static std::array<Comm::JsonDecoder, Comm::UsbDevice::s_maxChannelCount> s_decoder;
									s_decoder[channelIndex].CheckInput(buf, len);
								});

		if (!s_currentUsbDevice.connect())
		{
			LOG_ERROR("Failed to connect to target device");
			ret = false;
			goto finish;
		}

	finish:
		libusb_free_device_list(device_list, 1);
		return ret;
	}

	ssize_t sendUsbData(std::string_view data, std::size_t channelIndex)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_usbMutex);
		if (!s_currentUsbDevice.isConnected())
		{
			LOG_WARN("USB device not connected");
			return -1;
		}
		return s_currentUsbDevice.send(data, channelIndex);
	}

	void setUsbMode(const UsbMode mode)
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

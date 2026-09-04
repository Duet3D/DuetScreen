#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <libusb-1.0/libusb.h>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace Comm
{
	class UsbDevice;

	// Define a structure to pass user data to the callback
	using transfer_cb_t = std::function<void(const std::vector<unsigned char>&)>;
	using receive_cb_t = std::function<void(unsigned char* buf, size_t len, size_t channelIndex)>;
	struct TransferData
	{
		std::vector<unsigned char> buffer;
		transfer_cb_t callback = nullptr;
		bool completed = false; // Flag for transfer completion
	};

	class UsbDevice
	{
	  public:
		static constexpr std::size_t s_maxChannelCount = 2;
		static constexpr std::size_t s_maxClaimedInterfaceCount = s_maxChannelCount * 2;

		UsbDevice();
		~UsbDevice();

		bool init(const char* name, libusb_device* device, receive_cb_t callback = nullptr);
		bool connect();
		void reset();
		bool send(std::string_view data, std::size_t channelIndex = 0, unsigned int timeoutMs = 0);
		bool isConnected() const { return m_handle != nullptr; }

	  private:
		int setDtr(bool state);

		// CDC-ACM line coding helpers
		// Convenience: set baud as 8N1
		bool setBaud(uint32_t baud);
		// Full control: baud, stop bits (0=1,1=1.5,2=2), parity (0=None..4=Space), data bits
		bool setLineCoding(uint32_t baud, uint8_t stopBits, uint8_t parity, uint8_t dataBits);

		bool getDeviceInterface();

		struct ReceiveTransfer;

		static void LIBUSB_CALL sendTransferCallback(struct libusb_transfer* transfer);
		static void LIBUSB_CALL receiveTransferCallback(struct libusb_transfer* transfer);
		void eventLoop();
		void decodeLoop();
		bool startReceiving();
		void stopReceiving();
		bool submitReceive(ReceiveTransfer& receiveTransfer);

		struct ChannelConfig
		{
			uint8_t inEndpoint = 0;
			uint8_t outEndpoint = 0;
			uint16_t packetSize = 0;
			uint8_t dataInterfaceNumber = 0xFF;
			uint8_t controlInterfaceNumber = 0xFF;
		};

		static constexpr std::size_t s_receiveBufferSize = 4096;
		// Bulk IN transfers kept queued per channel so the device always has a URB to fill
		static constexpr std::size_t s_receiveTransferCount = 4;

		struct ReceiveTransfer
		{
			UsbDevice* device = nullptr;
			std::size_t channelIndex = 0;
			libusb_transfer* transfer = nullptr;
			unsigned char buffer[s_receiveBufferSize];
		};

		struct ReceivedChunk
		{
			std::size_t channelIndex;
			std::vector<unsigned char> data;
		};

		const char* m_name;
		libusb_device* m_device;
		libusb_device_handle* m_handle;

		std::array<ChannelConfig, s_maxChannelCount> m_channels;
		std::size_t m_channelCount;
		std::size_t m_claimedInterfaceCount;
		std::array<uint8_t, s_maxClaimedInterfaceCount> m_claimedInterfaces;

		std::atomic<bool> m_eventThreadRunning;
		std::thread m_eventLoopThread;

		receive_cb_t m_receiveCallback;
		std::array<std::array<ReceiveTransfer, s_receiveTransferCount>, s_maxChannelCount> m_receiveTransfers;
		std::atomic<std::size_t> m_pendingReceiveCount;

		std::atomic<bool> m_decodeThreadRunning;
		std::thread m_decodeThread;
		std::mutex m_receiveQueueMutex;
		std::condition_variable m_receiveQueueCondition;
		std::deque<ReceivedChunk> m_receiveQueue;
	};

	int usbInit();
	bool connectUsbDevice();
	UsbDevice& getCurrentUsbDevice();
	ssize_t sendUsbData(std::string_view data, std::size_t channelIndex = 0);

	enum class UsbMode
	{
		Host,
		Device,
		InternalWiFi,
		Unknown
	};
	void setUsbMode(const UsbMode mode);
	UsbMode getUsbMode();
} // namespace Comm

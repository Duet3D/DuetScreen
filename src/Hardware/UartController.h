#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include "tracy/Tracy.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <termios.h>
#include <thread>
#include <vector>

class UartController
{
  public:
	static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;
	static constexpr int s_writeTimeoutMs = 1000; // How long a write waits for room in the UART's buffer

	using DataCallback = std::function<void(const std::string_view data)>;

	UartController();
	~UartController();

	// Connection
	bool open(const std::string& device);
	void close();
	bool isOpen() const { return m_fd >= 0; }

	// Configuration
	bool setBaudRate(speed_t baudRate);
	speed_t getBaudRate() const { return m_currentBaudRate; }
	bool setParameters(int dataBits, int stopBits, char parity);

	// Data handling
	ssize_t send(std::string_view data);
	/** Give what is received to `callback` from now on, on the read thread */
	void setReceiveCallback(DataCallback callback);

	// Buffer management
	void setBufferSize(size_t size);
	size_t getBufferSize() const { return m_bufferSize; }

  private:
	int m_fd;
	std::atomic<bool> m_running;
	std::thread m_readThread;
	DataCallback m_receiveCallback;
	size_t m_bufferSize;
	speed_t m_currentBaudRate;
	TracyLockable(std::mutex, m_writeMutex);
	TracyLockable(std::mutex, m_callbackMutex);

	void readLoop();
	bool configurePort();
	ssize_t _send(const uint8_t* data, size_t length);
};

#endif

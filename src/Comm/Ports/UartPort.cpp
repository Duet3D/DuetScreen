/*
 * UartPort.cpp
 */

#include "UartPort.h"
#include "Configuration.h"
#include "Debug.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace Comm
{
	namespace
	{
		/** How long the read thread waits before looking whether it should stop, ms */
		constexpr int readWaitMs = 100;

		/** How long a write waits for room in the UART's buffer, ms */
		constexpr int writeWaitMs = 1000;

		/** The largest reply expected: the time it takes at the baud rate is part of the reply timeout */
		constexpr size_t largestReply = 16 * 1024;

		/** The rest of the reply timeout: the printer's own time to answer */
		constexpr std::chrono::milliseconds answerTime{2000};

		/** termios codes and the speeds they stand for */
		struct Speed
		{
			speed_t code;	   // e.g. B115200
			unsigned int bits; // e.g. 115200
		};

		/** Every speed the screen offers */
		constexpr Speed speeds[] = {
			{B1200, 1200},
			{B2400, 2400},
			{B4800, 4800},
			{B9600, 9600},
			{B19200, 19200},
			{B38400, 38400},
			{B57600, 57600},
			{B115200, 115200},
			{B230400, 230400},
#ifndef __APPLE__
			{B460800, 460800},
			{B921600, 921600},
#endif
		};
	} // namespace

	UartPort::UartPort(std::string device, speed_t baudRate)
		: m_device(std::move(device))
		, m_baudRate(baudRate)
	{
	}

	UartPort::~UartPort()
	{
		close();
	}

	unsigned int UartPort::bitsPerSecond(speed_t code)
	{
		for (const Speed& speed : speeds)
		{
			if (speed.code == code)
			{
				return speed.bits;
			}
		}
		return 0;
	}

	std::chrono::milliseconds UartPort::replyTimeout() const
	{
		// 10 bits a byte on the wire (8N1)
		const unsigned int bytesPerSecond = std::max(1u, bitsPerSecond(m_baudRate) / 10);
		return answerTime + std::chrono::milliseconds(largestReply * 1000 / bytesPerSecond);
	}

	bool UartPort::open(Receiver receiver)
	{
		ZoneScoped;
		close();
		LOG_INFO("Opening the UART {:s} at {:d} baud", m_device, bitsPerSecond(m_baudRate));
		m_fd = ::open(m_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
		if (m_fd < 0)
		{
			LOG_ERROR("Couldn't open {:s}: {:s}", m_device, strerror(errno));
			return false;
		}
		if (!configure())
		{
			::close(m_fd);
			m_fd = -1;
			return false;
		}
		m_receiver = std::move(receiver);
		m_failed = false;
		m_reading = true;
		m_reader = std::thread(&UartPort::readLoop, this);
		return true;
	}

	void UartPort::close()
	{
		ZoneScoped;
		m_reading = false;
		if (m_reader.joinable())
		{
			m_reader.join();
		}
		if (m_fd >= 0)
		{
			::close(m_fd);
			m_fd = -1;
		}
		m_receiver = nullptr;
	}

	bool UartPort::write(size_t, std::string_view bytes)
	{
		ZoneScoped;
		ZoneValue(bytes.size());
		size_t written = 0;
		while (m_fd >= 0 && written < bytes.size())
		{
			const ssize_t result = ::write(m_fd, bytes.data() + written, bytes.size() - written);
			if (result > 0)
			{
				written += static_cast<size_t>(result);
				continue;
			}
			if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				// The UART's buffer is full: wait for room rather than spinning
				pollfd room{.fd = m_fd, .events = POLLOUT, .revents = 0};
				if (::poll(&room, 1, writeWaitMs) > 0 && (room.revents & POLLOUT) != 0)
				{
					continue;
				}
				LOG_ERROR("Writing to the UART timed out ({:d} of {:d} bytes)", written, bytes.size());
				return false;
			}
			LOG_ERROR("Writing to the UART failed: {:s}", strerror(errno));
			m_failed = true;
			return false;
		}
		return written == bytes.size();
	}

	bool UartPort::configure()
	{
		ZoneScoped;
		termios tty{};
		if (tcgetattr(m_fd, &tty) != 0)
		{
			LOG_ERROR("Couldn't read the UART's settings: {:s}", strerror(errno));
			return false;
		}
		cfmakeraw(&tty);
		tty.c_cflag |= CLOCAL | CREAD; // no modem control
		tty.c_cflag &= ~static_cast<tcflag_t>(CSTOPB | PARENB | CRTSCTS);
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 0;
		if (cfsetispeed(&tty, m_baudRate) != 0 || cfsetospeed(&tty, m_baudRate) != 0 ||
			tcsetattr(m_fd, TCSANOW, &tty) != 0)
		{
			LOG_ERROR("Couldn't set the UART up: {:s}", strerror(errno));
			return false;
		}
		tcflush(m_fd, TCIOFLUSH); // nothing left over from before
		return true;
	}

	void UartPort::readLoop()
	{
		tracy::SetThreadName("UART");
		std::array<char, 4096> buffer;
		while (m_reading)
		{
			pollfd arrived{.fd = m_fd, .events = POLLIN, .revents = 0};
			const int ready = ::poll(&arrived, 1, readWaitMs);
			if (ready < 0 && errno != EINTR)
			{
				LOG_ERROR("Waiting for the UART failed: {:s}", strerror(errno));
				m_failed = true;
				return;
			}
			if (ready <= 0)
			{
				continue;
			}
			if ((arrived.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
			{
				LOG_ERROR("The UART has gone");
				m_failed = true;
				return;
			}
			const ssize_t count = ::read(m_fd, buffer.data(), buffer.size());
			if (count > 0)
			{
				ZoneScopedN("UART received");
				ZoneValue(count);
				m_receiver(0, std::string_view(buffer.data(), static_cast<size_t>(count)));
			}
			else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			{
				LOG_ERROR("Reading the UART failed: {:s}", strerror(errno));
				m_failed = true;
				return;
			}
		}
	}
} // namespace Comm

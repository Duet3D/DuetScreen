/*
 * UartPort.cpp
 */

#include "UartPort.h"
#include "Debug.h"

namespace Comm
{
	UartPort::UartPort(std::string device, speed_t baudRate)
		: m_device(std::move(device))
		, m_baudRate(baudRate)
	{
	}

	bool UartPort::open(Receiver receiver)
	{
		LOG_INFO("Opening UART {:s}", m_device);
		if (!m_uart.setBaudRate(m_baudRate))
		{
			LOG_ERROR("Failed to set the UART's baud rate");
			return false;
		}
		m_uart.setReceiveCallback([receiver = std::move(receiver)](std::string_view bytes) { receiver(0, bytes); });
		return m_uart.open(m_device);
	}

	void UartPort::close()
	{
		m_uart.close();
	}

	bool UartPort::write(size_t, std::string_view bytes)
	{
		return m_uart.send(bytes) == static_cast<ssize_t>(bytes.size());
	}
} // namespace Comm

/*
 * SerialIO.cpp
 *
 * Created: 04/02/2023
 *  Author: Loïc G.
 *
 *  This file is a modified version of the SerialIO.cpp source file from PanelDue
 *  The original can be found at https://github.com/Duet3D/PanelDueFirmware/blob/master/src/Hardware/SerialIo.cpp
 *
 */

#include "SerialIo.h"
#include "Comm/Communication.h"
#include "Comm/LineFramer.h"
#include "Comm/Receive.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include <array>
#include <string>

namespace SerialIo
{
	static std::unique_ptr<UartController> s_uart;
	static Comm::LineFramer s_framer(Comm::receiveLine); // Gathers the replies received into lines

	static void processData(const std::string_view data);

	bool Init(const std::string& device, speed_t baudRate)
	{
#if SIMULATION
		LOG_INFO("Initializing simulated UART on device: {:s}", device);
#endif
		s_uart = std::make_unique<UartController>();

		if (!s_uart->setBaudRate(baudRate))
		{
			LOG_ERROR("Failed to set baud rate");
			return false;
		}
		RestoreDataCallback();

		return s_uart->open(device);
	}

	void Shutdown()
	{
		if (s_uart)
		{
			s_uart.reset();
		}
	}

	ssize_t Send(std::string_view data)
	{
		if (!s_uart)
		{
			LOG_ERROR("UART not initialized");
			return -1;
		}
		return s_uart->send(data);
	}

	bool SetBaudRate(speed_t baudRate)
	{
		return s_uart && s_uart->setBaudRate(baudRate);
	}

	bool IsConnected()
	{
		return s_uart && s_uart->isOpen();
	}

	void SetDataCallback(UartController::DataCallback callback)
	{
		if (s_uart)
		{
			s_uart->setReceiveCallback(callback);
		}
	}

	void RestoreDataCallback()
	{
		if (s_uart)
		{
			s_uart->setReceiveCallback(processData);
		}
	}

	/** Hand on the bytes received: each whole line is a reply */
	static void processData(const std::string_view data)
	{
		LOG_VERBOSE("Received ({:d}) '{:s}'", data.length(), data);

		if (Comm::DUET.GetCommunicationType() != Comm::CommunicationType::uart)
		{
			s_framer.reset();
			return;
		}
		s_framer.push(data);
	}
} // namespace SerialIo

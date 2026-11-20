/*
 * UsbPort.cpp
 */

#include "UsbPort.h"
#include "Comm/Usb.h"
#include "tracy/Tracy.hpp"

namespace Comm
{
	UsbPort::~UsbPort()
	{
		ZoneScoped;
		close();
	}

	bool UsbPort::open(Receiver receiver)
	{
		ZoneScoped;
		return connectUsbDevice([receiver = std::move(receiver)](unsigned char* buf, size_t len, size_t channel)
								{ receiver(channel, std::string_view(reinterpret_cast<const char*>(buf), len)); });
	}

	void UsbPort::close()
	{
		ZoneScoped;
		getCurrentUsbDevice().reset();
	}

	bool UsbPort::write(size_t channel, std::string_view bytes)
	{
		ZoneScoped;
		return sendUsbData(bytes, channel) >= 0;
	}

	size_t UsbPort::channels() const
	{
		ZoneScoped;
		return getCurrentUsbDevice().channelCount();
	}
} // namespace Comm

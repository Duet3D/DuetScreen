/*
 * UsbPort.cpp
 */

#include "UsbPort.h"
#include "Comm/Usb.h"

namespace Comm
{
	UsbPort::~UsbPort()
	{
		close();
	}

	bool UsbPort::open(Receiver receiver)
	{
		return connectUsbDevice([receiver = std::move(receiver)](unsigned char* buf, size_t len, size_t channel)
								{ receiver(channel, std::string_view(reinterpret_cast<const char*>(buf), len)); });
	}

	void UsbPort::close()
	{
		getCurrentUsbDevice().reset();
	}

	bool UsbPort::write(size_t channel, std::string_view bytes)
	{
		return sendUsbData(bytes, channel) >= 0;
	}

	size_t UsbPort::channels() const
	{
		return getCurrentUsbDevice().channelCount();
	}
} // namespace Comm

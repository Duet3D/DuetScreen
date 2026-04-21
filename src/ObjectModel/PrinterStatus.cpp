/*
 * PrinterStatus.cpp
 *
 *  Created on: 9 Jan 2024
 *      Author: Andy Everitt
 */

#include "Debug.h"

#include "Hardware/Duet.h"
#include "ObjectModel/Utils.h"
#include "PrinterStatus.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "nameof.hpp"
#include <string>

namespace OM
{
	static std::string s_printerName;
	static std::string s_uniqueId;
	static PrinterStatus s_status = PrinterStatus::connecting;
	static uint32_t s_channelIndex = 0;

	bool IsPrintingStatus(OM::PrinterStatus status)
	{
		return status == OM::PrinterStatus::processing || status == OM::PrinterStatus::paused ||
			   status == OM::PrinterStatus::pausing || status == OM::PrinterStatus::resuming ||
			   status == OM::PrinterStatus::simulating;
	}

	bool PrintInProgress()
	{
		return IsPrintingStatus(s_status);
	}

	bool IsConnected()
	{
		return s_status != OM::PrinterStatus::connecting && s_status != OM::PrinterStatus::off &&
			   s_status != OM::PrinterStatus::disconnected;
	}

	// Return true if sending a command or file list request to the printer now is a good idea.
	// We don't want to send these when the printer is busy with a previous command, because they will block normal
	// status requests.
	bool OkToSend()
	{
		return s_status == OM::PrinterStatus::idle || s_status == OM::PrinterStatus::processing ||
			   s_status == OM::PrinterStatus::paused || s_status == OM::PrinterStatus::off;
	}

	// Return the printer status
	PrinterStatus GetStatus()
	{
		return s_status;
	}

	const char* GetStatusText()
	{
		if (s_status > OM::PrinterStatus::unknown && s_status < OM::PrinterStatus::NumTypes)
			return printerStatusMap[(int)s_status].key;
		return "unknown";
	}

	void SetStatus(const char* status)
	{
		const PrinterStatusMapEntry key = {status, OM::PrinterStatus::unknown};
		const PrinterStatusMapEntry* statusFromMap =
			(OM::PrinterStatusMapEntry*)bsearch(&key,
												OM::printerStatusMap,
												ARRAY_SIZE(OM::printerStatusMap),
												sizeof(OM::PrinterStatusMapEntry),
												compareKey<OM::PrinterStatusMapEntry>);
		if (!statusFromMap)
		{
			LOG_ERROR("unknown status {:s}", status);
			return;
		}
		SetStatus(statusFromMap->val);
	}

	// This is called when the status changes
	void SetStatus(const PrinterStatus newStatus)
	{
		const bool sendEvent = newStatus != s_status || newStatus == OM::PrinterStatus::connecting;

		if (newStatus != s_status)
		{
			LOG_INFO("printer status '{:s}' -> '{:s}'", nameof::nameof_enum(s_status), nameof::nameof_enum(newStatus));
			s_status = newStatus;
			if (s_status == OM::PrinterStatus::halted)
			{
				Comm::DUET.SendGcode("M999");
				Model::get().post<EventType::Response>(ResponseType::INFO, _("estop.reset_message"));
			}
		}
		if (sendEvent)
		{
			/**
			 * We post an event even if the status didn't change, if the status is "connecting", because we want to
			 * trigger a UI update when the connection method is changed.
			 */
			Model::get().post<EventType::Status>(GetStatus());
		}
	}

	const std::string& GetPrinterName()
	{
		return s_printerName;
	}

	void SetPrinterName(const char* name)
	{
		if (s_printerName == name)
		{
			return;
		}

		s_printerName = name;
		Model::get().post<EventType::NetworkName>();
	}

	const std::string& GetPrinterUniqueId()
	{
		return s_uniqueId;
	}

	void SetPrinterUniqueId(const char* id)
	{
		if (s_uniqueId == id)
		{
			return;
		}

		s_uniqueId = id;
		Model::get().post<EventType::PrinterUniqueId>();
	}

	uint32_t GetChannelIndex()
	{
		return s_channelIndex;
	}

	void SetChannelIndex(uint32_t index)
	{
		LOG_DBG("Setting input channel index to {:d}", index);
		s_channelIndex = index;
	}
} // namespace OM

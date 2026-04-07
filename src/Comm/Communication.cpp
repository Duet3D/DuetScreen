/*
 * Communication.cpp
 *
 * Created: 30/01/2023
 *  Author: Loïc G.
 *
 *  This file is a based on PanelDue project
 *  The original can be found at https://github.com/Duet3D/PanelDueFirmware/blob/master/src/PanelDue.cpp
 */

#include "Debug.h"

#include "Comm/Communication.h"
#include <stdarg.h>

#include "Comm/JsonDecoder.h"
#include "Comm/Usb.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "Hardware/SerialIo.h"

#include "Comm/ControlCommands.h"
#include "ObjectModel/Alert.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Spindle.h"
#include "ObjectModel/Tool.h"
#include "ObjectModel/Utils.h"
#include "tracy/Tracy.hpp"
#include "utils/TimeHelper.h"
#include <filesystem>

// These defines control which detailed M409 requests will be sent
// If one of the fields in the disabled ones need to be fetched the
// corresponding define has to be set to (1)
#define FETCH_BOARDS (1)
#define FETCH_DIRECTORIES (1)
#define FETCH_FANS (1)
#define FETCH_HEAT (1)
#define FETCH_INPUTS (1)
#define FETCH_JOB (1)
#define FETCH_MOVE (1)
#define FETCH_NETWORK (1)
#define FETCH_SCANNER (0)
#define FETCH_SENSORS (1)
#define FETCH_SPINDLES (1)
#define FETCH_STATE (1)
#define FETCH_TOOLS (1)
#define FETCH_VOLUMES (1)

namespace Comm
{

	static std::chrono::milliseconds s_lastResponseTime(0);

	Seq seqs[] = {
#if FETCH_NETWORK
		{.seqid = rcvSeqsNetwork, .lastSeq = 0, .state = SeqStateInit, .key = "network", .flags = "v"},
#endif
#if FETCH_BOARDS
		{.seqid = rcvSeqsBoards, .lastSeq = 0, .state = SeqStateInit, .key = "boards", .flags = "v"},
#endif
#if FETCH_MOVE
		{.seqid = rcvSeqsMove, .lastSeq = 0, .state = SeqStateInit, .key = "move", .flags = "vn"},
#endif
#if FETCH_HEAT
		{.seqid = rcvSeqsHeat, .lastSeq = 0, .state = SeqStateInit, .key = "heat", .flags = "v"},
#endif
#if FETCH_TOOLS
		{.seqid = rcvSeqsTools, .lastSeq = 0, .state = SeqStateInit, .key = "tools", .flags = "v"},
#endif
#if FETCH_SPINDLES
		{.seqid = rcvSeqsSpindles, .lastSeq = 0, .state = SeqStateInit, .key = "spindles", .flags = "v"},
#endif
#if FETCH_DIRECTORIES
		{.seqid = rcvSeqsDirectories, .lastSeq = 0, .state = SeqStateInit, .key = "directories", .flags = "v"},
#endif
#if FETCH_FANS
		{.seqid = rcvSeqsFans, .lastSeq = 0, .state = SeqStateInit, .key = "fans", .flags = "v"},
#endif
#if FETCH_INPUTS
		{.seqid = rcvSeqsInputs, .lastSeq = 0, .state = SeqStateInit, .key = "inputs", .flags = "v"},
#endif
#if FETCH_JOB
		{.seqid = rcvSeqsJob, .lastSeq = 0, .state = SeqStateInit, .key = "job", .flags = "vn"},
#endif
#if FETCH_SCANNER
		{.seqid = rcvSeqsScanner, .lastSeq = 0, .state = SeqStateInit, .key = "scanner", .flags = "v"},
#endif
#if FETCH_SENSORS
		{.seqid = rcvSeqsSensors, .lastSeq = 0, .state = SeqStateInit, .key = "sensors", .flags = "v"},
#endif
#if FETCH_STATE
		{.seqid = rcvSeqsState, .lastSeq = 0, .state = SeqStateInit, .key = "state", .flags = "vn"},
#endif
#if FETCH_VOLUMES
		{.seqid = rcvSeqsVolumes, .lastSeq = 0, .state = SeqStateInit, .key = "volumes", .flags = "v"},
#endif
		{.seqid = rcvSeqsGlobal, .lastSeq = 0, .state = SeqStateInit, .key = "global", .flags = "v"},
		{.seqid = rcvSeqsFreq, .lastSeq = 0, .state = SeqStateInit, .key = "", .flags = "d99f"}};

	Seq* g_currentReqSeq = nullptr;

	struct Seq* GetNextSeq(struct Seq* current)
	{
		ZoneScoped;
		if (current == nullptr)
		{
			current = seqs;
		}

		if (current == &seqs[ARRAY_SIZE(seqs) - 1])
		{
			current = seqs;
		}

		for (size_t i = current - seqs; i < ARRAY_SIZE(seqs); ++i)
		{
			current = &seqs[i];
			if (current->state == SeqStateError)
			{
				LOG_WARN("seq '{:s}' had an error", current->key);
				// skip and re-init if last request had an error
				current->state = SeqStateInit;
				continue;
			}
			if (current->state == SeqStateInit || current->state == SeqStateUpdate)
			{
				LOG_DBG("seq '{:s}'", current->key);
				return current;
			}
			if (current->state == SeqStateRequested && current->lastRequestTime + 500ms < TimeHelper::getRunningTime())
			{
				LOG_DBG("seq '{:s}' was requested but not updated, re-requesting", current->key);
				current->state = SeqStateUpdate;
				return current;
			}
		}
		return nullptr;
	}

	Seq* FindSeqByKey(const char* key)
	{
		ZoneScoped;
		LOG_VERBOSE("key {:s}\n", key);

		for (size_t i = 0; i < ARRAY_SIZE(seqs); ++i)
		{
			if (strcasecmp(seqs[i].key, key) == 0)
			{
				return &seqs[i];
			}
		}

		LOG_WARN("Received key '{:s}' does not match any known seq", key);
		return nullptr;
	}

	void UpdateSeq(const ReceivedDataEvent seqid, int32_t val)
	{
		ZoneScoped;
		for (size_t i = 0; i < ARRAY_SIZE(seqs); ++i)
		{
			if (seqs[i].seqid == seqid)
			{
				if (seqs[i].lastSeq != val)
				{
					LOG_DBG("Seq {:s} {:d} -> {:d}\n", seqs[i].key, seqs[i].lastSeq, val);
					seqs[i].lastSeq = val;
					seqs[i].state = SeqStateUpdate;
				}
			}
		}
	}

	void ResetSeqs()
	{
		ZoneScoped;
		for (size_t i = 0; i < ARRAY_SIZE(seqs); ++i)
		{
			seqs[i].lastSeq = 0;
			seqs[i].state = SeqStateInit;
		}
		g_currentReqSeq = nullptr;
	}

	static void RequestSeq(Seq* seq)
	{
		ZoneScoped;
		if (seq == nullptr)
		{
			LOG_ERROR("RequestSeq called with null seq");
			return;
		}

		if (seq->state == SeqStateRequested)
		{
			LOG_DBG("Seq {:s} already requested", seq->key);
			return;
		}

		LOG_DBG("Requesting seq '{:s}'", seq->key);
		seq->lastRequestTime = TimeHelper::getRunningTime();
		seq->state = SeqStateRequested;

		Comm::DUET.RequestModel(g_currentReqSeq->key, g_currentReqSeq->flags);
	}

	// Try to get an integer value from a string. If it is actually a floating point value, round it.
	// NB: We are using the old fashioned way (using strtol/strtof) instead of the safe functions provided
	// by the RRFLibraries because of the uClibc++ library not providing the things we need for that.
	bool GetInteger(const char s[], int32_t& rslt)
	{
		ZoneScoped;
		if (!s || s[0] == 0)
			return false; // empty string

		if (strlen(s) > 11) // this is guaranteed to be out of int32_t range
			return false;

		char* endptr;
		errno = 0;
		auto tmp = strtol(s, &endptr, 10);

		if (errno == ERANGE || tmp < INT32_MIN || tmp > INT32_MAX)
		{
			return false; // truncated or out of range
		}

		if (errno == 0 && *endptr == 0)
		{
			rslt = static_cast<int32_t>(tmp);
			return true; // we parsed an integer
		}

		errno = 0;
		float d = strtof(s, &endptr); // try parsing a floating point number
		if (errno == 0 && *endptr == 0)
		{
			rslt = static_cast<int32_t>(((d < 0.0f) ? d - 0.5f : d + 0.5f));
			return true;
		}
		return false;
	}

	// Try to get an unsigned integer value from a string
	// NB: We are using the old fashioned way (using strtol/strtof) instead of the safe functions provided
	// by the RRFLibraries because of the uClibc++ library not providing the things we need for that.
	bool GetUnsignedInteger(const char s[], uint32_t& rslt)
	{
		ZoneScoped;
		if (!s || s[0] == 0)
			return false; // empty string

		if (strlen(s) > 10) // this is guaranteed to be out of uint32_t range
			return false;

		char* endptr;
		errno = 0;
		auto tmp = strtoul(s, &endptr, 10);
		if (errno == ERANGE || tmp > UINT32_MAX)
		{
			return false; // truncated or out of range
		}

		if (errno == 0 && *endptr == 0)
		{
			rslt = static_cast<uint32_t>(tmp);
			return true; // we parsed an integer
		}

		errno = 0;
		float d = strtof(s, &endptr); // try parsing a floating point number
		if (errno == 0 && *endptr == 0)
		{
			if (d < 0.0f)
			{
				return false; // negative number
			}
			rslt = static_cast<uint32_t>(d + 0.5f);
			return true;
		}
		return *endptr == 0;
	}

	// Try to get a floating point value from a string. if it is actually a floating point value, round it.
	// NB: We are using the old fashioned way (using strtol/strtof) instead of the safe functions provided
	// by the RRFLibraries because of the uClibc++ library not providing the things we need for that.
	bool GetFloat(const char s[], float& rslt)
	{
		ZoneScoped;
		if (!s || s[0] == 0)
			return false; // empty string

		// GNU strtod is buggy, it's very slow for some long inputs, and some versions have a buffer overflow bug.
		// We presume strtof may be buggy too. Tame it by rejecting any strings that much longer than we expect to
		// receive.
		if (strlen(s) > 10)
			return false;

		char* endptr;
		errno = 0;
		auto tmp = strtof(s, &endptr);
		if (errno == ERANGE)
		{
			return false; // truncated or out of range
		}

		if (*endptr != 0)
		{
			return false; // not a valid float
		}

		rslt = tmp;
		return *endptr == 0; // we parsed a float
	}

	// Try to get a bool value from a string.
	bool GetBool(const char s[], bool& rslt)
	{
		ZoneScoped;
		if (!s || s[0] == 0)
			return false; // empty string

		rslt = (strcasecmp(s, "true") == 0);
		return true;
	}

	void Reconnect()
	{
		ZoneScoped;
		LOG_DBG("Reconnecting");
		KickWatchdog();
		//		lastOutOfBufferResponse = 0;
		OM::SetStatus(OM::PrinterStatus::connecting);
		// DUET.SendGcode("M29");
		DUET.Reconnect();
		OM::RemoveAll();
		ResetSeqs();
	}

	void KickWatchdog()
	{
		ZoneScoped;
		const std::chrono::milliseconds now = TimeHelper::getRunningTime();
		if (now > s_lastResponseTime)
		{
			s_lastResponseTime = TimeHelper::getRunningTime();
		}
	}

	//------------------------------------------------------------------------------------------------------------------

	/**
	 * @brief Sends the next request in sequence to the Duet mainboard
	 *
	 * This function handles the communication sequence with the Duet printer by:
	 * 1. Checking if the printer has timed out based on the last response time
	 * 2. Initiating a reconnection if a timeout is detected
	 * 3. Sending the next request in sequence or falling back to frequently changing data
	 *
	 * @return true if a regular sequence request was sent
	 * @return false if falling back to requesting frequently changing data
	 *
	 * @note Manages printer timeout detection and automatic reconnection
	 */
	bool sendNext()
	{
		ZoneScoped;
		if (!DUET.IsConnected() && !DUET.IsConnecting())
		{
			Reconnect();
			return false;
		}

		const auto now = TimeHelper::getRunningTime();
		const auto expectedResponseBy = s_lastResponseTime + DUET.GetScaledPollInterval() + PRINTER_REQUEST_TIMEOUT;
		if (now > expectedResponseBy)
		{
			LOG_WARN("No response from Duet for {} ms", PRINTER_REQUEST_TIMEOUT);
			LOG_VERBOSE("last response={}, now={}, expected by={}, diff={}",
						s_lastResponseTime,
						now,
						expectedResponseBy,
						now - expectedResponseBy);
			Reconnect();
		}

		g_currentReqSeq = GetNextSeq(g_currentReqSeq);
		if (g_currentReqSeq == nullptr)
		{
			LOG_DBG("No more seqs to request");
			return false;
		}

		RequestSeq(g_currentReqSeq);
		return true;
	}

	void init()
	{
		ZoneScoped;
		LOG_INFO("Initializing Communication Module...");
		// Sort the fieldTable prior searching using binary search
		std::filesystem::create_directories("/tmp/thumbnails");
		std::filesystem::create_directories("/tmp/heightmaps");
		std::filesystem::create_directories("/tmp/files");
		SortFieldTable();
		usbInit();
		ResetSeqs();
		KickWatchdog();
	}
} // namespace Comm

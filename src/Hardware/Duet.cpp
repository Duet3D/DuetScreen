/*
 * Duet.cpp
 *
 *  Created on: 26 Jan 2024
 *      Author: Andy Everitt
 */

#include "Comm/Communication.h"
#include "Comm/FileInfo.h"
#include "Comm/LineFramer.h"
#include "Comm/Receive.h"
#include "Comm/Replies.h"
#include "Comm/Responses.h"
#include "Comm/Usb.h"
#include "Debug.h"

#include "Duet.h"

#include "Duet3D/General/CRC16.h"
#include "Hardware/SerialIo.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Json.h"
#include "ObjectModel/Utils.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#if DUET_SIMULATOR
#  include "Simulator/Simulator.h"
#endif
#include "nameof.hpp"
#include "utils/StorageHelper.h"
#include "utils/TimeHelper.h"
#include "utils/utils.h"
#include <map>
#include <string>

namespace Comm
{
	/**
	 * An HTTP reply's body, read as JSON
	 * @return the JSON, or why there is none: an HTTP error, or a body that isn't JSON
	 */
	static std::expected<OM::Node, ReplyError> readJsonBody(const HttpResponse& r)
	{
		if (r.status_code != HTTP_STATUS_OK)
		{
			return std::unexpected(ReplyError{.message = fmt::format("HTTP error {:d}", (int)r.status_code)});
		}
		auto json = OM::parseJson(r.body);
		if (!json)
		{
			return std::unexpected(
				ReplyError{.message = fmt::format("a reply that isn't JSON: {:s}", json.error().message)});
		}
		return std::move(*json);
	}

	/** Hand an object model reply (rr_model's body) to the object model */
	static void receiveModelBody(const HttpResponse& r, std::string_view what)
	{
		auto json = readJsonBody(r);
		if (!json)
		{
			LOG_ERROR("Failed to get model update for {:s}: {:s}", what, json.error().message);
			return;
		}
		receiveReply(*json);
	}

	Duet::Duet()
		: m_lastRequestTime(0)
		, m_pollIntervalScale(1.0f)
		, m_sessionKey(sm_noSessionKey)
		, m_sessionTimeout(0)
		, m_sbcMode(false)
	{
	}

	void Duet::Init()
	{
		ZoneScoped;
		DuetConfig config = {
			.ipAddress = std::string(StorageHelper::getData(ID_DUET_IP_ADDRESS)),
			.password = std::string(StorageHelper::getData(ID_DUET_PASSWORD)),
			.pollInterval = StorageHelper::getData(ID_DUET_POLL_INTERVAL),
			.communicationType = static_cast<CommunicationType>(StorageHelper::getData(ID_DUET_COMMUNICATION_TYPE)),
			.baudRate = StorageHelper::getData(ID_DUET_BAUD_RATE),
		};

		SetPollInterval(config.pollInterval);
		SetBaudRate(config.baudRate);
		SetIPAddress(config.ipAddress);
		SetPassword(config.password);
		SetCommunicationType((CommunicationType)config.communicationType);

		OM::RemoveAll();
	}

	void Duet::Reset()
	{
		ZoneScoped;
		LOG_VERBOSE("Resetting Duet");
		m_sessionKey = sm_noSessionKey;
		m_sbcMode = false;
		m_sessionTimeout = std::chrono::milliseconds(0);
		m_lastRequestTime = TimeHelper::getRunningTime();
		m_pollIntervalScale = 1.0f;
		m_nextLineNumber = 0;
		FILEINFO_CACHE->ClearCache();

		OM::RemoveAll();
		Comm::ResetSeqs();
	}

	void Duet::Reconnect()
	{
		ZoneScoped;
		LOG_INFO("Reconnecting...");
		Disconnect();
		Connect();
	}

	void Duet::SetCommunicationType(CommunicationType type)
	{
		ZoneScoped;
		if (type == m_config.communicationType)
			return;
#if !DUET_SIMULATOR
		if (type == CommunicationType::simulator)
		{
			LOG_WARN("This build has no simulated Duet");
			return;
		}
#endif
		LOG_INFO("Setting communication type to {:s}", nameof::nameof_enum(type));
		Disconnect();

		m_config.communicationType = type;
		Connect();
		StorageHelper::setData(ID_DUET_COMMUNICATION_TYPE, type);
	}

	CommunicationType Duet::GetCommunicationType() const
	{
		ZoneScoped;
		return m_config.communicationType;
	}

	std::string_view Duet::GetCommunicationTypeName() const
	{
		ZoneScoped;
		CommunicationType type = m_config.communicationType;
		return duetCommunicationTypeNames[(int)type];
	}

	void Duet::SetPollInterval(std::chrono::milliseconds interval)
	{
		ZoneScoped;
		if (interval < MIN_PRINTER_POLL_INTERVAL)
		{
			LOG_WARN("Poll interval too low, setting to {}", MIN_PRINTER_POLL_INTERVAL);
			interval = MIN_PRINTER_POLL_INTERVAL;
		}
		LOG_INFO("Setting poll interval to {} (scaled to {})",
				 interval,
				 std::chrono::duration_cast<std::chrono::milliseconds>(interval * m_pollIntervalScale));

		m_config.pollInterval = interval;
		StorageHelper::setData(ID_DUET_POLL_INTERVAL, interval);
		// resetUserTimer(TIMER_UPDATE_DATA, static_cast<int>(m_pollInterval * m_pollIntervalScale));
	}

	void Duet::ScalePollIntervalScale(float scale)
	{
		ZoneScoped;
		if (scale <= 0.0f)
		{
			LOG_WARN("Invalid scale factor {:g}", scale);
			return;
		}

		LOG_INFO("Scalling poll interval by {:g} from {} to {}",
				 scale,
				 GetScaledPollInterval(),
				 std::chrono::duration_cast<std::chrono::milliseconds>(m_config.pollInterval * scale));

		m_pollIntervalScale = scale;
		// resetUserTimer(TIMER_UPDATE_DATA, static_cast<int>(m_pollInterval * m_pollIntervalScale));
	}

	std::chrono::milliseconds Duet::GetPollInterval() const
	{
		ZoneScoped;
		return m_config.pollInterval;
	}

	std::chrono::milliseconds Duet::GetScaledPollInterval() const
	{
		ZoneScoped;
		return std::chrono::duration_cast<std::chrono::milliseconds>(m_config.pollInterval * m_pollIntervalScale);
	}

	void Duet::PrepareRequest(HttpRequest& req, std::string_view subUrl, hv::QueryParams& queryParameters)
	{
		ZoneScoped;
		req.method = HTTP_GET;
		req.host = GetBaseUrl();
		req.path = subUrl;
		req.headers["Connection"] = "keep-alive";
		req.headers["Accept"] = "application/json";
		req.headers["Content-Type"] = "application/json";
		if (m_sessionKey != sm_noSessionKey)
		{
			req.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey);
		}
		req.query_params = queryParameters;
		req.timeout = HTTP_TIMEOUT;

		req.DumpUrl();
	}

	void Duet::AsyncGetInner(const HttpRequestPtr& req, HttpResponseCallback callback)
	{
		ZoneScoped;
		m_cli.sendAsync(req,
						[this, req, callback](const HttpResponsePtr& resp) { AsyncGetCallback(req, resp, callback); });
		m_lastRequestTime = TimeHelper::getRunningTime();
	}

	bool Duet::AsyncGetCallback(const HttpRequestPtr& req, const HttpResponsePtr& r, HttpResponseCallback callback)
	{
		ZoneScoped;
		if (r == NULL)
		{
			LOG_ERROR("request \"{:s}\" failed!", req->url.c_str());
			// AsyncGetInner(req, callback);
			return false;
		}
		LOG_DBG("Response (async): {:s} {:s}", req->url.c_str(), r->status_message());
		LOG_VERBOSE("{:s}", r->body.c_str());
		callback(r);
		return true;
	}

	bool Duet::AsyncGet(std::string_view path, hv::QueryParams& queryParameters, HttpResponseCallback callback)
	{
		ZoneScoped;
		if (!IsConnected() && path != "/rr_connect")
		{
			LOG_DBG("Not connected to Duet, cannot send get request {:s}", path);
			return false;
		}

		auto req = std::make_shared<HttpRequest>();
		PrepareRequest(*req, path, queryParameters);
		LOG_DBG("Get (async): \"{:s}\", sessionKey={:d}", req->url.c_str(), m_sessionKey);

		// `sendAsync()` requires the client to still be alive later and does appear to be thread safe using a single
		// client

		AsyncGetInner(req, std::move(callback));
		return true;
	}

	/*
	Tries to make a get request to Duet, if it returns 401 or 403 then it will run `rr_connect` and send the request
	again
	*/
	bool Duet::Get(std::string_view path, HttpResponse& r, hv::QueryParams& queryParameters)
	{
		ZoneScoped;
		if (!IsConnected() && path != "/rr_connect")
		{
			LOG_DBG("Not connected to Duet, cannot send get request {:s}", path);
			return false;
		}

		HttpRequest req;
		PrepareRequest(req, path, queryParameters);
		LOG_DBG("\"{:s}\", sessionKey={:d}", req.url.c_str(), m_sessionKey);

		hv::HttpClient cli;
		cli.send(&req, &r); // `send()` is not thread safe if using the same client so client is created on stack

		LOG_DBG("Response (async): {:s} {:s}", req.url.c_str(), r.status_message());

		if (r.status_code != HTTP_STATUS_OK)
		{
			LOG_ERROR("HTTP error {:d}: Likely invalid sessionKey {:d}.", (int)r.status_code, m_sessionKey);
			return false;
		}
		LOG_VERBOSE("{:s}", r.body.c_str());
		m_lastRequestTime = TimeHelper::getRunningTime();
		return true;
	}

	/*
	Tries to make a post request to Duet, if it returns 401 or 403 then it will run `rr_connect` and send the request
	again
	*/
	bool Duet::Post(std::string_view subUrl, HttpResponse& r, hv::QueryParams& queryParameters, std::string_view data)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			if (!Connect())
			{
				LOG_WARN("Failed to connect to Duet, cannot send post request {:s}", subUrl);
				return false;
			}
		}

		HttpRequest req;
		req.method = HTTP_POST;
		req.host = GetBaseUrl();
		req.path = subUrl;
		req.headers["Connection"] = "keep-alive";
		req.headers["Accept"] = "application/json";
		req.headers["Content-Type"] = "application/json";
		if (m_sessionKey != sm_noSessionKey)
		{
			req.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey);
		}
		req.query_params = queryParameters;
		req.timeout = HTTP_TIMEOUT;
		req.body = data;

		req.DumpUrl();

		hv::HttpClient cli;
		cli.send(&req, &r); // `send()` is not thread safe if using the same client so client is created on stack

		LOG_DBG("Response (post): {:s} {:s}", req.url.c_str(), r.status_message());

		if (r.status_code != HTTP_STATUS_OK)
		{
			LOG_ERROR("HTTP error {:d}: Likely invalid sessionKey {:d}.", (int)r.status_code, m_sessionKey);
			return false;
		}
		LOG_VERBOSE("{:s}", r.body.c_str());
		m_lastRequestTime = TimeHelper::getRunningTime();
		return true;
	}

	void Duet::Estop()
	{
		SendGcode("M112 M999\n", true);
		LOG_WARN("Emergency Stop sent to Duet");
		if (GetCommunicationType() == Comm::CommunicationType::network)
		{
			Disconnect();
		}
	}

	void Duet::SendGcode(std::string_view gcode, bool force)
	{
		ZoneScoped;
		if (!IsConnected() && !force)
		{
			LOG_DBG("Not connected to Duet, cannot send gcode: {:s}", gcode);
			return;
		}
		LOG_DBG("Sending gcode: '{:s}'", gcode);

		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_sendLock);
			CRC16 crc;
			size_t len = 0;
			std::string_view line;

			for (size_t i = 0; i < gcode.length(); i++)
			{
				char c = gcode[i];
				if (c == '\n')
				{
					line = gcode.substr(i - len, len);
					SerialIo::Send(line);
					SerialIo::Send(fmt::format("*{:05d}\n", crc.Get()));
					len = 0;
					crc.Reset(0);
					continue;
				}
				if (len == 0)
				{
					uint32_t lineNumber = GetNextLineNumber();
					std::string lineNumberStr = fmt::format("N{:d} ", lineNumber);
					for (char line_c : lineNumberStr)
					{
						crc.Update(line_c);
					}
					SerialIo::Send(lineNumberStr);
				}
				len++;
				crc.Update(c);
			}
			if (len > 0)
			{
				line = gcode.substr(gcode.length() - len, len);
				SerialIo::Send(line);
				SerialIo::Send(fmt::format("*{:05d}\n", crc.Get()));
			}
			break;
		}
		case CommunicationType::usb:
		{
			std::lock_guard<LockableBase(std::mutex)> lock(m_sendLock);
			std::size_t usbChannel = 0;
			size_t len = 0;
			std::string_view line;
			if (StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL))
			{
				std::string_view command = gcode;
				if (const size_t first = command.find_first_not_of(" \t\r\n"); first != std::string_view::npos)
				{
					command.remove_prefix(first);
				}
				if (const size_t commandEnd = command.find_first_of(" \t\r\n"); commandEnd != std::string_view::npos)
				{
					command = command.substr(0, commandEnd);
				}

				static constexpr std::string_view channelOneCommands[] = {
					"M409",
					"M112",
					"M999",
					"M111",
					"M122",
					"M108",
					"M25",
				};
				for (const std::string_view channelOneCommand : channelOneCommands)
				{
					if (command == channelOneCommand)
					{
						usbChannel = 1;
						break;
					}
				}
			}

			// Each line goes in one transfer, numbered and ended: sent in parts, a connection lost between them would
			// leave the start of a line at the Duet for the next line to be joined onto
			const auto sendLine = [this, usbChannel](std::string_view text)
			{ sendUsbData(fmt::format("N{:d} {:s}\n", GetNextLineNumber(), text), usbChannel); };
			for (size_t i = 0; i < gcode.length(); i++)
			{
				char c = gcode[i];
				if (c == '\n')
				{
					if (len > 0) // an empty line says nothing
					{
						line = gcode.substr(i - len, len);
						sendLine(line);
					}
					len = 0;
					continue;
				}
				len++;
			}
			if (len > 0)
			{
				line = gcode.substr(gcode.length() - len, len);
				sendLine(line);
			}
			break;
		}
		case CommunicationType::simulator:
#if DUET_SIMULATOR
			Sim::Simulator::get().send(gcode);
#endif
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["gcode"] = gcode;
			AsyncGet("/rr_gcode",
					 query,
					 [gcode = std::string(gcode)](const HttpResponsePtr& r)
					 {
						 if (r->status_code != HTTP_STATUS_OK)
						 {
							 LOG_ERROR("HTTP error {:d}: Failed to send gcode: {:s}", (int)r->status_code, gcode);
							 return false;
						 }
						 return true;
					 });
			break;
		}
		}
	}

	bool Duet::UploadFile(std::string_view filename, const std::string& contents)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot upload file {:s}", filename);
			return false;
		}

		LOG_INFO("Uploading file {:s}: {:d} bytes", filename, contents.size());
		// TODO add sleep

		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
		{
			/* UART is too slow to support uploading files */
			if (m_config.communicationType == CommunicationType::uart && contents.size() > MAX_UART_UPLOAD_SIZE)
			{
				LOG_WARN(
					"File too large ({:d}) to upload via UART, limit is {:d}", contents.size(), MAX_UART_UPLOAD_SIZE);
				return false;
			}

			/**
			 * TODO once DSF support M559, we should use that as it will be more efficient than echo which opens,
			 * writes, and closes the file for each line.
			 *
			 * Will need to find a way to prevent other gcode commands from being interleaved with the upload if using
			 * M559, possibly by pausing the screen from sending any other commands while the file upload is in
			 * progress.
			 */

			SendGcodef("M30 \"{:s}\"\n", filename); // delete file
			size_t prevPosition = 0;
			size_t position = contents.find("\n"); // Find the first occurrence of \n
			std::string line;
			while (prevPosition <= contents.size())
			{
				if (position == std::string::npos)
				{
					line = contents.substr(prevPosition);
				}
				else
				{
					line = contents.substr(prevPosition, position - prevPosition);
				}

				utils::replaceSubstring(line, "\"", "\"\"");
				SendGcodef("echo >>\"{:s}\" \"{:s}\"\n", filename, line);

				if (position == std::string::npos)
				{
					break;
				}

				prevPosition = position + 1;
				position = contents.find("\n", prevPosition); // Find the next occurrence, if any
			}
			// SendGcode("M29\n");
			break;
		}
		case CommunicationType::network:
		{
			HttpResponse r;
			hv::QueryParams query;
			query["name"] = filename;
			if (!Post("/rr_upload", r, query, contents))
			{
				LOG_ERROR("HTTP error {:d} {:s}: Failed to upload file: {:s}", (int)r.status_code, r.body, filename);
				return false;
			}
			break;
		}
		}
		return true;
	}

	bool Duet::DownloadFile(std::string_view filename, std::function<void(const std::string&)> onComplete)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot download file {:s}", filename);
			return false;
		}

		LOG_INFO("Downloading file {}", filename);
		switch (m_config.communicationType)
		{
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["name"] = filename;
			if (!AsyncGet("/rr_download",
						  query,
						  [cb = std::move(onComplete), filename = std::string(filename)](const HttpResponsePtr& r)
						  {
							  if (r->status_code != HTTP_STATUS_OK)
							  {
								  LOG_ERROR(
									  "HTTP error {:d}: Failed to download file: {}", (int)r->status_code, filename);
								  return false;
							  }
							  cb(r->body);
							  return true;
						  }))
			{
				LOG_ERROR("Failed to download file {:s}", filename);
				return false;
			}
			break;
		}
		default:
			LOG_WARN("Communication type not supported for downloading files");
			return false;
		}
		return true;
	}

	bool Duet::DeleteFile(std::string_view filename)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot delete file {:s}", filename);
			return false;
		}

		LOG_INFO("Deleting file {}", filename);
		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M30 \"{:s}\"\n", filename);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["name"] = filename;
			if (!AsyncGet("/rr_delete",
						  query,
						  [filename = std::string(filename)](const HttpResponsePtr& r)
						  {
							  if (r->status_code != HTTP_STATUS_OK)
							  {
								  LOG_ERROR(
									  "HTTP error {:d}: Failed to delete file: {}", (int)r->status_code, filename);
								  return false;
							  }
							  return true;
						  }))
			{
				LOG_ERROR("Failed to delete file {:s}", filename);
				return false;
			}
			break;
		}
		}
		return true;
	}

	void Duet::RequestModel(std::string_view flags)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request model with flags: {:s}", flags);
			return;
		}

		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M409 F\"{:s}\"\n", flags);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["flags"] = flags;
			HttpResponse r;
			Get("/rr_model", r, query);
			receiveModelBody(r, fmt::format("flags {:s}", flags));
			break;
		}
		}
	}

	void Duet::RequestModel(std::string_view key, std::string_view flags)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request model with key: {:s}, flags: {:s}", key, flags);
			return;
		}

		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M409 K\"{:s}\" F\"{:s}\"\n", key, flags);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["key"] = key;
			query["flags"] = flags;
			HttpResponse r;
			Get("/rr_model", r, query);
			receiveModelBody(r, fmt::format("key {:s}, flags {:s}", key, flags));
			break;
		}
		}
		return;
	}

	bool Duet::RequestFileList(std::string_view dir, const size_t first)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request file list for dir: {:s}", dir);
			return false;
		}

		bool ret = true;
		LOG_DBG("dir = {:s}, first = {:d}", dir, first);
		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M20 S3 P\"{:s}\" R{:d}\n", dir, first);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["dir"] = dir;
			query["first"] = fmt::format("{:d}", first);
			ret = AsyncGet(
				"/rr_filelist",
				query,
				[dir = std::string(dir)](const HttpResponsePtr& r) -> bool
				{
					OM::FileSystem::FileListArrived(
						dir, readJsonBody(*r).and_then([](const OM::Node& json) { return FileList::read(json); }));
					return true;
				});
			break;
		}
		}
		return ret;
	}

	bool Duet::RequestFileInfo(std::string_view filename)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request file info for file: {:s}", filename);
			return false;
		}

		LOG_DBG("for {:s}", filename);
		bool ret = true;
		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M36 \"{:s}\"\n", filename);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["name"] = filename;
			ret = AsyncGet(
				"/rr_fileinfo",
				query,
				[name = std::string(filename)](const HttpResponsePtr& r) -> bool
				{
					FILEINFO_CACHE->FileInfoArrived(
						name,
						readJsonBody(*r).and_then([](const OM::Node& json) { return FileInfoReply::read(json); }));
					return true;
				});
			break;
		}
		}
		return ret;
	}

	bool Duet::RequestThumbnail(std::string_view filename, uint32_t offset)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request thumbnail for file: {:s}", filename);
			return false;
		}

		LOG_DBG("for {:s}, offset={:d}", filename, offset);
		bool ret = true;
		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		case CommunicationType::usb:
		case CommunicationType::simulator:
			SendGcodef("M36.1 P\"{:s}\" S{:d}\n", filename, offset);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["name"] = filename;
			query["offset"] = fmt::format("{:d}", offset);
			ret = AsyncGet("/rr_thumbnail",
						   query,
						   [name = std::string(filename)](const HttpResponsePtr& r) -> bool
						   {
							   FILEINFO_CACHE->ThumbnailArrived(
								   name,
								   readJsonBody(*r).and_then([](const OM::Node& json)
															 { return FileChunk::read(json, "thumbnail"); }));
							   return true;
						   });
			break;
		}
		}
		return ret;
	}

	void Duet::ProcessReply(HttpResponse& reply)
	{
		ZoneScoped;
		if (m_config.communicationType != CommunicationType::network)
			return;

		if (reply.body.empty())
		{
			LOG_DBG("Empty reply received");
			return;
		}

		if (reply.body.front() == '{')
		{
			receiveLine(reply.body);
			return;
		}

		// rr_reply gathers the responses since it was last asked: each line is one, as over USB and UART
		std::string_view rest = reply.body;
		while (!rest.empty())
		{
			const size_t end = rest.find('\n');
			showResponse(rest.substr(0, end));
			rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
		}
	}

	void Duet::RequestReply(HttpResponse& r)
	{
		ZoneScoped;
		if (!IsConnected())
		{
			LOG_DBG("Not connected to Duet, cannot request reply");
			return;
		}

		hv::QueryParams query;
		Get("/rr_reply", r, query);
	}

	bool Duet::Connect(bool useSessionKey)
	{
		ZoneScoped;
		Disconnect();
		bool ret = false;
		m_connectionState = ConnectionState::CONNECTING;

		OM::SetStatus(OM::PrinterStatus::connecting);

		LOG_DBG("Connecting to Duet, communication type: {:s}", nameof::nameof_enum(m_config.communicationType));

		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
		{
			LOG_INFO("Opening UART {:s} at {:d}", DEFAULT_UART_PORT, GetBaudRate().rate);
			ret = SerialIo::Init(DEFAULT_UART_PORT, GetBaudRate().internal);
			if (!ret)
			{
				LOG_ERROR("Failed to open UART {:s} at {:d}", DEFAULT_UART_PORT, GetBaudRate().rate);
				break;
			}

			SerialIo::SetDataCallback(
				[this](const std::string_view data)
				{
					UNUSED(data);
					m_connectionState = ConnectionState::CONNECTED;
					SerialIo::RestoreDataCallback();
					Model::get().post<EventType::Connected>();
				});
			SendGcode("M115", true); // arbitrary command to trigger a response
			break;
		}
		case CommunicationType::network:
		{
			LOG_INFO("Connecting to Duet at {:s}", GetBaseUrl());

			hv::QueryParams query;
			query["password"] = m_config.password;
			if (useSessionKey)
				query["sessionKey"] = "yes";

			ret = AsyncGet("/rr_connect",
						   query,
						   [this](const HttpResponsePtr& r)
						   {
							   if (r->status_code != HTTP_STATUS_OK)
							   {
								   LOG_ERROR("rr_connect failed, returned response {:d}", (int)r->status_code);
								   m_connectionState = ConnectionState::DISCONNECTED;
								   return;
							   }

							   LOG_VERBOSE("parsing rr_connect response");
							   auto body = nlohmann::json::parse(r->body, nullptr, false);
							   if (body.is_discarded())
							   {
								   LOG_ERROR("Failed to parse JSON response from rr_connect");
								   m_connectionState = ConnectionState::DISCONNECTED;
								   return;
							   }

							   if (body.contains("err") && body["err"].get<int>() != 0)
							   {
								   LOG_ERROR("rr_connect failed, returned error {:d}", body["err"].get<int>());
								   m_connectionState = ConnectionState::DISCONNECTED;
								   return;
							   }

							   if (body.contains("sessionTimeout"))
							   {
								   m_sessionTimeout = std::chrono::milliseconds(body["sessionTimeout"].get<int>());
								   m_lastRequestTime = TimeHelper::getRunningTime();
								   LOG_INFO("Duet session timeout set to {}", m_sessionTimeout);
							   }

							   if (body.contains("sessionKey"))
							   {
								   SetSessionKey(body["sessionKey"].get<unsigned int>());
								   LOG_INFO("Duet session key = {:d}", m_sessionKey);
							   }
							   if (body.contains("isEmulated"))
							   {
								   SetSessionKey(sm_noSessionKey);
								   m_sbcMode = true;
								   LOG_INFO("Connected to Duet in SBC mode");
							   }
							   LOG_INFO("rr_connect succeeded");
							   OM::SetChannelIndex(
								   0); // state.thisInput is not returned by `rr_model` so manually set here
							   m_connectionState = ConnectionState::CONNECTED;
							   Model::get().post<EventType::Connected>();
							   return;
						   });

			break;
		}
		case CommunicationType::simulator:
		{
#if DUET_SIMULATOR
			ret = Sim::Simulator::get().connect(
				[framer = std::make_shared<LineFramer>(receiveLine)](std::string_view reply) { framer->push(reply); });
			if (ret)
			{
				m_connectionState = ConnectionState::CONNECTED;
			}
#endif
			break;
		}
		case CommunicationType::usb:
		{
			LOG_DBG("Attempting to connect to Duet via USB");
			ret = connectUsbDevice();
			if (ret)
			{
				LOG_DBG("Connected to USB device");
				m_connectionState = ConnectionState::CONNECTED; // set connected state so SendGcode actually works
				SendGcode("M575 P0 S0\n",
						  true); // set serial comm parameters for USB port to use JSON responses (no CRC as USB already
								 // has error checking), this also serves as a test command to verify that the
								 // connection is working
				if (StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL))
				{
					SendGcode("M575 P1 S0\n", true); // set second USB channel parameters
				}
			}
			else
			{
				LOG_ERROR("Failed to connect to Duet via USB");
			}
			break;
		}
		}

		if (!ret)
		{
			m_connectionState = ConnectionState::DISCONNECTED;
			return ret;
		}

		/* Only for USB and the simulator, UART & network is async */
		if (IsConnected())
		{
			Model::get().post<EventType::Connected>();
		}

		return ret;
	}

	bool Duet::Disconnect()
	{
		ZoneScoped;
		if (m_connectionState == ConnectionState::DISCONNECTED)
		{
			return true;
		}

		LOG_INFO("Disconnecting from Duet");

		bool ret = false;
		switch (m_config.communicationType)
		{
		case CommunicationType::uart:
			SerialIo::Shutdown();
			ret = true;
			break;
		case CommunicationType::network:
		{
			if (m_sessionKey == sm_noSessionKey)
			{
				break;
			}
			hv::QueryParams query;
			ret = AsyncGet("/rr_disconnect",
						   query,
						   [](const HttpResponsePtr& r)
						   {
							   if (r->status_code != HTTP_STATUS_OK)
							   {
								   LOG_ERROR("rr_disconnect failed, returned response {:d}", (int)r->status_code);
								   return;
							   }
						   });

			if (!ret)
			{
				LOG_ERROR("rr_disconnect failed");
			}
			break;
		}
		case CommunicationType::usb:
		{
			getCurrentUsbDevice().reset();
			ret = true;
			break;
		}
		case CommunicationType::simulator:
#if DUET_SIMULATOR
			Sim::Simulator::get().disconnect();
#endif
			ret = true;
			break;
		}

		Reset();
		bool wasConnected = IsConnected();
		m_connectionState = ConnectionState::DISCONNECTED;
		if (wasConnected)
		{
			Model::get().post<EventType::Disconnected>();
		}
		return ret;
	}

	const std::string& Duet::GetBaseUrl() const
	{
		ZoneScoped;
		LOG_VERBOSE("Using IP address {:s}", m_config.ipAddress.c_str());
		return m_config.ipAddress;
	}

	void Duet::SetBaudRate(const speed_t baudRateCode)
	{
		ZoneScoped;
		for (size_t i = 0; i < std::size(baudRates); i++)
		{
			if (baudRates[i].internal == baudRateCode)
			{
				SetBaudRate(baudRates[i]);
				return;
			}
		}
		LOG_WARN("Baud rate {:d} not found", baudRateCode);
	}

	void Duet::SetBaudRate(const baudrate_t& baudRate)
	{
		ZoneScoped;
		LOG_INFO("Setting baud rate to {:d} ({:d})", baudRate.rate, baudRate.internal);
		SerialIo::SetBaudRate(baudRate.internal);
		m_config.baudRate = baudRate.internal;
		StorageHelper::setData(ID_DUET_BAUD_RATE, baudRate.internal);
	}

	const baudrate_t& Duet::GetBaudRate() const
	{
		ZoneScoped;
		for (auto& baud : baudRates)
		{
			if (baud.internal == m_config.baudRate)
			{
				return baud;
			}
		}
		LOG_WARN("Baud rate {:d} not found", m_config.baudRate);
		return baudRates[0];
	}

	void Duet::SetIPAddress(std::string_view ipAddress)
	{
		ZoneScoped;
		LOG_INFO("Setting IP address to {:s}", ipAddress);
		if (m_config.ipAddress == ipAddress)
		{
			return;
		}

		m_config.ipAddress = ipAddress;
		StorageHelper::setData(ID_DUET_IP_ADDRESS, ipAddress);
		if (m_config.communicationType == CommunicationType::network)
		{
			Connect();
		}
	}

	const std::string& Duet::GetIPAddress() const
	{
		ZoneScoped;
		return m_config.ipAddress;
	}

	void Duet::ClearIPAddress()
	{
		ZoneScoped;
		m_config.ipAddress.clear();
		LOG_DBG("IP address cleared \"{:s}\"", m_config.ipAddress.c_str());
	}

	void Duet::SetPassword(std::string_view password)
	{
		ZoneScoped;
		LOG_INFO("Setting Duet password");
		m_config.password = password;
		StorageHelper::setData(ID_DUET_PASSWORD, password);
		if (m_config.communicationType == CommunicationType::network)
		{
			Connect();
		}
	}

	const std::string& Duet::GetPassword() const
	{
		ZoneScoped;
		return m_config.password;
	}

	void Duet::SetSessionKey(const uint32_t key)
	{
		ZoneScoped;
		m_sessionKey = key;
		LOG_INFO("Set Duet session key = {:d}", m_sessionKey);
	}
} // namespace Comm

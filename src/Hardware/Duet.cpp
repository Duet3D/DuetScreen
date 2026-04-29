/*
 * Duet.cpp
 *
 *  Created on: 26 Jan 2024
 *      Author: Andy Everitt
 */

#include "Comm/Communication.h"
#include "Comm/FileInfo.h"
#include "Comm/JsonDecoder.h"
#include "Comm/Usb.h"
#include "Debug.h"

#include "Duet.h"

#include "Duet3D/General/CRC16.h"
#include "Hardware/SerialIo.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Utils.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "nameof.hpp"
#include "utils/StorageHelper.h"
#include "utils/TimeHelper.h"
#include "utils/utils.h"
#include <map>
#include <string>

namespace Comm
{
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
			// TODO: Determine why session key isn't working
			// req.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey).c_str();
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

#if 0
		if (((!m_sbcMode && m_sessionKey == sm_noSessionKey) ||
			 (TimeHelper::getCurrentTime() - m_lastRequestTime > m_sessionTimeout)) &&
			(path != "/rr_connect" && path != "/rr_disconnect"))
		{
			LOG_DBG("Session expired, reconnecting. Request '{:s}' will not be sent", path);
			Reconnect();
			return false;
		}
#endif

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

#if 0
		if (((!m_sbcMode && m_sessionKey == sm_noSessionKey) ||
			 (TimeHelper::getCurrentTime() - m_lastRequestTime > m_sessionTimeout)) &&
			(path != "/rr_connect" && path != "/rr_disconnect"))
		{
			LOG_DBG("Session expired, reconnecting. Request '{:s}' will not be sent", path);
			Reconnect();
			return false;
		}
#endif

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
			// req.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey).c_str();
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

			auto send_cb = [usbChannel](std::string_view payload) { return sendUsbData(payload, usbChannel); };
			for (size_t i = 0; i < gcode.length(); i++)
			{
				char c = gcode[i];
				if (c == '\n')
				{
					line = gcode.substr(i - len, len);
					send_cb(line);
					send_cb("\n");
					len = 0;
					continue;
				}
				if (len == 0)
				{
					uint32_t lineNumber = GetNextLineNumber();
					std::string lineNumberStr = fmt::format("N{:d} ", lineNumber);
					send_cb(lineNumberStr);
				}
				len++;
			}
			if (len > 0)
			{
				line = gcode.substr(gcode.length() - len, len);
				send_cb(line);
				send_cb("\n");
			}
			break;
		}
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["gcode"] = gcode;
			AsyncGet("/rr_gcode",
					 query,
					 [gcode](const HttpResponsePtr& r)
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
#if 1
			HttpResponse r;
			hv::QueryParams query;
			query["name"] = filename;
			if (!AsyncGet("/rr_download",
						  query,
						  [cb = std::move(onComplete), filename](const HttpResponsePtr& r)
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
#endif
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
			SendGcodef("M30 \"{:s}\"\n", filename);
			break;
		case CommunicationType::network:
		{
			HttpResponse r;
			hv::QueryParams query;
			query["name"] = filename;
			if (!AsyncGet("/rr_delete",
						  query,
						  [filename](const HttpResponsePtr& r)
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
			SendGcodef("M409 F\"{:s}\"\n", flags);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["flags"] = flags;
#if ASYNC_RR_MODEL
			AsyncGet("/rr_model",
					 query,
					 [flags](const HttpResponsePtr& r)
					 {
						 JsonDecoder decoder;
						 if (r->status_code != HTTP_STATUS_OK)
						 {
							 LOG_ERROR("HTTP error {:d}: Failed to get model update for flags: {:s}",
									   (int)r->status_code,
									   flags);
							 return false;
						 }
						 decoder.CheckInput((const unsigned char*)r->body.c_str(), r->body.length() + 1);
						 return true;
					 });
#else
			HttpResponse r;
			Get("/rr_model", r, query);
			JsonDecoder decoder;
			if (r.status_code != HTTP_STATUS_OK)
			{
				LOG_ERROR("HTTP error {:d}: Failed to get model update for flags: {:s}", (int)r.status_code, flags);
				break;
			}
			decoder.CheckInput((const unsigned char*)r.body.c_str(), r.body.length() + 1);
#endif
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
			SendGcodef("M409 K\"{:s}\" F\"{:s}\"\n", key, flags);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["key"] = key;
			query["flags"] = flags;
#if ASYNC_RR_MODEL
			AsyncGet("/rr_model",
					 query,
					 [key, flags](const HttpResponsePtr& r)
					 {
						 JsonDecoder decoder;
						 if (r->status_code != HTTP_STATUS_OK)
						 {
							 LOG_ERROR("HTTP error {:d}: Failed to get model update for key: {:s}, flags: {:s}",
									   (int)r->status_code,
									   key,
									   flags);
							 return false;
						 }
						 decoder.CheckInput((const unsigned char*)r->body.c_str(), r->body.length() + 1);
						 return true;
					 });
#else
			HttpResponse r;
			Get("/rr_model", r, query);
			JsonDecoder decoder;
			if (r.status_code != HTTP_STATUS_OK)
			{
				LOG_ERROR("HTTP error {:d}: Failed to get model update for key: {:s}, flags: {:s}",
						  (int)r.status_code,
						  key,
						  flags);
				break;
			}
			decoder.CheckInput((const unsigned char*)r.body.c_str(), r.body.length() + 1);
#endif
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
			SendGcodef("M20 S3 P\"{:s}\" R{:d}\n", dir, first);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["dir"] = dir;
			query["first"] = fmt::format("{:d}", first);
			ret = AsyncGet("/rr_filelist",
						   query,
						   [dir](const HttpResponsePtr& r) -> bool
						   {
							   JsonDecoder decoder;
							   if (r->status_code != 200)
							   {
								   LOG_ERROR("HTTP error {:d} ({:s}): Failed to get file list for {:s}",
											 (int)r->status_code,
											 r->status_message(),
											 dir);
								   return false;
							   }
							   decoder.CheckInput((const unsigned char*)r->body.c_str(), r->body.length() + 1);
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
			SendGcodef("M36 \"{:s}\"\n", filename);
			break;
		case CommunicationType::network:
		{
			hv::QueryParams query;
			query["name"] = filename;

#if 1
			ret = AsyncGet("/rr_fileinfo",
						   query,
						   [](const HttpResponsePtr& r) -> bool
						   {
							   JsonDecoder decoder;
							   if (r->status_code != 200)
							   {
								   LOG_ERROR("HTTP error {:d}: Failed to get file info for file: {:s}",
											 (int)r->status_code,
											 r->body.c_str());
								   return false;
							   }
							   decoder.CheckInput((const unsigned char*)r->body.c_str(), r->body.length() + 1);
							   return true;
						   });

			break;
#endif

/* This way is quicker but duplicates code to decode the received data */
#if 0
			std::string name(filename);
			ret = AsyncGet(
				"/rr_fileinfo",
				query,
				[this, name](HttpResponse& r) -> bool {
					Json::Reader reader;
					Json::Value body;
					const char* filename = name.c_str();
					LOG_DBG("Name = {:s}", filename);
					if (r.code != 200)
					{
						LOG_ERROR("HTTP error {:d}: Failed to get file info for file: {:s}", (int)r.code, r.body);
						return false;
					}
					reader.parse(r.body, body);
					if (body.isMember("err") && body["err"].asInt() != 0)
					{
						LOG_ERROR("Failed to get file info for file: {:s}, returned error {:d}", r.body, body["err"].asInt());
						return false;
					}
					if (!body.isMember("thumbnails"))
					{
						LOG_INFO("No thumbnails found for {:s}", filename);
						return false;
					}
					Json::Value thumbnailsJson = body["thumbnails"];
					for (Json::ArrayIndex i = 0; i < thumbnailsJson.size(); i++)
					{
						Thumbnail thumbnail;
						ThumbnailContext context;
						ThumbnailInit(thumbnail);
						if (!thumbnailsJson[i].isMember("width"))
						{
							continue;
						}
						thumbnail.meta.width = thumbnailsJson[i]["width"].asInt();

						if (!thumbnailsJson[i].isMember("height"))
						{
							continue;
						}
						thumbnail.meta.height = thumbnailsJson[i]["height"].asInt();

						if (!thumbnailsJson[i].isMember("offset"))
						{
							continue;
						}
						context.next = thumbnailsJson[i]["offset"].asInt();

						if (!thumbnailsJson[i].isMember("format"))
						{
							continue;
						}
						std::string format = thumbnailsJson[i]["format"].asString();
						if (!thumbnail.meta.SetImageFormat(format.c_str()))
						{
							LOG_WARN("Unsupported thumbnail format: {:s}", format.c_str());
							continue;
						}
						thumbnail.image.New(thumbnail.meta, filename);

						LOG_INFO("File {:s} has thumbnail {:d}: {:d}x{:d}", filename, i, thumbnail.width, thumbnail.height);

						hv::QueryParams query;
						query["name"] = filename;
						while (context.next != 0)
						{
							// Request thumbnail data
							query["offset"] = fmt::format("{:d}", context.next);
							LOG_INFO("Requesting thumbnail data for {:s} at offset {:d}\n", filename, context.next);
							if (!Get("/rr_thumbnail", r, query))
							{
								LOG_ERROR("Failed to get thumbnail data for {:s} at offset {:d}", filename, context.next);
								continue;
							}
							LOG_DBG("Parsing rr_thumbnail response");
							reader.parse(r.body, body);

							if (body.isMember("err") && body["err"].asInt() != 0)
							{
								LOG_ERROR("Failed to get thumbnail data for {:s} at offset {:d}: {:d}",
									  filename,
									  context.next,
									  body["err"].asInt());
								continue;
							}

							if (body.isMember("next"))
							{
								context.next = body["next"].asInt();
								LOG_DBG("Next thumbnail offset: {:d}", context.next);
							}

							LOG_DBG("Decoding thumbnail data");
							if (body.isMember("data"))
							{
								ThumbnailBuf data;
								data.size = std::min(body["data"].asString().size(), sizeof(data.buffer));
								memcpy(data.buffer, body["data"].asString().c_str(), data.size);
								ThumbnailDecodeChunk(thumbnail, data);
							}
						}
						thumbnail.Close();
						OM::FileSystem::GetListView()->refreshListView();
					}
					return true;
				});
			break;
#endif
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
			SendGcodef("M36.1 P\"{:s}\" S{:d}\n", filename, offset);
			break;
		case CommunicationType::network:
		{
#if 1
			hv::QueryParams query;
			query["name"] = filename;
			query["offset"] = fmt::format("{:d}", offset);
			ret = AsyncGet("/rr_thumbnail",
						   query,
						   [](const HttpResponsePtr& r) -> bool
						   {
							   JsonDecoder decoder;
							   if (r->status_code != 200)
							   {
								   LOG_ERROR("HTTP error {:d}: Failed to get thumbnail for file: {:s}",
											 (int)r->status_code,
											 r->body.c_str());
								   return false;
							   }
							   decoder.SetPrefix("thumbnail:");
							   decoder.CheckInput((const unsigned char*)r->body.c_str(), r->body.size() + 1);
							   return true;
						   });
#endif
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

		JsonDecoder decoder;
		if (reply.body[0] != '{')
		{
			LOG_DBG("Reply not json: assuming it is a gcode response");

			LOG_DBG("Removing \\r from reply body");
			utils::removeCharFromString(reply.body, '\r');
			size_t prevPosition = 0;
			size_t position = reply.body.find("\n"); // Find the first occurrence of \n

			// Split reply by new line and handle each as its own response.
			// The replicates the uart behaviour and is required because rr_reply will group multiple
			// replies together into a single response.
			StringRef ref((char*)"resp", 5);
			size_t indices[MAX_ARRAY_NESTING] = {0};
			while (position != std::string::npos)
			{
				std::string line =
					reply.body.substr(prevPosition, position - prevPosition); // `ProcessReceivedValue()` needs
																			  // null terminated string
				LOG_DBG("line: {:s}", line);
				prevPosition = position + 1;
				position = reply.body.find("\n", position + 1); // Find the next occurrence, if any
				LOG_VERBOSE("position={:d}, prevPosition={:d}", position, prevPosition);
				if (line.empty())
				{
					LOG_VERBOSE("Skipping empty line");
					continue;
				}
				// Can skip checking the input since we know it's a gcode response
				decoder.ProcessReceivedValue(ref, line.data(), indices);
			}
			return;
		}
		decoder.CheckInput((const unsigned char*)reply.body.c_str(), reply.body.length() + 1);
		return;
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
			query["password"] = std::string("\"") + m_config.password + "\"";
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

		/* Only for USB, UART & network is async */
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

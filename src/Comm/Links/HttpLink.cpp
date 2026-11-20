/*
 * HttpLink.cpp
 */

#include "HttpLink.h"
#include "Comm/Lines.h"
#include "Comm/Responses.h"
#include "Configuration.h"
#include "Debug.h"
#include "tracy/Tracy.hpp"
#include <fmt/format.h>

namespace Comm
{
	namespace
	{
		/** The password RepRapFirmware takes when none is set; DSF refuses an empty one */
		constexpr std::string_view defaultPassword = "reprap";

		/** How long a file may take to come or go, seconds */
		constexpr uint16_t fileTransferTimeout = 180;

		/** A handler of a request's JSON that reads it as `T` with `read` */
		template <typename T, typename Read>
		HttpRequester::JsonHandler readAs(Handler<T> handler, Read read)
		{
			return [handler = std::move(handler), read](std::expected<OM::Node, ReplyError> json)
			{
				ZoneScopedN("readAs_cb");
				handler(json.and_then([&read](const OM::Node& node) { return read(node); }));
			};
		}

		/** A handler of a request's JSON that tells `handler` whether the firmware's "err" says it worked */
		HttpRequester::JsonHandler firmwareSucceeded(Handler<void> handler)
		{
			return [handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
			{
				ZoneScopedN("firmwareSucceeded_cb");
				if (!json)
				{
					handler(std::unexpected(json.error()));
				}
				else if (const auto error = firmwareError(*json))
				{
					handler(std::unexpected(*error));
				}
				else
				{
					handler({});
				}
			};
		}
	} // namespace

	HttpLink::HttpLink(std::string host, std::string password, HttpRequester::Post post, UnaskedHandler unasked)
		: m_password(password.empty() ? std::string(defaultPassword) : std::move(password))
		, m_unasked(std::move(unasked))
		, m_http(std::move(host), std::move(post), maxInFlight)
	{
	}

	void HttpLink::close()
	{
		ZoneScoped;
		m_http.close(m_sessionKey != 0 ? "/rr_disconnect" : "");
	}

	void HttpLink::open(Handler<Session> handler)
	{
		ZoneScoped;
		LOG_INFO("Connecting to the Duet at {:s}", m_http.host());
		hv::QueryParams query;
		query["password"] = m_password;
		query["sessionKey"] = "yes";
		m_http.getJson("/rr_connect",
					   std::move(query),
					   [this, handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
					   {
						   ZoneScopedN("rr_connect_cb");
						   if (!json)
						   {
							   handler(std::unexpected(json.error()));
							   return;
						   }
						   if (const auto error = firmwareError(*json))
						   {
							   // 1: the password is wrong; 2: the Duet has no session free
							   handler(std::unexpected(*error));
							   return;
						   }
						   m_sessionKey = static_cast<uint32_t>((*json)["sessionKey"].integer(0));
						   if (m_sessionKey != 0)
						   {
							   m_http.setHeader("X-Session-Key", fmt::format("{:d}", m_sessionKey));
						   }
						   handler(Session{.sbc = (*json)["isEmulated"].boolean()});
					   });
	}

	void HttpLink::requestModel(std::string_view key, std::string_view flags, ModelHandler handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["key"] = key;
		query["flags"] = flags;
		m_http.getJson("/rr_model", std::move(query), std::move(handler));
	}

	void HttpLink::fetchResponses()
	{
		ZoneScoped;
		m_http.send({.path = "/rr_reply", .timeout = HTTP_TIMEOUT},
					[this](const HttpRequester::Response& response)
					{
						ZoneScopedN("fetchResponses_cb");
						const auto text = HttpRequester::text(response);
						if (!text || text->empty())
						{
							return;
						}
						if (text->front() == '{')
						{
							auto json = HttpRequester::json(*text);
							if (json)
							{
								m_unasked(*json);
							}
							return;
						}
						// The responses since rr_reply was last asked, a line each
						forEachLine(*text, EmptyLines::skip, [](std::string_view line) { showResponse(line); });
					});
	}

	void HttpLink::sendGcode(std::string_view gcode)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["gcode"] = gcode;
		m_http.getJson("/rr_gcode",
					   std::move(query),
					   [gcode = std::string(gcode)](std::expected<OM::Node, ReplyError> json)
					   {
						   ZoneScopedN("sendGcode_cb");
						   if (!json)
						   {
							   LOG_ERROR("Failed to send \"{:s}\": {:s}", gcode, json.error().message);
						   }
					   });
	}

	void HttpLink::emergencyStop()
	{
		ZoneScoped;
		hv::QueryParams query;
		query["gcode"] = "M112 M999";
		m_http.send({.path = "/rr_gcode",
					 .query = std::move(query),
					 .timeout = HTTP_TIMEOUT,
					 .lane = HttpRequester::Lane::urgent},
					[](const HttpRequester::Response& response)
					{
						ZoneScopedN("emergencyStop_cb");
						if (const auto text = HttpRequester::text(response); !text)
						{
							LOG_ERROR("The emergency stop wasn't taken: {:s}", text.error().message);
						}
					});
	}

	void HttpLink::listFiles(std::string_view dir, size_t first, Handler<FileList> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["dir"] = dir;
		query["first"] = fmt::format("{:d}", first);
		m_http.getJson("/rr_filelist",
					   std::move(query),
					   readAs(std::move(handler), [](const OM::Node& json) { return FileList::read(json); }));
	}

	void HttpLink::fileInfo(std::string_view file, Handler<FileInfo> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.getJson("/rr_fileinfo",
					   std::move(query),
					   readAs(std::move(handler), [](const OM::Node& json) { return FileInfo::read(json); }));
	}

	void HttpLink::thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		query["offset"] = fmt::format("{:d}", offset);
		m_http.getJson(
			"/rr_thumbnail",
			std::move(query),
			readAs(std::move(handler), [](const OM::Node& json) { return FileChunk::read(json, "thumbnail"); }));
	}

	void HttpLink::readFile(std::string_view file, Handler<std::string> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.send({.path = "/rr_download",
					 .query = std::move(query),
					 .timeout = fileTransferTimeout,
					 .lane = HttpRequester::Lane::slow},
					[handler = std::move(handler)](const HttpRequester::Response& response)
					{
						ZoneScopedN("readFile_cb");
						handler(HttpRequester::text(response));
					});
	}

	void HttpLink::writeFile(std::string_view file, std::string contents, Progress, Handler<void> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.send({.method = HTTP_POST,
					 .path = "/rr_upload",
					 .query = std::move(query),
					 .timeout = fileTransferTimeout,
					 .lane = HttpRequester::Lane::slow,
					 .body = std::move(contents)},
					[handler = firmwareSucceeded(std::move(handler))](const HttpRequester::Response& response)
					{
						ZoneScopedN("writeFile_cb");
						handler(HttpRequester::text(response).and_then([](const std::string& body)
																	   { return HttpRequester::json(body); }));
					});
	}

	void HttpLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.getJson("/rr_delete", std::move(query), firmwareSucceeded(std::move(handler)));
	}
} // namespace Comm

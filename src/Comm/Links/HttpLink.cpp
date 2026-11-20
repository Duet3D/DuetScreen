/*
 * HttpLink.cpp
 */

#include "HttpLink.h"
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
			ZoneScoped;
			return [handler = std::move(handler), read](std::expected<OM::Node, ReplyError> json)
			{ handler(json.and_then([&read](const OM::Node& node) { return read(node); })); };
		}

		/** A handler of a request's JSON that only says whether it worked ({"err":0}) */
		HttpRequester::JsonHandler succeeded(Handler<void> handler)
		{
			ZoneScoped;
			return [handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
			{
				ZoneScopedN("succeeded_cb");
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
		ZoneScoped;
	}

	HttpLink::~HttpLink()
	{
		ZoneScoped;
		if (m_sessionKey != 0 && !m_http.unauthorised())
		{
			// Waited for (briefly), as the requester goes with the link
			HttpRequest request;
			request.method = HTTP_GET;
			request.host = m_http.host();
			request.path = "/rr_disconnect";
			request.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey);
			request.timeout = 1;
			request.DumpUrl();
			HttpResponse response;
			hv::HttpClient().send(&request, &response);
		}
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
		m_http.send(HTTP_GET,
					"/rr_reply",
					{},
					HTTP_TIMEOUT,
					[this](const HttpResponsePtr& response)
					{
						ZoneScopedN("fetchResponses_cb");
						const auto text = HttpRequester::readText(response);
						if (!text || text->empty())
						{
							return;
						}
						if (text->front() == '{')
						{
							auto json = HttpRequester::readJson(response);
							if (json)
							{
								m_unasked(*json);
							}
							return;
						}
						// The responses since rr_reply was last asked, a line each
						std::string_view rest = *text;
						while (!rest.empty())
						{
							const size_t end = rest.find('\n');
							showResponse(rest.substr(0, end));
							rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
						}
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
		sendGcode("M112 M999");
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

	void HttpLink::fileInfo(std::string_view file, Handler<FileInfoReply> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.getJson("/rr_fileinfo",
					   std::move(query),
					   readAs(std::move(handler), [](const OM::Node& json) { return FileInfoReply::read(json); }));
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
		m_http.send(HTTP_GET,
					"/rr_download",
					std::move(query),
					fileTransferTimeout,
					[handler = std::move(handler)](const HttpResponsePtr& response)
					{ handler(HttpRequester::readText(response)); });
	}

	void HttpLink::writeFile(std::string_view file, std::string contents, Handler<void> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.send(
			HTTP_POST,
			"/rr_upload",
			std::move(query),
			fileTransferTimeout,
			[handler = succeeded(std::move(handler))](const HttpResponsePtr& response)
			{ handler(HttpRequester::readJson(response)); },
			std::move(contents));
	}

	void HttpLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		ZoneScoped;
		hv::QueryParams query;
		query["name"] = file;
		m_http.getJson("/rr_delete", std::move(query), succeeded(std::move(handler)));
	}
} // namespace Comm

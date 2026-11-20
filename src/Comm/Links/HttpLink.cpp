/*
 * HttpLink.cpp
 */

#include "HttpLink.h"
#include "Comm/Responses.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectModel/Tree/Json.h"
#include <fmt/format.h>

namespace Comm
{
	namespace
	{
		/** The password RepRapFirmware takes when none is set; DSF refuses an empty one */
		constexpr std::string_view defaultPassword = "reprap";

		/** How long a file may take to come or go, seconds */
		constexpr uint16_t fileTransferTimeout = 180;

		/** A request handler's JSON read as `T` by `read` */
		template <typename T, typename Read>
		std::function<void(std::expected<OM::Node, ReplyError>)> readAs(Handler<T> handler, Read read)
		{
			return [handler = std::move(handler), read](std::expected<OM::Node, ReplyError> json)
			{ handler(json.and_then([&read](const OM::Node& node) { return read(node); })); };
		}

		/** A handler for a request whose JSON only says whether it worked ({"err":0}) */
		std::function<void(std::expected<OM::Node, ReplyError>)> succeeded(Handler<void> handler)
		{
			return [handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
			{
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

	HttpLink::HttpLink(std::string host, std::string password, Post post, UnaskedHandler unasked)
		: m_host(std::move(host))
		, m_password(password.empty() ? std::string(defaultPassword) : std::move(password))
		, m_post(std::move(post))
		, m_unasked(std::move(unasked))
		, m_alive(std::make_shared<bool>(true))
		, m_lastHeard(std::chrono::steady_clock::now())
	{
	}

	HttpLink::~HttpLink()
	{
		m_alive.reset();
		if (m_sessionKey != 0 && !m_sessionLost)
		{
			// Waited for (briefly), as the client goes with the link
			HttpRequest request;
			request.method = HTTP_GET;
			request.host = m_host;
			request.path = "/rr_disconnect";
			request.headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey);
			request.timeout = 1;
			HttpResponse response;
			hv::HttpClient().send(&request, &response);
		}
	}

	void HttpLink::open(Handler<Session> handler)
	{
		LOG_INFO("Connecting to the Duet at {:s}", m_host);
		hv::QueryParams query;
		query["password"] = m_password;
		query["sessionKey"] = "yes";
		sendForJson("/rr_connect",
					std::move(query),
					[this, handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
					{
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
						handler(Session{.sbc = (*json)["isEmulated"].boolean()});
					});
	}

	void HttpLink::requestModel(std::string_view key, std::string_view flags, ModelHandler handler)
	{
		hv::QueryParams query;
		query["key"] = key;
		query["flags"] = flags;
		sendForJson("/rr_model", std::move(query), std::move(handler));
	}

	void HttpLink::fetchResponses()
	{
		send(HTTP_GET,
			 "/rr_reply",
			 {},
			 HTTP_TIMEOUT,
			 [this](const HttpResponsePtr& response)
			 {
				 if (!response || response->status_code != HTTP_STATUS_OK || response->body.empty())
				 {
					 return;
				 }
				 if (response->body.front() == '{')
				 {
					 auto json = readJson(response);
					 if (json)
					 {
						 m_unasked(*json);
					 }
					 return;
				 }
				 // The responses since rr_reply was last asked, a line each
				 std::string_view rest = response->body;
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
		hv::QueryParams query;
		query["gcode"] = gcode;
		sendForJson("/rr_gcode",
					std::move(query),
					[gcode = std::string(gcode)](std::expected<OM::Node, ReplyError> json)
					{
						if (!json)
						{
							LOG_ERROR("Failed to send \"{:s}\": {:s}", gcode, json.error().message);
						}
					});
	}

	void HttpLink::emergencyStop()
	{
		sendGcode("M112 M999");
	}

	void HttpLink::listFiles(std::string_view dir, size_t first, Handler<FileList> handler)
	{
		hv::QueryParams query;
		query["dir"] = dir;
		query["first"] = fmt::format("{:d}", first);
		sendForJson("/rr_filelist",
					std::move(query),
					readAs(std::move(handler), [](const OM::Node& json) { return FileList::read(json); }));
	}

	void HttpLink::fileInfo(std::string_view file, Handler<FileInfoReply> handler)
	{
		hv::QueryParams query;
		query["name"] = file;
		sendForJson("/rr_fileinfo",
					std::move(query),
					readAs(std::move(handler), [](const OM::Node& json) { return FileInfoReply::read(json); }));
	}

	void HttpLink::thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler)
	{
		hv::QueryParams query;
		query["name"] = file;
		query["offset"] = fmt::format("{:d}", offset);
		sendForJson(
			"/rr_thumbnail",
			std::move(query),
			readAs(std::move(handler), [](const OM::Node& json) { return FileChunk::read(json, "thumbnail"); }));
	}

	void HttpLink::readFile(std::string_view file, Handler<std::string> handler)
	{
		hv::QueryParams query;
		query["name"] = file;
		send(HTTP_GET,
			 "/rr_download",
			 std::move(query),
			 fileTransferTimeout,
			 [handler = std::move(handler)](const HttpResponsePtr& response)
			 {
				 if (!response)
				 {
					 handler(std::unexpected(ReplyError{.message = "no response from the Duet"}));
				 }
				 else if (response->status_code != HTTP_STATUS_OK)
				 {
					 handler(std::unexpected(
						 ReplyError{.message = fmt::format("HTTP error {:d}", (int)response->status_code)}));
				 }
				 else
				 {
					 handler(std::move(response->body));
				 }
			 });
	}

	void HttpLink::writeFile(std::string_view file, std::string contents, Handler<void> handler)
	{
		hv::QueryParams query;
		query["name"] = file;
		send(
			HTTP_POST,
			"/rr_upload",
			std::move(query),
			fileTransferTimeout,
			[this, handler = succeeded(std::move(handler))](const HttpResponsePtr& response)
			{ handler(readJson(response)); },
			std::move(contents));
	}

	void HttpLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		hv::QueryParams query;
		query["name"] = file;
		sendForJson("/rr_delete", std::move(query), succeeded(std::move(handler)));
	}

	void HttpLink::send(http_method method,
						std::string_view path,
						hv::QueryParams query,
						uint16_t timeout,
						ResponseHandler handler,
						std::string body)
	{
		auto request = std::make_shared<HttpRequest>();
		request->method = method;
		request->host = m_host;
		request->path = path;
		request->query_params = std::move(query);
		request->timeout = timeout;
		request->body = std::move(body);
		request->headers["Connection"] = "keep-alive";
		if (m_sessionKey != 0)
		{
			request->headers["X-Session-Key"] = fmt::format("{:d}", m_sessionKey);
		}
		m_queue.push_back({.request = std::move(request), .handler = std::move(handler)});
		sendQueued();
	}

	void HttpLink::sendForJson(std::string_view path,
							   hv::QueryParams query,
							   std::function<void(std::expected<OM::Node, ReplyError>)> handler)
	{
		send(HTTP_GET,
			 path,
			 std::move(query),
			 HTTP_TIMEOUT,
			 [this, handler = std::move(handler)](const HttpResponsePtr& response) { handler(readJson(response)); });
	}

	void HttpLink::sendQueued()
	{
		while (m_inFlight < maxInFlight && !m_queue.empty())
		{
			Queued next = std::move(m_queue.front());
			m_queue.pop_front();
			m_inFlight++;
			LOG_DBG("HTTP {:s}", next.request->path);
			m_client.sendAsync(
				next.request,
				[this, alive = std::weak_ptr<bool>(m_alive), post = m_post, handler = std::move(next.handler)](
					const HttpResponsePtr& response)
				{
					// On libhv's thread: everything else happens on the connection's
					post(
						[this, alive, handler, response]
						{
							if (!alive.lock())
							{
								return; // the link has gone
							}
							m_inFlight--;
							if (response)
							{
								m_lastHeard = std::chrono::steady_clock::now();
							}
							if (response && response->status_code == HTTP_STATUS_UNAUTHORIZED)
							{
								LOG_WARN("The Duet no longer knows the session");
								m_sessionLost = true;
							}
							handler(response);
							if (alive.lock()) // the handler may have closed the link
							{
								sendQueued();
							}
						});
				});
		}
	}

	std::expected<OM::Node, ReplyError> HttpLink::readJson(const HttpResponsePtr& response)
	{
		if (!response)
		{
			return std::unexpected(ReplyError{.message = "no response from the Duet"});
		}
		if (response->status_code != HTTP_STATUS_OK)
		{
			return std::unexpected(ReplyError{.message = fmt::format("HTTP error {:d}", (int)response->status_code)});
		}
		auto json = OM::parseJson(response->body);
		if (!json)
		{
			return std::unexpected(
				ReplyError{.message = fmt::format("a reply that isn't JSON: {:s}", json.error().message)});
		}
		return std::move(*json);
	}
} // namespace Comm

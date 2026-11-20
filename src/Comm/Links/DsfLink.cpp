/*
 * DsfLink.cpp
 */

#include "DsfLink.h"
#include "Comm/Responses.h"
#include "Configuration.h" // before libhv's TcpClient.h, whose hlog.h defines a DEFAULT_LOG_FILE macro
#include "Debug.h"
#include "ObjectModel/Tree/Json.h"
#include "ObjectModel/Tree/Patch.h"
#include "ObjectModel/Tree/Tree.h"
#include <cctype>
#include <fmt/format.h>
#include <hv/WebSocketClient.h>

namespace Comm
{
	namespace
	{
		/** How long G-code may take to run (DSF answers once it has run: a long move, heating up), seconds */
		constexpr uint16_t codeTimeout = 3600;

		/** How long a file may take to come or go, seconds */
		constexpr uint16_t fileTransferTimeout = 180;

		/** A handler of a response that only says whether it worked */
		HttpRequester::ResponseHandler succeeded(Handler<void> handler)
		{
			return [handler = std::move(handler)](const HttpResponsePtr& response)
			{
				const auto text = HttpRequester::readText(response);
				if (!text)
				{
					handler(std::unexpected(text.error()));
					return;
				}
				handler({});
			};
		}

		/** `text` with every character a URL path segment can't hold written as %XX */
		std::string escapeSegment(std::string_view text)
		{
			std::string escaped;
			for (const char c : text)
			{
				const auto byte = static_cast<unsigned char>(c);
				if (std::isalnum(byte) || c == '-' || c == '_' || c == '.' || c == '~' || c == ':')
				{
					escaped += c;
				}
				else
				{
					escaped += fmt::format("%{:02X}", byte);
				}
			}
			return escaped;
		}
	} // namespace

	DsfLink::DsfLink(std::string host, std::string password, HttpRequester::Post post)
		: m_password(std::move(password))
		, m_http(std::move(host), post, maxInFlight)
		, m_post(std::move(post))
		, m_alive(std::make_shared<bool>(true))
		, m_lastHeard(Clock::now())
		, m_lastSent(Clock::now())
	{
	}

	DsfLink::~DsfLink()
	{
		m_alive.reset();
		if (m_socket)
		{
			m_socket->close();
			m_socket.reset();
		}
		if (!m_sessionKey.empty() && !m_http.unauthorised())
		{
			// Waited for (briefly), as the requester goes with the link
			HttpRequest request;
			request.method = HTTP_GET;
			request.host = m_http.host();
			request.path = "/machine/disconnect";
			request.headers["X-Session-Key"] = m_sessionKey;
			request.timeout = 1;
			request.DumpUrl();
			HttpResponse response;
			hv::HttpClient().send(&request, &response);
		}
	}

	void DsfLink::open(Handler<void> handler)
	{
		LOG_INFO("Connecting to DSF at {:s}", m_http.host());
		hv::QueryParams query;
		if (!m_password.empty())
		{
			query["password"] = m_password;
		}
		m_http.getJson("/machine/connect",
					   std::move(query),
					   [this, handler = std::move(handler)](std::expected<OM::Node, ReplyError> json) mutable
					   {
						   if (!json)
						   {
							   handler(std::unexpected(json.error()));
							   return;
						   }
						   m_sessionKey = (*json)["sessionKey"].text();
						   if (m_sessionKey.empty())
						   {
							   handler(std::unexpected(ReplyError{.message = "DSF gave no session key"}));
							   return;
						   }
						   m_http.setHeader("X-Session-Key", m_sessionKey);
						   openSocket(std::move(handler));
					   });
	}

	void DsfLink::openSocket(Handler<void> handler)
	{
		m_opened = std::move(handler);
		m_socket = std::make_unique<hv::WebSocketClient>();
		// Its events arrive on libhv's thread: everything else happens on the connection's
		const auto onThread = [this, alive = std::weak_ptr<bool>(m_alive)](std::function<void()> task)
		{
			m_post(
				[alive, task = std::move(task)]
				{
					if (alive.lock())
					{
						task();
					}
				});
		};
		m_socket->onmessage = [this, onThread](const std::string& message)
		{ onThread([this, message] { messageArrived(message); }); };
		m_socket->onclose = [this, onThread] { onThread([this] { socketClosed(); }); };
		const std::string url = fmt::format("ws://{:s}/machine?sessionKey={:s}", m_http.host(), m_sessionKey);
		if (m_socket->open(url.c_str()) != 0)
		{
			LOG_ERROR("Couldn't open {:s}", url);
			m_closed = true;
		}
	}

	void DsfLink::messageArrived(const std::string& message)
	{
		m_lastHeard = Clock::now();
		if (message.starts_with("PONG"))
		{
			return;
		}
		auto json = OM::parseJson(message);
		if (!json)
		{
			LOG_WARN("DSF sent something that isn't JSON: {:s}", json.error().message);
		}
		else if (!m_modelReceived)
		{
			OM::Tree::get().apply(OM::Patch::wholeModel(*json));
			m_modelReceived = true;
		}
		else
		{
			OM::Tree::get().apply(OM::Patch::liveModel(*json));
		}
		sendOnSocket("OK\n"); // DSF sends the next change once this one is taken
		if (m_opened && m_modelReceived)
		{
			Handler<void> opened = std::move(m_opened);
			m_opened = nullptr;
			opened({});
		}
	}

	void DsfLink::socketClosed()
	{
		LOG_WARN("DSF closed the socket");
		m_closed = true;
		if (m_opened)
		{
			Handler<void> opened = std::move(m_opened);
			m_opened = nullptr;
			opened(std::unexpected(ReplyError{.message = "DSF closed the socket"}));
		}
	}

	void DsfLink::sendOnSocket(const std::string& text)
	{
		if (m_socket && !m_closed)
		{
			m_socket->send(text);
			m_lastSent = Clock::now();
		}
	}

	std::string DsfLink::filePath(std::string_view prefix, std::string_view file)
	{
		return std::string(prefix) + escapeSegment(file);
	}

	void DsfLink::requestModel(std::string_view, std::string_view, ModelHandler handler)
	{
		handler(std::unexpected(ReplyError{.message = "DSF pushes the object model: it isn't asked for"}));
	}

	void DsfLink::sendGcode(std::string_view gcode)
	{
		m_http.send(
			HTTP_POST,
			"/machine/code",
			{},
			codeTimeout,
			[gcode = std::string(gcode)](const HttpResponsePtr& response)
			{
				const auto reply = HttpRequester::readText(response);
				if (!reply)
				{
					LOG_ERROR("Failed to send \"{:s}\": {:s}", gcode, reply.error().message);
					return;
				}
				showResponse(*reply);
			},
			std::string(gcode));
	}

	void DsfLink::emergencyStop()
	{
		sendGcode("M112\nM999");
	}

	void DsfLink::listFiles(std::string_view dir, size_t first, Handler<FileList> handler)
	{
		if (first > 0)
		{
			handler(FileList{.dir = std::string(dir),
							 .first = first,
							 .files = {},
							 .next = 0}); // DSF lists a folder whole, in one page
			return;
		}
		m_http.getJson(filePath("/machine/directory/", dir),
					   {},
					   [handler = std::move(handler), dir = std::string(dir)](std::expected<OM::Node, ReplyError> json)
					   { handler(json.and_then([&dir](const OM::Node& list) { return FileList::read(list, dir); })); });
	}

	void DsfLink::fileInfo(std::string_view file, Handler<FileInfoReply> handler)
	{
		m_http.getJson(filePath("/machine/fileinfo/", file),
					   {},
					   [handler = std::move(handler)](std::expected<OM::Node, ReplyError> json)
					   { handler(json.and_then([](const OM::Node& info) { return FileInfoReply::read(info); })); });
	}

	void DsfLink::thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler)
	{
		hv::QueryParams query;
		query["readThumbnailContent"] = "true";
		m_http.getJson(
			filePath("/machine/fileinfo/", file),
			std::move(query),
			[handler = std::move(handler), file = std::string(file), offset](std::expected<OM::Node, ReplyError> json)
			{
				if (!json)
				{
					handler(std::unexpected(json.error()));
					return;
				}
				const OM::Node& thumbnails = (*json)["thumbnails"];
				for (size_t i = 0; i < thumbnails.size(); i++)
				{
					if (thumbnails[i]["offset"].integer(-1) == static_cast<int64_t>(offset))
					{
						// The whole thumbnail, in one piece
						handler(FileChunk{.fileName = file,
										  .offset = offset,
										  .data = std::string(thumbnails[i]["data"].text()),
										  .next = 0});
						return;
					}
				}
				handler(
					std::unexpected(ReplyError{.message = fmt::format("{:s} has no thumbnail at {:d}", file, offset)}));
			});
	}

	void DsfLink::readFile(std::string_view file, Handler<std::string> handler)
	{
		m_http.send(HTTP_GET,
					filePath("/machine/file/", file),
					{},
					fileTransferTimeout,
					[handler = std::move(handler)](const HttpResponsePtr& response)
					{ handler(HttpRequester::readText(response)); });
	}

	void DsfLink::writeFile(std::string_view file, std::string contents, Handler<void> handler)
	{
		m_http.send(HTTP_PUT,
					filePath("/machine/file/", file),
					{},
					fileTransferTimeout,
					succeeded(std::move(handler)),
					std::move(contents));
	}

	void DsfLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		m_http.send(HTTP_DELETE, filePath("/machine/file/", file), {}, HTTP_TIMEOUT, succeeded(std::move(handler)));
	}

	void DsfLink::expire(Clock::time_point now)
	{
		if (m_modelReceived && now - m_lastSent > pingInterval)
		{
			sendOnSocket("PING\n");
		}
	}
} // namespace Comm

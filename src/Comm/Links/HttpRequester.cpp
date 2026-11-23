/*
 * HttpRequester.cpp
 */

#include "HttpRequester.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectModel/Tree/Json.h"
#include "tracy/Tracy.hpp"
#include <fmt/format.h>
#include <hv/requests.h>
#include <thread>
#include <vector>

namespace Comm
{
	namespace
	{
		/** Why a request given up by close() has no response */
		ReplyError closedError()
		{
			return ReplyError{.message = "the connection closed"};
		}

		/** Why a request has no response, for want of one */
		ReplyError noResponse()
		{
			return ReplyError{.message = "no response from the printer"};
		}

		/** How much of a file is read and sent at a time */
		constexpr size_t uploadPieceSize = 32 * 1024;
	} // namespace

	HttpRequester::HttpRequester(std::string host, Post post, size_t maxNormal)
		: m_host(std::move(host))
		, m_post(std::move(post))
		, m_maxNormal(maxNormal)
		, m_alive(std::make_shared<bool>(true))
		, m_lastHeard(std::chrono::steady_clock::now())
	{
	}

	HttpRequester::~HttpRequester()
	{
		m_alive.reset();
	}

	void HttpRequester::send(Request request, ResponseHandler handler)
	{
		ZoneScoped;
		ZoneText(request.path.data(), request.path.size());
		if (m_closed)
		{
			handler(std::unexpected(closedError()));
			return;
		}
		Pending pending{.request = build(request),
						.lane = request.lane,
						.handler = std::move(handler),
						.upload = std::move(request.upload),
						.progress = std::move(request.progress),
						.stop = std::make_shared<std::atomic_bool>(false)};
		if (request.lane == Lane::urgent)
		{
			dispatch(std::move(pending)); // ahead of everything, whatever is in flight
			return;
		}
		m_queue.push_back(std::move(pending));
		sendQueued();
	}

	void HttpRequester::getJson(std::string path, hv::QueryParams query, JsonHandler handler)
	{
		ZoneScoped;
		send(Request{.path = std::move(path), .query = std::move(query), .timeout = HTTP_TIMEOUT},
			 [handler = std::move(handler)](const Response& response)
			 {
				 ZoneScopedN("getJson_cb");
				 handler(text(response).and_then([](const std::string& body) { return json(body); }));
			 });
	}

	void HttpRequester::close(std::string_view endSession)
	{
		ZoneScoped;
		if (m_closed)
		{
			return;
		}
		m_closed = true;
		if (!endSession.empty() && !m_unauthorised)
		{
			// Not waited for: the printer may have stopped answering, and this link is going anyway
			requests::async(build(Request{.path = std::string(endSession), .timeout = 1}),
							[](const HttpResponsePtr&) {});
		}
		std::vector<ResponseHandler> handlers;
		for (Pending& pending : m_queue)
		{
			handlers.push_back(std::move(pending.handler));
		}
		for (auto& [id, pending] : m_inFlight)
		{
			pending.stop->store(true);						// a file being sent stops after the piece being sent
			handlers.push_back(std::move(pending.handler)); // its answer, if one comes, finds nothing to give it to
		}
		m_queue.clear();
		m_inFlight.clear();
		TracyPlot("HTTP requests in flight", static_cast<int64_t>(0));
		for (const ResponseHandler& handler : handlers)
		{
			handler(std::unexpected(closedError()));
		}
	}

	std::expected<std::string, ReplyError> HttpRequester::text(const Response& response)
	{
		ZoneScoped;
		if (!response)
		{
			return std::unexpected(response.error());
		}
		const HttpResponsePtr& http = *response;
		if (http->status_code < HTTP_STATUS_OK || http->status_code >= 300)
		{
			return std::unexpected(ReplyError{.message = fmt::format("HTTP error {:d}", (int)http->status_code)});
		}
		return http->body;
	}

	std::expected<OM::Node, ReplyError> HttpRequester::json(std::string_view text)
	{
		ZoneScoped;
		auto json = OM::parseJson(text);
		if (!json)
		{
			return std::unexpected(
				ReplyError{.message = fmt::format("a reply that isn't JSON: {:s}", json.error().message)});
		}
		return std::move(*json);
	}

	HttpRequestPtr HttpRequester::build(const Request& request) const
	{
		ZoneScoped;
		auto http = std::make_shared<HttpRequest>();
		http->method = request.method;
		http->host = m_host;
		http->path = request.path;
		http->query_params = request.query;
		http->timeout = request.timeout;
		http->body = request.body;
		http->headers["Connection"] = "keep-alive";
		for (const auto& [name, value] : m_headers)
		{
			http->headers[name] = value;
		}
		http->DumpUrl(); // the URL, from the host, path and query
		return http;
	}

	bool HttpRequester::hasRoom(Lane lane) const
	{
		size_t inLane = 0;
		for (const auto& [id, pending] : m_inFlight)
		{
			inLane += pending.lane == lane ? 1 : 0;
		}
		return inLane < (lane == Lane::slow ? 1 : m_maxNormal);
	}

	void HttpRequester::sendQueued()
	{
		ZoneScoped;
		// In turn within a lane; a slow request waiting doesn't hold up a normal one behind it
		for (auto it = m_queue.begin(); it != m_queue.end() && !m_closed;)
		{
			if (!hasRoom(it->lane))
			{
				++it;
				continue;
			}
			Pending pending = std::move(*it);
			it = m_queue.erase(it);
			dispatch(std::move(pending));
		}
	}

	void HttpRequester::dispatch(Pending pending)
	{
		ZoneScoped;
		const uint64_t id = m_nextId++;
		const HttpRequestPtr request = pending.request;
		LOG_DBG("HTTP {:s} {:s}", http_method_str(request->method), request->path);
		const auto& [it, added] = m_inFlight.emplace(id, std::move(pending));
		TracyPlot("HTTP requests in flight", static_cast<int64_t>(m_inFlight.size()));
		if (it->second.upload)
		{
			stream(id, it->second);
			return;
		}
		m_client.sendAsync(
			request,
			[this, id, alive = std::weak_ptr<bool>(m_alive), post = m_post](const HttpResponsePtr& response)
			{
				ZoneScopedN("httpReceived_cb"); // on libhv's thread: the rest happens on the connection's
				post(
					[this, id, alive, response]
					{
						ZoneScopedN("httpResponse_cb");
						if (alive.lock())
						{
							finished(id, response ? Response(response) : std::unexpected(noResponse()));
						}
					});
			});
	}

	void HttpRequester::stream(uint64_t id, const Pending& pending)
	{
		ZoneScoped;
		const HttpRequestPtr request = pending.request;
		const FileContents contents = *pending.upload;
		request->SetHeader("Content-Length", std::to_string(contents.size()));
		// Only what is posted to the connection's thread touches the requester, and only while it is still here
		std::thread(
			[this,
			 id,
			 request,
			 contents,
			 progress = pending.progress,
			 stop = pending.stop,
			 alive = std::weak_ptr<bool>(m_alive),
			 post = m_post]
			{
				tracy::SetThreadName("HTTP upload");
				const auto answer = [&](Response response)
				{
					post(
						[this, id, alive, response = std::move(response)]
						{
							if (alive.lock())
							{
								finished(id, response);
							}
						});
				};

				hv::HttpClient client;
				if (client.sendHeader(request.get()) != 0)
				{
					answer(std::unexpected(noResponse()));
					return;
				}
				size_t sent = 0;
				while (sent < contents.size())
				{
					if (stop->load())
					{
						return; // given up: its handler has been told
					}
					const std::expected<std::string, ReplyError> piece = contents.read(sent, uploadPieceSize);
					if (!piece)
					{
						answer(std::unexpected(piece.error()));
						return;
					}
					if (client.sendData(piece->data(), static_cast<int>(piece->size())) !=
						static_cast<int>(piece->size()))
					{
						answer(std::unexpected(ReplyError{.message = "the file couldn't be sent"}));
						return;
					}
					sent += piece->size();
					post(
						[progress, alive, stop, sent, total = contents.size()]
						{
							if (alive.lock() && !stop->load() && progress)
							{
								progress(sent, total);
							}
						});
				}
				auto response = std::make_shared<HttpResponse>();
				if (client.recvResponse(response.get()) != 0)
				{
					answer(std::unexpected(noResponse()));
					return;
				}
				answer(response);
			})
			.detach();
	}

	void HttpRequester::finished(uint64_t id, const Response& response)
	{
		ZoneScoped;
		const auto it = m_inFlight.find(id);
		if (it == m_inFlight.end())
		{
			return; // given up by close()
		}
		const std::weak_ptr<bool> alive = m_alive; // taken now: the handler may close the link, and this with it
		ResponseHandler handler = std::move(it->second.handler);
		m_inFlight.erase(it);
		TracyPlot("HTTP requests in flight", static_cast<int64_t>(m_inFlight.size()));
		if (!response)
		{
			handler(response);
		}
		else
		{
			m_lastHeard = std::chrono::steady_clock::now();
			if ((*response)->status_code == HTTP_STATUS_UNAUTHORIZED)
			{
				LOG_WARN("The printer no longer knows the session");
				m_unauthorised = true;
			}
			handler(response);
		}
		if (alive.lock())
		{
			sendQueued();
		}
	}
} // namespace Comm

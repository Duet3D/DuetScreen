/*
 * HttpRequester.cpp
 */

#include "HttpRequester.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectModel/Tree/Json.h"
#include "tracy/Tracy.hpp"
#include <fmt/format.h>

namespace Comm
{
	HttpRequester::HttpRequester(std::string host, Post post, size_t maxInFlight)
		: m_host(std::move(host))
		, m_post(std::move(post))
		, m_maxInFlight(maxInFlight)
		, m_alive(std::make_shared<bool>(true))
		, m_lastHeard(std::chrono::steady_clock::now())
	{
		ZoneScoped;
	}

	HttpRequester::~HttpRequester()
	{
		ZoneScoped;
		m_alive.reset();
	}

	void HttpRequester::send(http_method method,
							 std::string_view path,
							 hv::QueryParams query,
							 uint16_t timeout,
							 ResponseHandler handler,
							 std::string body)
	{
		ZoneScoped;
		ZoneText(path.data(), path.size());
		auto request = std::make_shared<HttpRequest>();
		request->method = method;
		request->host = m_host;
		request->path = path;
		request->query_params = std::move(query);
		request->timeout = timeout;
		request->body = std::move(body);
		request->headers["Connection"] = "keep-alive";
		for (const auto& [name, value] : m_headers)
		{
			request->headers[name] = value;
		}
		request->DumpUrl(); // the URL, from the host, path and query
		m_queue.push_back({.request = std::move(request), .handler = std::move(handler)});
		sendQueued();
	}

	void HttpRequester::getJson(std::string_view path, hv::QueryParams query, JsonHandler handler)
	{
		ZoneScoped;
		send(HTTP_GET,
			 path,
			 std::move(query),
			 HTTP_TIMEOUT,
			 [handler = std::move(handler)](const HttpResponsePtr& response) { handler(readJson(response)); });
	}

	std::expected<OM::Node, ReplyError> HttpRequester::readJson(const HttpResponsePtr& response)
	{
		ZoneScoped;
		auto text = readText(response);
		if (!text)
		{
			return std::unexpected(text.error());
		}
		auto json = OM::parseJson(*text);
		if (!json)
		{
			return std::unexpected(
				ReplyError{.message = fmt::format("a reply that isn't JSON: {:s}", json.error().message)});
		}
		return std::move(*json);
	}

	std::expected<std::string, ReplyError> HttpRequester::readText(const HttpResponsePtr& response)
	{
		ZoneScoped;
		if (!response)
		{
			return std::unexpected(ReplyError{.message = "no response from the printer"});
		}
		if (response->status_code < HTTP_STATUS_OK || response->status_code >= 300)
		{
			return std::unexpected(ReplyError{.message = fmt::format("HTTP error {:d}", (int)response->status_code)});
		}
		return response->body;
	}

	void HttpRequester::sendQueued()
	{
		ZoneScoped;
		while (m_inFlight < m_maxInFlight && !m_queue.empty())
		{
			Queued next = std::move(m_queue.front());
			m_queue.pop_front();
			m_inFlight++;
			TracyPlot("HTTP requests in flight", static_cast<int64_t>(m_inFlight));
			LOG_DBG("HTTP {:s} {:s}", http_method_str(next.request->method), next.request->path);
			m_client.sendAsync(
				next.request,
				[this, alive = std::weak_ptr<bool>(m_alive), post = m_post, handler = std::move(next.handler)](
					const HttpResponsePtr& response)
				{
					ZoneScopedN("httpReceived_cb");
					// On libhv's thread: everything else happens on the connection's
					post(
						[this, alive, handler, response]
						{
							ZoneScopedN("httpResponse_cb");
							if (!alive.lock())
							{
								return; // the requester has gone
							}
							m_inFlight--;
							TracyPlot("HTTP requests in flight", static_cast<int64_t>(m_inFlight));
							if (response)
							{
								m_lastHeard = std::chrono::steady_clock::now();
								if (response->status_code == HTTP_STATUS_UNAUTHORIZED)
								{
									LOG_WARN("The printer no longer knows the session");
									m_unauthorised = true;
								}
							}
							handler(response);
							if (alive.lock()) // the handler may have closed the link, and the requester with it
							{
								sendQueued();
							}
						});
				});
		}
	}
} // namespace Comm

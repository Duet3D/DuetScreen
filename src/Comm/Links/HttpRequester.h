/*
 * HttpRequester.h
 *
 * Sends a network link's HTTP requests (HttpLink's rr_ requests, DsfLink's /machine requests) through one HTTP client,
 * so its connections are kept alive between requests, with only so many in flight at once (the rest wait their turn).
 * Answers arrive on libhv's thread and are posted to the connection's thread before anything is done with them; an
 * answer that arrives after the requester has gone is dropped.
 */

#pragma once

#include "Comm/Replies.h"
#include "ObjectModel/Tree/Node.h"
#include <chrono>
#include <cstdint>
#include <deque>
#include <expected>
#include <functional>
#include <hv/HttpClient.h>
#include <map>
#include <memory>
#include <string>

namespace Comm
{
	class HttpRequester
	{
	  public:
		/** Runs a task on the connection's thread */
		using Post = std::function<void(std::function<void()>)>;

		/** Receives an HTTP response (or nullptr if there was none), on the connection's thread */
		using ResponseHandler = std::function<void(const HttpResponsePtr& response)>;

		/** Receives a response's JSON, or why there is none */
		using JsonHandler = std::function<void(std::expected<OM::Node, ReplyError> json)>;

		/**
		 * @param host The printer's address, e.g. "192.168.1.20"
		 * @param post Runs a task on the connection's thread
		 * @param maxInFlight How many requests may be in flight at once
		 */
		HttpRequester(std::string host, Post post, size_t maxInFlight);

		/** Forget every answer still to come */
		~HttpRequester();

		HttpRequester(const HttpRequester&) = delete;
		HttpRequester& operator=(const HttpRequester&) = delete;

		/** The printer's address */
		const std::string& host() const { return m_host; }

		/** Send the header `name` with every request from now on (e.g. a session key) */
		void setHeader(std::string name, std::string value) { m_headers[std::move(name)] = std::move(value); }

		/**
		 * Send a request when there is room
		 * @param method E.g. HTTP_GET
		 * @param path E.g. "/rr_model"
		 * @param query Its parameters
		 * @param timeout How long its response may take, seconds
		 * @param handler Given the response
		 * @param body What is sent with it
		 */
		void send(http_method method,
				  std::string_view path,
				  hv::QueryParams query,
				  uint16_t timeout,
				  ResponseHandler handler,
				  std::string body = {});

		/** As send(), for a GET answered with JSON: `handler` is given the JSON, or why there is none */
		void getJson(std::string_view path, hv::QueryParams query, JsonHandler handler);

		/** A response's body as JSON, or why there is none (no response, an HTTP error, not JSON) */
		static std::expected<OM::Node, ReplyError> readJson(const HttpResponsePtr& response);

		/** A response's body as text, or why there is none (no response, an HTTP error) */
		static std::expected<std::string, ReplyError> readText(const HttpResponsePtr& response);

		/** Whether a request is in flight or waiting its turn */
		bool waiting() const { return m_inFlight > 0 || !m_queue.empty(); }

		/** When the printer last answered */
		std::chrono::steady_clock::time_point lastHeard() const { return m_lastHeard; }

		/** Whether the printer has refused a request for want of a session (it restarted, or forgot it) */
		bool unauthorised() const { return m_unauthorised; }

	  private:
		/** A request waiting its turn */
		struct Queued
		{
			HttpRequestPtr request;	 // The request
			ResponseHandler handler; // Given its response
		};

		/** Send the requests waiting while there is room */
		void sendQueued();

		std::string m_host;								   // The printer's address
		Post m_post;									   // Runs a task on the connection's thread
		size_t m_maxInFlight;							   // How many requests may be in flight at once
		std::map<std::string, std::string> m_headers;	   // Sent with every request
		hv::HttpClient m_client;						   // Sends every request, keeping connections alive
		std::shared_ptr<bool> m_alive;					   // Still here: answers after it has gone are dropped
		std::deque<Queued> m_queue;						   // Requests waiting their turn
		size_t m_inFlight = 0;							   // Requests sent and not yet answered
		std::chrono::steady_clock::time_point m_lastHeard; // When the printer last answered
		bool m_unauthorised = false;					   // The printer refused a request for want of a session
	};
} // namespace Comm

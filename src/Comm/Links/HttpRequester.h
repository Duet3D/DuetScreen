/*
 * HttpRequester.h
 *
 * Sends a network link's HTTP requests (HttpLink's rr_ requests, DsfLink's /machine requests) through one HTTP client,
 * so its connections are kept alive between requests. Requests go in lanes:
 *
 *   - urgent: sent at once, ahead of everything and whatever is in flight (an emergency stop)
 *   - normal: a few at a time, in turn (the object model, file information, thumbnails)
 *   - slow:   one at a time, in a lane of their own (files coming or going, DSF's G-code, which answers only once it
 *             has run), so they never hold up the normal lane, and the printer is still heard from meanwhile
 *
 * Answers arrive on libhv's thread and are posted to the connection's thread before anything is done with them. Every
 * request's handler is called once: with the response, or with why there is none (no response, or the requester was
 * closed). After close(), or once the requester has gone, nothing more is sent or answered.
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

		/** A response, whatever its status, or why there is none */
		using Response = std::expected<HttpResponsePtr, ReplyError>;

		/** Receives a request's response, on the connection's thread */
		using ResponseHandler = std::function<void(const Response& response)>;

		/** Receives a response's JSON, or why there is none */
		using JsonHandler = std::function<void(std::expected<OM::Node, ReplyError> json)>;

		/** Which lane a request goes in (see the file comment) */
		enum class Lane
		{
			urgent,
			normal,
			slow,
		};

		/** One request */
		struct Request
		{
			http_method method = HTTP_GET; // E.g. HTTP_GET
			std::string path;			   // E.g. "/rr_model"
			hv::QueryParams query = {};	   // Its parameters
			uint16_t timeout = 5;		   // How long its response may take, seconds
			Lane lane = Lane::normal;	   // Which lane it goes in
			std::string body = {};		   // What is sent with it
		};

		/**
		 * @param host The printer's address, e.g. "192.168.1.20"
		 * @param post Runs a task on the connection's thread
		 * @param maxNormal How many requests of the normal lane may be in flight at once
		 */
		HttpRequester(std::string host, Post post, size_t maxNormal);

		/** Forget every answer still to come, without calling its handler (see close()) */
		~HttpRequester();

		HttpRequester(const HttpRequester&) = delete;
		HttpRequester& operator=(const HttpRequester&) = delete;

		/** The printer's address */
		const std::string& host() const { return m_host; }

		/** Send the header `name` with every request from now on (e.g. a session key) */
		void setHeader(std::string name, std::string value) { m_headers[std::move(name)] = std::move(value); }

		/** Send `request` when its lane has room, and give `handler` its response */
		void send(Request request, ResponseHandler handler);

		/** Send a GET of `path` in the normal lane, and give `handler` the JSON it answers with */
		void getJson(std::string path, hv::QueryParams query, JsonHandler handler);

		/**
		 * Give up every request still waiting or in flight, telling each handler, and send nothing more
		 * @param endSession A request that ends the session (e.g. "/rr_disconnect"), sent without waiting for its
		 * response, unless the printer has already refused the session; none if empty
		 */
		void close(std::string_view endSession = {});

		/** A response's body as text, or why there is none (no response, an HTTP error) */
		static std::expected<std::string, ReplyError> text(const Response& response);

		/** `text` as JSON, or why it isn't */
		static std::expected<OM::Node, ReplyError> json(std::string_view text);

		/** Whether a request is in flight or waiting its turn */
		bool waiting() const { return !m_inFlight.empty() || !m_queue.empty(); }

		/** When the printer last answered */
		std::chrono::steady_clock::time_point lastHeard() const { return m_lastHeard; }

		/** Whether the printer has refused a request for want of a session (it restarted, or forgot it) */
		bool unauthorised() const { return m_unauthorised; }

	  private:
		/** A request with its handler, waiting its turn or in flight */
		struct Pending
		{
			HttpRequestPtr request;	 // The request
			Lane lane;				 // Its lane
			ResponseHandler handler; // Given its response
		};

		/** The request `request` describes, with the headers and the URL */
		HttpRequestPtr build(const Request& request) const;

		/** Send the requests waiting whose lanes have room, in turn */
		void sendQueued();

		/** Whether `lane` has room for another request in flight */
		bool hasRoom(Lane lane) const;

		/** Send `pending` now */
		void dispatch(Pending pending);

		/** The response to the request in flight as `id` arrived (or didn't) */
		void finished(uint64_t id, const HttpResponsePtr& response);

		std::string m_host;								   // The printer's address
		Post m_post;									   // Runs a task on the connection's thread
		size_t m_maxNormal;								   // How many normal requests may be in flight at once
		std::map<std::string, std::string> m_headers;	   // Sent with every request
		hv::HttpClient m_client;						   // Sends every request, keeping connections alive
		std::shared_ptr<bool> m_alive;					   // Still here: answers after it has gone are dropped
		std::deque<Pending> m_queue;					   // Requests waiting their turn, oldest first
		std::map<uint64_t, Pending> m_inFlight;			   // Requests sent and not yet answered, by id
		uint64_t m_nextId = 1;							   // The id the next request sent gets
		bool m_closed = false;							   // close() was called: nothing more is sent
		std::chrono::steady_clock::time_point m_lastHeard; // When the printer last answered
		bool m_unauthorised = false;					   // The printer refused a request for want of a session
	};
} // namespace Comm

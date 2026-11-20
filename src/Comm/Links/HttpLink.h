/*
 * HttpLink.h
 *
 * A standalone Duet on the network, reached by its rr_* requests (RepRapFirmware's HTTP API, as DWC uses it):
 * rr_connect for a session, rr_model for the object model, rr_gcode and rr_reply for G-code and its responses,
 * rr_filelist, rr_fileinfo, rr_thumbnail, rr_download, rr_upload and rr_delete for files.
 *
 * One HTTP client is kept, so its connections are kept alive between requests, and only a couple of requests are in
 * flight at once (RepRapFirmware has only a few HTTP sessions); the rest wait their turn. Answers arrive on libhv's
 * thread and are posted to the connection's thread (see Connection.h) before anything is done with them.
 */

#pragma once

#include "Comm/Link.h"
#include "ObjectModel/Tree/Node.h"
#include <chrono>
#include <deque>
#include <functional>
#include <hv/HttpClient.h>
#include <memory>
#include <string>

namespace Comm
{
	class HttpLink : public Link
	{
	  public:
		/** Runs a task on the connection's thread */
		using Post = std::function<void(std::function<void()>)>;

		/** Receives a reply nothing asked for: a JSON response from rr_reply */
		using UnaskedHandler = std::function<void(OM::Node& reply)>;

		/** What rr_connect said */
		struct Session
		{
			bool sbc = false; // The Duet has an SBC, and DSF answered for it
		};

		/** How many requests are in flight at once at most */
		static constexpr size_t maxInFlight = 2;

		/**
		 * @param host The Duet's address, e.g. "192.168.1.20"
		 * @param password Its password; RepRapFirmware's default ("reprap") if empty
		 * @param post Runs a task on the connection's thread
		 * @param unasked Given a reply nothing asked for
		 */
		HttpLink(std::string host, std::string password, Post post, UnaskedHandler unasked);

		/** End the session (rr_disconnect), and forget every answer still to come */
		~HttpLink() override;

		HttpLink(const HttpLink&) = delete;
		HttpLink& operator=(const HttpLink&) = delete;

		/** Start a session (rr_connect) */
		void open(Handler<Session> handler);

		// Link

		void requestModel(std::string_view key, std::string_view flags, ModelHandler handler) override;
		void fetchResponses() override;
		void sendGcode(std::string_view gcode) override;
		void emergencyStop() override;
		void listFiles(std::string_view dir, size_t first, Handler<FileList> handler) override;
		void fileInfo(std::string_view file, Handler<FileInfoReply> handler) override;
		void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) override;
		void readFile(std::string_view file, Handler<std::string> handler) override;
		void writeFile(std::string_view file, std::string contents, Handler<void> handler) override;
		void deleteFile(std::string_view file, Handler<void> handler) override;
		void expire(std::chrono::steady_clock::time_point) override {}
		std::chrono::steady_clock::time_point lastHeard() const override { return m_lastHeard; }
		bool failed() const override { return m_sessionLost; }
		bool waiting() const override { return m_inFlight > 0 || !m_queue.empty(); }

	  private:
		/** Receives an HTTP response (or nullptr if there was none), on the connection's thread */
		using ResponseHandler = std::function<void(const HttpResponsePtr& response)>;

		/** A request waiting its turn */
		struct Queued
		{
			HttpRequestPtr request;	 // The request
			ResponseHandler handler; // Given its response
		};

		/**
		 * Send a request when there is room
		 * @param method HTTP_GET or HTTP_POST
		 * @param path E.g. "/rr_model"
		 * @param query Its parameters
		 * @param timeout How long its response may take, seconds
		 * @param body What is posted
		 */
		void send(http_method method,
				  std::string_view path,
				  hv::QueryParams query,
				  uint16_t timeout,
				  ResponseHandler handler,
				  std::string body = {});

		/** As send(), for a request answered with JSON: `handler` is given the JSON, or why there is none */
		void sendForJson(std::string_view path,
						 hv::QueryParams query,
						 std::function<void(std::expected<OM::Node, ReplyError>)> handler);

		/** Send the requests waiting while there is room */
		void sendQueued();

		/** The response's body as JSON, or why there is none */
		std::expected<OM::Node, ReplyError> readJson(const HttpResponsePtr& response);

		std::string m_host;					 // The Duet's address
		std::string m_password;				 // Its password
		Post m_post;						 // Runs a task on the connection's thread
		UnaskedHandler m_unasked;			 // Given replies nothing asked for
		hv::HttpClient m_client;			 // Sends every request, keeping connections alive
		std::shared_ptr<bool> m_alive;		 // Still here: answers arriving after the link has gone are dropped
		std::deque<Queued> m_queue;			 // Requests waiting their turn
		size_t m_inFlight = 0;				 // Requests sent and not yet answered
		uint32_t m_sessionKey = 0;			 // The session's key, or 0 for none
		bool m_sessionLost = false;			 // The Duet refused a request for want of a session (it restarted)
		std::chrono::steady_clock::time_point m_lastHeard; // When the Duet last answered
	};
} // namespace Comm

/*
 * DsfLink.h
 *
 * A Duet with an SBC, reached through DSF's own API (as DWC reaches it):
 *
 *   - GET /machine/connect starts a session; its key goes with every request (X-Session-Key)
 *   - the WebSocket /machine pushes the object model: the whole of it first, then a patch whenever something changes
 *     (objects carry only what changed; arrays carry every item, {} for one that didn't). Each message is answered
 *     "OK" before DSF sends the next, and "PING" keeps a quiet socket open. Nothing is polled.
 *   - POST /machine/code runs G-code and answers with its response
 *   - /machine/directory, /machine/fileinfo and /machine/file list, describe, read, write and delete files, with the
 *     field names RepRapFirmware uses; a thumbnail comes whole in its file's information
 *
 * The connection finds it is talking to DSF when rr_connect says so (see HttpLink.h).
 */

#pragma once

#include "Comm/Link.h"
#include "HttpRequester.h"
#include <algorithm>
#include <chrono>
#include <memory>
#include <string>

namespace hv
{
	class WebSocketClient;
}

namespace Comm
{
	class DsfLink : public Link
	{
	  public:
		using Clock = std::chrono::steady_clock; // What times are measured with

		/** How long the socket may stay quiet before a PING is sent */
		static constexpr std::chrono::milliseconds pingInterval{4000};

		/** How many requests are in flight at once at most (DSF answers many at once) */
		static constexpr size_t maxInFlight = 4;

		/**
		 * @param host The SBC's address, e.g. "192.168.1.20"
		 * @param password The Duet's password; none if empty
		 * @param post Runs a task on the connection's thread
		 */
		DsfLink(std::string host, std::string password, HttpRequester::Post post);

		/** Close the socket and end the session */
		~DsfLink() override;

		DsfLink(const DsfLink&) = delete;
		DsfLink& operator=(const DsfLink&) = delete;

		/** Start a session and open the socket; `handler` is told once the whole object model has arrived */
		void open(Handler<void> handler);

		// Link

		bool pushesModel() const override { return true; }
		void requestModel(std::string_view key, std::string_view flags, ModelHandler handler) override;
		void sendGcode(std::string_view gcode) override;
		void emergencyStop() override;
		void listFiles(std::string_view dir, size_t first, Handler<FileList> handler) override;
		void fileInfo(std::string_view file, Handler<FileInfoReply> handler) override;
		void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) override;
		void readFile(std::string_view file, Handler<std::string> handler) override;
		void writeFile(std::string_view file, std::string contents, Handler<void> handler) override;
		void deleteFile(std::string_view file, Handler<void> handler) override;
		void expire(Clock::time_point now) override;
		Clock::time_point lastHeard() const override { return std::max(m_lastHeard, m_http.lastHeard()); }
		bool failed() const override { return m_closed || m_http.unauthorised(); }
		bool waiting() const override { return m_http.waiting(); }

	  private:
		/** Open the socket, telling `handler` once the whole object model has arrived */
		void openSocket(Handler<void> handler);

		/** A message arrived on the socket */
		void messageArrived(const std::string& message);

		/** The socket closed */
		void socketClosed();

		/** Send `text` on the socket */
		void sendOnSocket(const std::string& text);

		/** The path of `file` (e.g. "0:/gcodes/a b.gcode") in a request after `prefix` (e.g. "/machine/file/") */
		static std::string filePath(std::string_view prefix, std::string_view file);

		std::string m_password;						   // The Duet's password
		HttpRequester m_http;						   // Sends the requests
		HttpRequester::Post m_post;					   // Runs a task on the connection's thread
		std::unique_ptr<hv::WebSocketClient> m_socket; // Pushes the object model
		std::shared_ptr<bool> m_alive;				   // Still here: socket events after the link has gone are dropped
		std::string m_sessionKey;					   // The session's key
		Handler<void> m_opened;						   // Told once the whole model has arrived
		bool m_modelReceived = false;				   // The whole model has arrived: later messages are patches
		bool m_closed = false;						   // The socket closed
		Clock::time_point m_lastHeard;				   // When the socket last received anything
		Clock::time_point m_lastSent;				   // When anything was last sent on the socket
	};
} // namespace Comm

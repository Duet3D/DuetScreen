/*
 * HttpLink.h
 *
 * A standalone Duet on the network, reached by its rr_* requests (RepRapFirmware's HTTP API, as DWC uses it):
 * rr_connect for a session, rr_model for the object model, rr_gcode and rr_reply for G-code and its responses,
 * rr_filelist, rr_fileinfo, rr_thumbnail, rr_download, rr_upload and rr_delete for files.
 *
 * Only a couple of requests are in flight at once (RepRapFirmware has only a few HTTP sessions), with files coming
 * and going in a lane of their own and an emergency stop ahead of everything (see HttpRequester.h).
 *
 * A Duet with an SBC answers rr_connect too (DSF emulates it), and says so: the connection then talks to it through
 * DSF's own API instead (DsfLink.h).
 */

#pragma once

#include "Comm/Link.h"
#include "HttpRequester.h"
#include "ObjectModel/Tree/Node.h"
#include <chrono>
#include <functional>
#include <string>

namespace Comm
{
	class HttpLink : public Link
	{
	  public:
		/** Receives a reply nothing asked for: a JSON response from rr_reply */
		using UnaskedHandler = std::function<void(OM::Node& reply)>;

		/** What rr_connect said */
		struct Session
		{
			bool sbc = false; // The Duet has an SBC: DSF answered for it
		};

		/** How many requests are in flight at once at most */
		static constexpr size_t maxInFlight = 2;

		/**
		 * @param host The Duet's address, e.g. "192.168.1.20"
		 * @param password Its password; RepRapFirmware's default ("reprap") if empty
		 * @param post Runs a task on the connection's thread
		 * @param unasked Given a reply nothing asked for
		 */
		HttpLink(std::string host, std::string password, HttpRequester::Post post, UnaskedHandler unasked);

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
		std::optional<std::chrono::steady_clock::time_point> nextDue() const override { return std::nullopt; }
		void close() override;
		std::chrono::steady_clock::time_point lastHeard() const override { return m_http.lastHeard(); }
		bool failed() const override { return m_http.unauthorised(); }
		bool waiting() const override { return m_http.waiting(); }

	  private:
		std::string m_password;	   // The Duet's password
		UnaskedHandler m_unasked;  // Given replies nothing asked for
		HttpRequester m_http;	   // Sends the requests
		uint32_t m_sessionKey = 0; // The session's key, or 0 for none
	};
} // namespace Comm

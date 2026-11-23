/*
 * test_http_upload.cpp
 *
 * A file written over the network (src/Comm/Links/HttpRequester.h) is sent as it is read, a piece at a time, with its
 * progress told after each, to a printer played by a local HTTP server.
 */

#include "Comm/Links/HttpRequester.h"
#include <chrono>
#include <deque>
#include <fmt/format.h>
#include <functional>
#include <gtest/gtest.h>
#include <hv/HttpServer.h>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
	/** A printer that takes uploads, keeping what it was sent */
	class UploadServer
	{
	  public:
		/** Listen on a port of this test process's own */
		UploadServer()
		{
			m_service.POST("/rr_upload",
						   [this](HttpRequest* request, HttpResponse* response)
						   {
							   std::lock_guard lock(m_mutex);
							   m_received = request->body;
							   m_contentLength = request->GetHeader("Content-Length");
							   return response->String(R"({"err":0})");
						   });
			m_server.registerHttpService(&m_service);
			m_server.setPort(port());
			m_server.setThreadNum(1);
			m_server.start();
		}

		/** Stop listening */
		~UploadServer() { m_server.stop(); }

		/** The port it listens on */
		static int port() { return 20000 + static_cast<int>(getpid() % 10000); }

		/** What the last upload sent */
		std::string received()
		{
			std::lock_guard lock(m_mutex);
			return m_received;
		}

		/** The last upload's Content-Length header */
		std::string contentLength()
		{
			std::lock_guard lock(m_mutex);
			return m_contentLength;
		}

	  private:
		hv::HttpService m_service;	 // Answers /rr_upload
		hv::HttpServer m_server;	 // Serves it
		std::mutex m_mutex;			 // Guards what was received: the server answers on a thread of its own
		std::string m_received;		 // The last upload's body
		std::string m_contentLength; // Its Content-Length header
	};

	/** The connection's thread, played by the test: tasks posted to it run when it pumps them */
	class Tasks
	{
	  public:
		/** Post `task` (from any thread) */
		void post(std::function<void()> task)
		{
			std::lock_guard lock(m_mutex);
			m_tasks.push_back(std::move(task));
		}

		/** Run the tasks posted until `done`, for at most 10 s */
		void runUntil(const std::function<bool()>& done)
		{
			const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
			while (!done() && std::chrono::steady_clock::now() < until)
			{
				std::deque<std::function<void()>> tasks;
				{
					std::lock_guard lock(m_mutex);
					tasks.swap(m_tasks);
				}
				for (auto& task : tasks)
				{
					task();
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
		}

	  private:
		std::mutex m_mutex;						   // Guards the tasks
		std::deque<std::function<void()>> m_tasks; // Posted, not yet run
	};
} // namespace

TEST(TestHttpUpload, AFileIsSentInPiecesWithItsProgressTold)
{
	UploadServer server;
	Tasks tasks;
	Comm::HttpRequester requester(
		fmt::format("127.0.0.1:{:d}", UploadServer::port()),
		[&tasks](std::function<void()> task) { tasks.post(std::move(task)); },
		2);

	std::string contents;
	for (int i = 0; contents.size() < 100 * 1000; i++)
	{
		contents += fmt::format("G1 X{:d} Y{:d}\n", i % 200, i % 150);
	}
	std::vector<size_t> progress;
	std::optional<Comm::HttpRequester::Response> response;
	requester.send({.method = HTTP_POST,
					.path = "/rr_upload",
					.query = {{"name", "0:/gcodes/big.gcode"}},
					.timeout = 10,
					.lane = Comm::HttpRequester::Lane::slow,
					.upload = Comm::FileContents::bytes(contents),
					.progress = [&progress](size_t sent, size_t) { progress.push_back(sent); }},
				   [&response](const Comm::HttpRequester::Response& answer) { response = answer; });
	tasks.runUntil([&response] { return response.has_value(); });

	ASSERT_TRUE(response);
	ASSERT_TRUE(*response) << response->error().message;
	EXPECT_EQ((**response)->status_code, HTTP_STATUS_OK);
	EXPECT_EQ(server.received(), contents) << "every piece, in order";
	EXPECT_EQ(server.contentLength(), std::to_string(contents.size())) << "its length said up front";
	ASSERT_GE(progress.size(), 3u) << "told after each piece";
	EXPECT_EQ(progress.back(), contents.size());
	EXPECT_TRUE(std::is_sorted(progress.begin(), progress.end()));
	EXPECT_FALSE(requester.waiting());
}

TEST(TestHttpUpload, AFileThatCantBeSentFails)
{
	Tasks tasks;
	// Nothing listens there
	Comm::HttpRequester requester(
		fmt::format("127.0.0.1:{:d}", UploadServer::port() + 1),
		[&tasks](std::function<void()> task) { tasks.post(std::move(task)); },
		2);
	std::optional<Comm::HttpRequester::Response> response;
	requester.send({.method = HTTP_POST,
					.path = "/rr_upload",
					.timeout = 2,
					.lane = Comm::HttpRequester::Lane::slow,
					.upload = Comm::FileContents::bytes("G28\n")},
				   [&response](const Comm::HttpRequester::Response& answer) { response = answer; });
	tasks.runUntil([&response] { return response.has_value(); });
	ASSERT_TRUE(response);
	EXPECT_FALSE(*response);
	EXPECT_FALSE(requester.waiting());
}

TEST(TestHttpUpload, AFileCancelledIsNotSent)
{
	UploadServer server;
	Tasks tasks;
	Comm::HttpRequester requester(
		fmt::format("127.0.0.1:{:d}", UploadServer::port()),
		[&tasks](std::function<void()> task) { tasks.post(std::move(task)); },
		2);
	const Comm::FileContents contents = Comm::FileContents::bytes(std::string(100 * 1000, 'x'));
	contents.cancel();
	std::optional<Comm::HttpRequester::Response> response;
	requester.send({.method = HTTP_POST,
					.path = "/rr_upload",
					.timeout = 10,
					.lane = Comm::HttpRequester::Lane::slow,
					.upload = contents},
				   [&response](const Comm::HttpRequester::Response& answer) { response = answer; });
	tasks.runUntil([&response] { return response.has_value(); });
	ASSERT_TRUE(response);
	ASSERT_FALSE(*response);
	EXPECT_EQ(response->error().message, "cancelled");
	EXPECT_TRUE(server.received().empty());
}

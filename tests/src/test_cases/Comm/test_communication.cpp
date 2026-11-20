/*
 * test_communication.cpp
 *
 * Talking to a printer over a serial port (Comm/Links/SerialLink.h), through a port the test plays the printer on:
 * how lines are sent, and how replies find the request they answer by line number.
 */

#include "Comm/Gcode.h"
#include "Comm/Links/SerialLink.h"
#include "ObjectModel/Tree/Json.h"
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

using Comm::SerialLink;

namespace
{
	/** A port that keeps what is written to it, one or two channels */
	class FakePort : public Comm::SerialPort
	{
	  public:
		/** @param channelCount How many channels it has */
		explicit FakePort(size_t channelCount = 1, bool checksums = false)
			: m_channels(channelCount)
			, m_checksums(checksums)
		{
		}

		bool open(Receiver) override { return true; }
		void close() override {}

		bool write(size_t channel, std::string_view bytes) override
		{
			written.push_back({channel, std::string(bytes)});
			return writable;
		}

		size_t channels() const override { return m_channels; }
		bool needsChecksums() const override { return m_checksums; }
		std::chrono::milliseconds replyTimeout() const override { return timeout; }
		size_t maxUploadSize() const override { return maxUpload; }

		/** One write */
		struct Write
		{
			size_t channel;	   // Which channel
			std::string bytes; // What
		};

		std::vector<Write> written;				 // Everything written, oldest first
		bool writable = true;					 // Whether writes work
		std::chrono::milliseconds timeout{5000}; // How long a request waits for its reply
		size_t maxUpload = SIZE_MAX;			 // The largest file written

	  private:
		size_t m_channels; // How many channels it has
		bool m_checksums;  // Whether lines need checksums
	};

	/** The lines written to `port`, without their endings */
	std::vector<std::string> lines(const FakePort& port)
	{
		std::vector<std::string> list;
		for (const auto& write : port.written)
		{
			EXPECT_TRUE(write.bytes.ends_with('\n')) << "a line is written whole";
			list.push_back(write.bytes.substr(0, write.bytes.size() - 1));
		}
		return list;
	}
} // namespace

class TestSerialLink : public ::testing::Test
{
  protected:
	/** Make the link over `port` */
	void open(FakePort& port)
	{
		link.emplace(port, [this](OM::Node& reply) { unasked.push_back(reply.toJson()); });
	}

	std::optional<SerialLink> link;	  // What is tested
	std::vector<std::string> unasked; // Replies given to the unasked handler, as JSON
};

TEST_F(TestSerialLink, EveryLineIsNumberedAndSentWhole)
{
	FakePort port;
	open(port);
	link->sendGcode("G28\nM400\r\n\nG1 X10");
	EXPECT_EQ(lines(port), (std::vector<std::string>{"N0 G28", "N1 M400", "N2 G1 X10"}))
		<< "an empty line says nothing";
}

TEST_F(TestSerialLink, UartLinesCarryAChecksum)
{
	FakePort port(1, true);
	open(port);
	link->sendGcode("M115");
	ASSERT_EQ(lines(port).size(), 1u);
	EXPECT_TRUE(lines(port)[0].starts_with("N0 M115*")) << lines(port)[0];
	EXPECT_EQ(lines(port)[0].size(), std::string("N0 M115*12345").size()) << "a five-digit CRC";
}

TEST_F(TestSerialLink, RepliesFindTheirRequestsByLineNumber)
{
	FakePort port;
	open(port);
	std::vector<std::string> answers;
	const auto keep = [&answers](std::string name)
	{
		return [&answers, name](std::expected<OM::Node, Comm::ReplyError> reply)
		{ answers.push_back(name + ":" + (reply ? std::string((*reply)["key"].text()) : reply.error().message)); };
	};
	link->requestModel("heat", "vo", keep("heat"));
	link->requestModel("fans", "v", keep("fans"));
	EXPECT_EQ(lines(port), (std::vector<std::string>{R"(N0 M409 K"heat" F"vo")", R"(N1 M409 K"fans" F"v")"}));
	EXPECT_TRUE(link->waiting());

	// Out of order, with a status report nobody asked for between them
	link->receive(0, R"({"key":"fans","flags":"v","result":[],"line":1})");
	link->receive(0, R"({"key":"","flags":"d99f","result":{"state":{"status":"busy"}}})");
	link->receive(0, R"({"key":"heat","flags":"vo","result":{},"line":0})");
	EXPECT_EQ(answers, (std::vector<std::string>{"fans:fans", "heat:heat"}));
	ASSERT_EQ(unasked.size(), 1u);
	EXPECT_NE(unasked[0].find("busy"), std::string::npos);
	EXPECT_FALSE(link->waiting());

	// A reply to a line nothing waits for (G-code's response, a reply too late) is unasked too
	link->receive(0, R"({"resp":"ok","line":7})");
	EXPECT_EQ(unasked.size(), 2u);
	link->receive(0, "not JSON");
	EXPECT_EQ(unasked.size(), 2u) << "a garbled line is dropped";
}

TEST_F(TestSerialLink, ARequestNotAnsweredInTimeIsGivenUp)
{
	FakePort port;
	port.timeout = std::chrono::milliseconds(100);
	open(port);
	std::optional<std::string> error;
	link->fileInfo("0:/gcodes/a.gcode",
				   [&error](std::expected<Comm::FileInfoReply, Comm::ReplyError> reply)
				   { error = reply ? "" : reply.error().message; });
	ASSERT_TRUE(link->nextDue());
	link->expire(SerialLink::Clock::now());
	EXPECT_FALSE(error) << "not yet";
	link->expire(*link->nextDue() + std::chrono::milliseconds(1));
	ASSERT_TRUE(error);
	EXPECT_NE(error->find("M36"), std::string::npos) << *error;

	link->receive(0, R"({"err":0,"fileName":"0:/gcodes/a.gcode","line":0})");
	EXPECT_EQ(unasked.size(), 1u) << "the late reply answers nothing";
}

TEST_F(TestSerialLink, RequestsStillWaitingAreToldWhenTheLinkCloses)
{
	FakePort port;
	open(port);
	std::optional<std::string> error;
	link->listFiles("0:/gcodes",
					0,
					[&error](std::expected<Comm::FileList, Comm::ReplyError> reply)
					{ error = reply ? "" : reply.error().message; });
	link->close();
	ASSERT_TRUE(error);
	EXPECT_FALSE(error->empty());
	error.reset();
	link->fileInfo("0:/gcodes/a.gcode",
				   [&error](std::expected<Comm::FileInfoReply, Comm::ReplyError> reply)
				   { error = reply ? "" : reply.error().message; });
	EXPECT_TRUE(error) << "asking a closed link is answered at once";
	EXPECT_EQ(lines(port).size(), 1u) << "and sends nothing";
}

TEST_F(TestSerialLink, FileRequestsAreReadAsWhatTheyAskFor)
{
	FakePort port;
	open(port);
	std::optional<Comm::FileList> list;
	link->listFiles(
		"0:/gcodes", 20, [&list](std::expected<Comm::FileList, Comm::ReplyError> reply) { list = reply.value(); });
	EXPECT_EQ(lines(port).back(), R"(N0 M20 S3 P"0:/gcodes" R20)");
	link->receive(0, R"({"dir":"0:/gcodes","first":20,"files":[{"type":"f","name":"a.gcode"}],"next":0,"line":0})");
	ASSERT_TRUE(list);
	EXPECT_EQ(list->files[0].name, "a.gcode");

	std::optional<Comm::ReplyError> error;
	link->fileInfo("0:/gcodes/missing.gcode",
				   [&error](std::expected<Comm::FileInfoReply, Comm::ReplyError> reply) { error = reply.error(); });
	link->receive(0, R"({"err":1,"line":1})");
	ASSERT_TRUE(error) << "an error without a file name still reaches its request";
	EXPECT_EQ(error->code, 1);

	std::optional<std::string> contents;
	link->readFile("0:/sys/heightmap.csv",
				   [&contents](std::expected<std::string, Comm::ReplyError> reply) { contents = reply.value(); });
	EXPECT_EQ(lines(port).back(), R"(N2 M36.2 P"0:/sys/heightmap.csv" S0)");
	link->receive(0, R"({"fragment":{"fileName":"0:/sys/heightmap.csv","offset":0,"data":"a,b\n","next":4},"line":2})");
	EXPECT_FALSE(contents) << "the file isn't whole yet";
	EXPECT_EQ(lines(port).back(), R"(N3 M36.2 P"0:/sys/heightmap.csv" S4)");
	link->receive(0, R"({"fragment":{"fileName":"0:/sys/heightmap.csv","offset":4,"data":"1,2\n","next":0},"line":3})");
	ASSERT_TRUE(contents);
	EXPECT_EQ(*contents, "a,b\n1,2\n");
}

TEST_F(TestSerialLink, StateRequestsGoOnTheSecondChannel)
{
	FakePort port(2);
	open(port);
	link->requestModel("", "d99f", [](auto) {});
	link->sendGcode("G28");
	link->emergencyStop();
	ASSERT_EQ(port.written.size(), 3u);
	EXPECT_EQ(port.written[0].channel, 1u) << "M409";
	EXPECT_EQ(port.written[1].channel, 0u) << "G28";
	EXPECT_EQ(port.written[2].channel, 1u) << "M112";

	FakePort single;
	open(single);
	link->requestModel("", "d99f", [](auto) {});
	EXPECT_EQ(single.written[0].channel, 0u) << "with one channel, everything goes on it";
}

TEST_F(TestSerialLink, FilesAreWrittenALineAtATimeWithTheirQuotesKept)
{
	FakePort port(2);
	port.maxUpload = 64;
	open(port);
	std::optional<bool> written;
	link->writeFile("0:/sys/a \"b\".g",
					"M118 S\"it's\"\nG4 S1",
					[&written](std::expected<void, Comm::ReplyError> result) { written = result.has_value(); });
	EXPECT_EQ(lines(port),
			  (std::vector<std::string>{R"(N0 echo >"0:/sys/a ""b"".g" "M118 S""it''s""")",
										R"(N1 echo >>"0:/sys/a ""b"".g" "G4 S1")",
										R"(N2 M409 K"state.status" F"v")"}));
	EXPECT_EQ(port.written[2].channel, 0u) << "the acknowledgement follows the lines on their channel";
	EXPECT_FALSE(written) << "not done until the printer has dealt with every line";
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":2})");
	ASSERT_TRUE(written);
	EXPECT_TRUE(*written);

	std::optional<Comm::ReplyError> tooLarge;
	link->writeFile("0:/sys/big.g",
					std::string(65, 'x'),
					[&tooLarge](std::expected<void, Comm::ReplyError> result) { tooLarge = result.error(); });
	EXPECT_TRUE(tooLarge);
}

TEST_F(TestSerialLink, AnErrorFromALineOfAFileFailsIt)
{
	FakePort port;
	open(port);
	std::optional<std::expected<void, Comm::ReplyError>> deleted;
	link->deleteFile("0:/gcodes/none.gcode",
					 [&deleted](std::expected<void, Comm::ReplyError> result) { deleted = result; });
	EXPECT_EQ(lines(port),
			  (std::vector<std::string>{R"(N0 M30 "0:/gcodes/none.gcode")", R"(N1 M409 K"state.status" F"v")"}));
	link->receive(0, R"({"resp":"Error: M30: file not found","line":0})");
	EXPECT_EQ(unasked.size(), 1u) << "the error is shown too";
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":1})");
	ASSERT_TRUE(deleted);
	ASSERT_FALSE(*deleted);
	EXPECT_EQ(deleted->error().message, "Error: M30: file not found");
}

TEST_F(TestSerialLink, ALineTooLongForTheFirmwareIsntSent)
{
	FakePort port;
	open(port);
	std::optional<Comm::ReplyError> error;
	link->writeFile("0:/sys/long.g",
					"G1 X1\n" + std::string(SerialLink::maxLineLength, 'x'),
					[&error](std::expected<void, Comm::ReplyError> result) { error = result.error(); });
	ASSERT_TRUE(error);
	EXPECT_TRUE(port.written.empty()) << "no line of it is sent, so no file is left half written";
}

TEST_F(TestSerialLink, AWriteThatFailsMeansThePortHasGone)
{
	FakePort port;
	open(port);
	EXPECT_FALSE(link->failed());
	port.writable = false;
	link->sendGcode("M115");
	EXPECT_TRUE(link->failed());
}

TEST(Gcode, TextIsQuotedAsRepRapFirmwareReadsIt)
{
	EXPECT_EQ(Comm::quoted("0:/gcodes/part.gcode"), R"("0:/gcodes/part.gcode")");
	EXPECT_EQ(Comm::quoted("Andy's part.gcode"), R"("Andy''s part.gcode")") << "a lone ' lowers the next letter";
	EXPECT_EQ(Comm::quoted(R"(say "hi")"), R"("say ""hi""")");
	EXPECT_EQ(Comm::quoted(""), R"("")");
}

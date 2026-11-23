/*
 * test_communication.cpp
 *
 * Talking to a printer over a serial port (Comm/Links/SerialLink.h), through a port the test plays the printer on:
 * how lines are sent, how replies find the request they answer by line number, and how files are written (M559).
 */

#include "Comm/Gcode.h"
#include "Comm/Links/SerialLink.h"
#include "Duet3D/General/CRC32.h"
#include "ObjectModel/Tree/Json.h"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <unistd.h>
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

	/** `text`'s CRC-32, as M559 C gives it */
	uint32_t crc32(std::string_view text)
	{
		CRC32 crc;
		crc.Update(text.data(), text.size());
		return crc.Get();
	}

	/** A file named `name` holding `contents`, in a folder of this test process's own */
	std::filesystem::path scratchFile(std::string_view name, std::string_view contents)
	{
		const std::filesystem::path folder =
			std::filesystem::temp_directory_path() / fmt::format("duetscreen_test_communication_{:d}", getpid());
		std::filesystem::create_directories(folder);
		const std::filesystem::path path = folder / name;
		std::ofstream(path, std::ios::binary) << contents;
		return path;
	}

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

	/**
	 * Write `contents` to `file`, keeping its progress in `progress` and its result in `written`; the printer says
	 * the channel is free and that it waits for the file (the replies to the first two lines written from now)
	 */
	void startWriting(FakePort& port, std::string_view file, Comm::FileContents contents)
	{
		const size_t first = port.written.size();
		link->writeFile(
			file,
			std::move(contents),
			[this](size_t sent, size_t total) { progress.push_back(fmt::format("{:d}/{:d}", sent, total)); },
			[this](std::expected<void, Comm::ReplyError> result) { written = result; });
		ASSERT_EQ(port.written.size(), first + 1);
		link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":)" + std::to_string(first) + "}");
		ASSERT_EQ(port.written.size(), first + 2);
		link->receive(0, R"({"seq":1,"resp":"Writing to file: x\n","line":)" + std::to_string(first + 1) + "}");
	}

	std::optional<SerialLink> link;								  // What is tested
	std::vector<std::string> unasked;							  // Replies given to the unasked handler, as JSON
	std::vector<std::string> progress;							  // Progress told by writeFile(), as "sent/total"
	std::optional<std::expected<void, Comm::ReplyError>> written; // What writeFile() was told, once told
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
				   [&error](std::expected<Comm::FileInfo, Comm::ReplyError> reply)
				   { error = reply ? "" : reply.error().message; });
	ASSERT_TRUE(link->nextDue());
	link->tick(SerialLink::Clock::now());
	EXPECT_FALSE(error) << "not yet";
	link->tick(*link->nextDue() + std::chrono::milliseconds(1));
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
				   [&error](std::expected<Comm::FileInfo, Comm::ReplyError> reply)
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
				   [&error](std::expected<Comm::FileInfo, Comm::ReplyError> reply) { error = reply.error(); });
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

TEST(Crc32, IsTheCrcRepRapFirmwareChecksFilesWith)
{
	EXPECT_EQ(crc32("123456789"), 0xCBF43926u) << "CRC-32/IEEE's check value";
	EXPECT_EQ(crc32(""), 0u);
}

TEST_F(TestSerialLink, AFileIsWrittenWithM559OnceTheChannelIsFree)
{
	FakePort port(2);
	open(port);
	const std::string contents = "M118 S\"it's\"\nG4 S1\n";
	link->writeFile("0:/sys/a \"b\".g",
					Comm::FileContents::bytes(contents),
					nullptr,
					[this](std::expected<void, Comm::ReplyError> result) { written = result; });
	EXPECT_EQ(lines(port), (std::vector<std::string>{R"(N0 M409 K"state.status" F"v")"}))
		<< "first, a reply on the file's channel says the channel has finished what it was doing";
	EXPECT_EQ(port.written[0].channel, 0u);

	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":0})");
	EXPECT_EQ(lines(port).back(),
			  fmt::format(R"(N1 M559 P"0:/sys/a ""b"".g" S{:d} C{:d})", contents.size(), crc32(contents)))
		<< "the name quoted as RepRapFirmware reads it, the size and the CRC";
	EXPECT_EQ(port.written[1].channel, 0u);

	link->receive(0, R"({"seq":1,"resp":"Writing to file: 0:/sys/a \"b\".g\n","line":1})");
	EXPECT_EQ(port.written.size(), 2u) << "the bytes go at the next tick";
	const std::optional<SerialLink::Clock::time_point> due = link->nextDue();
	ASSERT_TRUE(due);
	EXPECT_LE(*due, SerialLink::Clock::now()) << "at once";
	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 4u);
	EXPECT_EQ(port.written[2].bytes, contents) << "the file as it is, not as lines";
	EXPECT_EQ(port.written[2].channel, 0u);
	EXPECT_EQ(port.written[3].bytes, "N2 M409 K\"state.status\" F\"v\"\n") << "then the acknowledgement";
	EXPECT_EQ(port.written[3].channel, 0u) << "on the file's channel, where its reply follows the file";

	EXPECT_FALSE(written) << "not done until the printer has dealt with the file";
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":2})");
	ASSERT_TRUE(written);
	EXPECT_TRUE(*written);
	EXPECT_FALSE(link->waiting());
}

TEST_F(TestSerialLink, AFileIsSentInPiecesWithItsProgressTold)
{
	FakePort port;
	open(port);
	const std::string contents(2 * SerialLink::filePieceSize + 100, 'x');
	startWriting(port, "0:/gcodes/big.gcode", Comm::FileContents::bytes(contents));
	for (int piece = 0; piece < 3; piece++)
	{
		link->tick(SerialLink::Clock::now());
	}
	ASSERT_EQ(port.written.size(), 6u) << "two lines, three pieces, the acknowledgement";
	EXPECT_EQ(port.written[2].bytes + port.written[3].bytes + port.written[4].bytes, contents);
	EXPECT_EQ(port.written[2].bytes.size(), SerialLink::filePieceSize);
	EXPECT_EQ(progress, (std::vector<std::string>{"1024/2148", "2048/2148", "2148/2148"}));
	link->tick(SerialLink::Clock::now());
	EXPECT_EQ(port.written.size(), 6u) << "nothing more to send";
}

TEST_F(TestSerialLink, AFileOnTheScreenIsReadAPieceAtATimeAsItIsSent)
{
	const std::string contents = std::string(SerialLink::filePieceSize, 'a') + std::string(100, 'b');
	const std::filesystem::path path = scratchFile("upload.gcode", contents);
	auto file = Comm::FileContents::file(path);
	ASSERT_TRUE(file) << file.error().message;
	FakePort port;
	open(port);
	startWriting(port, "0:/gcodes/upload.gcode", *file);
	EXPECT_EQ(lines(port)[1],
			  fmt::format(R"(N1 M559 P"0:/gcodes/upload.gcode" S{:d} C{:d})", contents.size(), crc32(contents)))
		<< "its size and CRC are known before a byte is sent";
	link->tick(SerialLink::Clock::now());
	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 5u) << "two lines, two pieces, the acknowledgement";
	EXPECT_EQ(port.written[2].bytes + port.written[3].bytes, contents);
}

TEST_F(TestSerialLink, AFileThatCantBeReadAnyMoreMeansConnectingAgain)
{
	const std::filesystem::path path = scratchFile("pulled.gcode", std::string(2 * SerialLink::filePieceSize, 'x'));
	auto file = Comm::FileContents::file(path);
	ASSERT_TRUE(file) << file.error().message;
	FakePort port;
	open(port);
	startWriting(port, "0:/gcodes/pulled.gcode", *file);
	link->tick(SerialLink::Clock::now());
	std::filesystem::resize_file(path, 10); // as good as its drive taken out
	link->tick(SerialLink::Clock::now());
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_TRUE(link->failed()) << "the printer still waits for the rest, and would take whatever is sent next as it";
}

TEST(TestFileContents, BytesAndAFileAreReadAlike)
{
	const std::string text = "G28\nG1 X10\n";
	const std::filesystem::path path = scratchFile("contents.g", text);
	const Comm::FileContents bytes = Comm::FileContents::bytes(text);
	const auto file = Comm::FileContents::file(path);
	ASSERT_TRUE(file);
	for (const Comm::FileContents* contents : {&bytes, &*file})
	{
		EXPECT_EQ(contents->size(), text.size());
		EXPECT_EQ(contents->crc32(), crc32(text));
		EXPECT_EQ(contents->read(0, 4), "G28\n");
		EXPECT_EQ(contents->read(4, 100), "G1 X10\n") << "fewer at the end";
		EXPECT_EQ(contents->read(text.size(), 4), "") << "nothing after it";
	}
	EXPECT_FALSE(Comm::FileContents::file(path.parent_path() / "missing.g"));
}

TEST_F(TestSerialLink, NothingElseIsSentWhileAFileIs)
{
	FakePort port;
	open(port);
	link->writeFile(
		"0:/sys/f.g", Comm::FileContents::bytes(std::string(SerialLink::filePieceSize + 1, 'x')), nullptr, [](auto) {});
	std::optional<bool> answered;
	link->requestModel("heat", "v", [&answered](auto reply) { answered = reply.has_value(); });
	link->sendGcode("G28");
	EXPECT_EQ(port.written.size(), 1u) << "held while the channel is waited for";

	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":0})");
	link->receive(0, R"({"resp":"Writing to file: 0:/sys/f.g\n","line":1})");
	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 3u);
	link->emergencyStop();
	EXPECT_EQ(port.written.size(), 3u) << "the printer would take it as part of the file";
	EXPECT_TRUE(link->waiting());

	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 8u);
	EXPECT_EQ(port.written[3].bytes, "x") << "the file's last piece";
	EXPECT_EQ(port.written[4].bytes, "N2 M409 K\"state.status\" F\"v\"\n");
	EXPECT_EQ(port.written[5].bytes, "N3 M112 M999\n") << "the emergency stop goes first";
	EXPECT_EQ(port.written[6].bytes, "N4 M409 K\"heat\" F\"v\"\n") << "then the rest, in the order asked";
	EXPECT_EQ(port.written[7].bytes, "N5 G28\n");
	link->receive(0, R"({"key":"heat","flags":"v","result":{},"line":4})");
	EXPECT_EQ(answered, true);
}

TEST_F(TestSerialLink, AnEmergencyStopGoesAtOnceWhileTheChannelIsWaitedFor)
{
	FakePort port;
	open(port);
	link->writeFile("0:/sys/f.g", Comm::FileContents::bytes("x"), nullptr, [](auto) {});
	link->emergencyStop();
	EXPECT_EQ(lines(port), (std::vector<std::string>{R"(N0 M409 K"state.status" F"v")", "N1 M112 M999"}))
		<< "before M559 is sent, nothing can be taken as part of the file";
}

TEST_F(TestSerialLink, ASecondChannelTakesTheEmergencyStopWhileAFileIsSent)
{
	FakePort port(2);
	open(port);
	startWriting(port, "0:/sys/f.g", Comm::FileContents::bytes(std::string(SerialLink::filePieceSize + 1, 'x')));
	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 3u);
	link->emergencyStop();
	ASSERT_EQ(port.written.size(), 4u);
	EXPECT_EQ(port.written[3].bytes, "N2 M112 M999\n");
	EXPECT_EQ(port.written[3].channel, 1u);
	link->requestModel("", "d99f", [](auto) {});
	EXPECT_EQ(port.written.size(), 4u) << "everything else waits, even what would go on the second channel";
}

TEST_F(TestSerialLink, AFileThePrinterFindsWrongFails)
{
	FakePort port;
	open(port);
	startWriting(port, "0:/sys/f.g", Comm::FileContents::bytes("abc"));
	link->tick(SerialLink::Clock::now());
	link->receive(0, R"({"seq":2,"resp":"Error: CRC32 checksum doesn't match\n","line":1})");
	EXPECT_EQ(unasked.size(), 1u) << "the error is shown too";
	EXPECT_FALSE(written);
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":2})");
	ASSERT_TRUE(written);
	ASSERT_FALSE(*written);
	EXPECT_EQ(written->error().message, "Error: CRC32 checksum doesn't match\n");
}

TEST_F(TestSerialLink, AFileThePrinterCantOpenFails)
{
	FakePort port;
	open(port);
	link->writeFile("0:/nowhere/f.g",
					Comm::FileContents::bytes("abc"),
					nullptr,
					[this](std::expected<void, Comm::ReplyError> result) { written = result; });
	link->sendGcode("M115");
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":0})");
	link->receive(0, R"({"resp":"Error: M559: Can't open file 0:/nowhere/f.g for writing\n","line":1})");
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_EQ(lines(port).back(), "N2 M115") << "what was held is sent";
	EXPECT_FALSE(link->failed());
}

TEST_F(TestSerialLink, ABusyChannelFailsTheFileBeforeItStarts)
{
	FakePort port;
	port.timeout = std::chrono::milliseconds(100);
	open(port);
	link->writeFile("0:/sys/f.g",
					Comm::FileContents::bytes("abc"),
					nullptr,
					[this](std::expected<void, Comm::ReplyError> result) { written = result; });
	link->tick(*link->nextDue() + std::chrono::milliseconds(1));
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_FALSE(link->failed()) << "M559 wasn't sent, so nothing is left waiting for the file";
	EXPECT_EQ(lines(port).size(), 1u);
}

TEST_F(TestSerialLink, NoReplyToM559MeansConnectingAgain)
{
	FakePort port;
	port.timeout = std::chrono::milliseconds(100);
	open(port);
	link->writeFile("0:/sys/f.g",
					Comm::FileContents::bytes("abc"),
					nullptr,
					[this](std::expected<void, Comm::ReplyError> result) { written = result; });
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":0})");
	link->tick(*link->nextDue() + std::chrono::milliseconds(1));
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_TRUE(link->failed()) << "the printer may take M559 yet, and whatever is sent next as the file";
}

TEST_F(TestSerialLink, AnEmptyFileEndsWithTheEndOfFileMarker)
{
	FakePort port;
	open(port);
	link->writeFile("0:/sys/empty.g", Comm::FileContents::bytes(""), nullptr, [](auto) {});
	link->receive(0, R"({"key":"state.status","flags":"v","result":"idle","line":0})");
	EXPECT_EQ(lines(port).back(), R"(N1 M559 P"0:/sys/empty.g")") << "S0 would mean no size given";
	link->receive(0, R"({"resp":"Writing to file: 0:/sys/empty.g\n","line":1})");
	link->tick(SerialLink::Clock::now());
	ASSERT_EQ(port.written.size(), 4u);
	EXPECT_EQ(port.written[2].bytes, "<!-- **EoF** -->");
}

TEST_F(TestSerialLink, AFileTooLargeOrNamedTooLongIsntSent)
{
	FakePort port;
	port.maxUpload = 64;
	open(port);
	link->writeFile("0:/sys/big.g",
					Comm::FileContents::bytes(std::string(65, 'x')),
					nullptr,
					[this](auto result) { written = result; });
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);

	written.reset();
	link->writeFile("0:/sys/" + std::string(SerialLink::maxLineLength, 'x'),
					Comm::FileContents::bytes("x"),
					nullptr,
					[this](auto result) { written = result; });
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_TRUE(port.written.empty());
	EXPECT_FALSE(link->waiting());
}

TEST_F(TestSerialLink, AFileBeingSentAndWhatWaitsForItAreToldWhenTheLinkCloses)
{
	FakePort port;
	open(port);
	startWriting(port, "0:/sys/f.g", Comm::FileContents::bytes(std::string(SerialLink::filePieceSize + 1, 'x')));
	link->tick(SerialLink::Clock::now());
	std::optional<bool> listed;
	link->listFiles("0:/gcodes", 0, [&listed](auto reply) { listed = reply.has_value(); });
	link->close();
	ASSERT_TRUE(written);
	EXPECT_FALSE(*written);
	EXPECT_EQ(listed, false);
	EXPECT_FALSE(link->waiting());
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

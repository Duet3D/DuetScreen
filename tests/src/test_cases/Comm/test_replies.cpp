/*
 * test_replies.cpp
 *
 * How what the printer sends is taken in: gathered into lines (Comm::LineFramer) and read as the replies that aren't
 * the object model (Comm/Replies.h), from recordings of real printers, standalone and through DSF.
 */

#include "Comm/LineFramer.h"
#include "Comm/Replies.h"
#include "ObjectModel/Tree/Json.h"
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <vector>

using Comm::FileChunk;
using Comm::FileInfoReply;
using Comm::FileList;
using Comm::LineFramer;

namespace
{
	/** `text` read as JSON, which it must be */
	OM::Node json(std::string_view text)
	{
		auto node = OM::parseJson(text);
		EXPECT_TRUE(node) << text;
		return node ? std::move(*node) : OM::Node();
	}

	/** The contents of the recorded reply `name` */
	std::string recording(const std::string& name)
	{
		std::ifstream file("tests/object_model/" + name);
		EXPECT_TRUE(file.is_open()) << name;
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}
} // namespace

TEST(LineFramer, LinesArriveWholeHoweverTheBytesAreSplit)
{
	const std::string sent = "{\"a\":1}\n{\"resp\":\"ok\\n\"}\r\n\n{\"b\":[1,2]}\n";
	const std::vector<std::string> expected = {R"({"a":1})", R"({"resp":"ok\n"})", R"({"b":[1,2]})"};

	for (size_t split = 0; split <= sent.size(); split++)
	{
		std::vector<std::string> lines;
		LineFramer framer([&](std::string_view line) { lines.emplace_back(line); });
		framer.push(std::string_view(sent).substr(0, split));
		framer.push(std::string_view(sent).substr(split));
		EXPECT_EQ(lines, expected) << "split at " << split;
	}

	std::vector<std::string> lines;
	LineFramer framer([&](std::string_view line) { lines.emplace_back(line); });
	for (const char c : sent)
	{
		framer.push(std::string_view(&c, 1));
	}
	EXPECT_EQ(lines, expected) << "one byte at a time";
}

TEST(LineFramer, AnOverlongLineIsDroppedUpToItsEnd)
{
	std::vector<std::string> lines;
	LineFramer framer([&](std::string_view line) { lines.emplace_back(line); });
	framer.push(std::string(LineFramer::maxLineLength, 'x'));
	framer.push("yz\n{\"next\":1}\n");
	EXPECT_EQ(lines, std::vector<std::string>{R"({"next":1})"});
}

TEST(Replies, AFolderListingIsRead)
{
	const auto list = FileList::read(json(recording("rr_filelist/gcodes.json")));
	ASSERT_TRUE(list) << list.error().message;
	EXPECT_EQ(list->dir, "0:/gcodes/");
	EXPECT_EQ(list->first, 0u);
	EXPECT_EQ(list->next, 0u);
	ASSERT_EQ(list->files.size(), 2u);
	EXPECT_FALSE(list->files[0].folder);
	EXPECT_EQ(list->files[0].name, "test.gcode");
	EXPECT_EQ(list->files[0].size, 712685u);
	EXPECT_EQ(list->files[0].date, "2025-03-18T18:21:58");
	EXPECT_TRUE(list->files[1].folder);

	const auto page = FileList::read(json(R"({"dir":"0:/gcodes","first":20,"files":[],"next":40,"err":0})"));
	ASSERT_TRUE(page);
	EXPECT_EQ(page->first, 20u);
	EXPECT_EQ(page->next, 40u);

	const auto dsf = FileList::read(json(R"([{"type":"d","name":"jobtest","date":"2026-08-27T13:54:52"},)"
										 R"({"type":"f","name":"Andy's part.gcode","size":54}])"),
									"0:/gcodes");
	ASSERT_TRUE(dsf) << "DSF's API sends a bare array";
	EXPECT_EQ(dsf->dir, "0:/gcodes");
	EXPECT_EQ(dsf->files[1].name, "Andy's part.gcode");

	const auto missing = FileList::read(json(R"({"dir":"0:/nothing","err":2})"));
	ASSERT_FALSE(missing);
	EXPECT_EQ(missing.error().code, 2);
	EXPECT_FALSE(FileList::read(json(R"({"dir":"0:/gcodes"})"))) << "a listing without files";
}

TEST(Replies, FileInformationIsRead)
{
	// A real Duet in SBC mode: DSF escapes the '+'
	const auto info = FileInfoReply::read(
		json(R"({"err":0,"fileName":"0:/gcodes/test.gcode","size":5910665,"lastModified":"2026-05-07T10:40:04",)"
			 R"("height":22.45,"layerHeight":0.2,"numLayers":112,"printTime":3089,"filament":[3811.5,3269.7],)"
			 R"("thumbnails":[{"width":32,"height":32,"format":"png","offset":104,"size":2404},)"
			 R"({"width":400,"height":300,"format":"png","offset":2654,"size":65048}],)"
			 R"("generatedBy":"PrusaSlicer 2.7.1+win64 on 2024-01-31 at 15:01:35 UTC"})"));
	ASSERT_TRUE(info) << info.error().message;
	EXPECT_EQ(info->fileName, "0:/gcodes/test.gcode");
	EXPECT_EQ(info->size, 5910665u);
	EXPECT_EQ(info->lastModified, "2026-05-07T10:40:04");
	EXPECT_DOUBLE_EQ(info->height, 22.45);
	EXPECT_DOUBLE_EQ(info->layerHeight, 0.2);
	EXPECT_EQ(info->numLayers, 112u);
	EXPECT_EQ(info->printTime, 3089u);
	EXPECT_EQ(info->simulatedTime, 0u);
	EXPECT_EQ(info->filament, (std::vector<double>{3811.5, 3269.7}));
	EXPECT_EQ(info->generatedBy, "PrusaSlicer 2.7.1+win64 on 2024-01-31 at 15:01:35 UTC");
	ASSERT_EQ(info->thumbnails.size(), 2u);
	EXPECT_EQ(info->thumbnails[1].format, "png");
	EXPECT_EQ(info->thumbnails[1].width, 400u);
	EXPECT_EQ(info->thumbnails[1].offset, 2654u);
	EXPECT_EQ(info->thumbnails[1].size, 65048u);

	const auto bare = FileInfoReply::read(json(R"({"err":0,"fileName":"0:/gcodes/empty.gcode","size":0})"));
	ASSERT_TRUE(bare) << "a file without a slicer's information still has information";
	EXPECT_TRUE(bare->generatedBy.empty());

	const auto notFound = FileInfoReply::read(json(R"({"err":1})"));
	ASSERT_FALSE(notFound);
	EXPECT_EQ(notFound.error().code, 1);
}

TEST(Replies, PiecesOfFilesAreRead)
{
	// Over USB and UART the fields are wrapped; over the network they aren't
	const auto serial = FileChunk::read(
		json(R"({"thumbnail":{"fileName":"0:/gcodes/a.gcode","offset":104,"data":"iVBORw0KGgo=","next":0,"err":0}})"),
		"thumbnail");
	ASSERT_TRUE(serial) << serial.error().message;
	EXPECT_EQ(serial->fileName, "0:/gcodes/a.gcode");
	EXPECT_EQ(serial->offset, 104u);
	EXPECT_EQ(serial->data, "iVBORw0KGgo=");
	EXPECT_EQ(serial->next, 0u);

	const auto network = FileChunk::read(
		json(R"({"fileName":"0:/gcodes/a.gcode","offset":104,"data":"iVBO+Rw0K","next":1104,"err":0})"), "thumbnail");
	ASSERT_TRUE(network);
	EXPECT_EQ(network->data, "iVBO+Rw0K") << "DSF escapes '+', which base64 uses";
	EXPECT_EQ(network->next, 1104u);

	const auto contents = FileChunk::read(
		json(R"({"fragment":{"fileName":"0:/sys/heightmap.csv","offset":0,"data":"a,b\n1,2\n","next":0}})"),
		"fragment");
	ASSERT_TRUE(contents);
	EXPECT_EQ(contents->data, "a,b\n1,2\n");

	const auto failed = FileChunk::read(json(R"({"thumbnail":{"fileName":"0:/gcodes/a.gcode","err":1}})"), "thumbnail");
	ASSERT_FALSE(failed);
	EXPECT_EQ(failed.error().code, 1);
	EXPECT_FALSE(FileChunk::read(json(R"({"thumbnail":{"fileName":"x","offset":0}})"), "thumbnail")) << "no data";
}

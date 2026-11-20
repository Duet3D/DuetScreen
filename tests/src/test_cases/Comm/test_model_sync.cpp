/*
 * test_model_sync.cpp
 *
 * Keeping the object model up to date (Comm/ModelSync.h): what is asked for, when, and what is done with the replies,
 * through a link the test answers itself.
 */

#include "Comm/ModelSync.h"
#include "ObjectModel/Tree/Json.h"
#include <deque>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using Comm::ModelSync;
using Clock = ModelSync::Clock;

namespace
{
	/** A link that keeps what it is asked for until the test answers */
	class FakeLink : public Comm::ModelLink
	{
	  public:
		/** One request waiting for its answer */
		struct Request
		{
			std::string key;	  // What was asked for
			std::string flags;	  // With which flags
			ModelHandler handler; // Given the answer
		};

		void requestModel(std::string_view key, std::string_view flags, ModelHandler handler) override
		{
			requests.push_back({std::string(key), std::string(flags), std::move(handler)});
		}

		void fetchResponses() override { responsesFetched++; }

		/** "key flags" of each request waiting, oldest first */
		std::vector<std::string> waiting() const
		{
			std::vector<std::string> list;
			for (const Request& request : requests)
			{
				list.push_back(request.key + " " + request.flags);
			}
			return list;
		}

		/** Answer the oldest request with `json` */
		void answer(std::string_view json)
		{
			ASSERT_FALSE(requests.empty());
			Request request = std::move(requests.front());
			requests.pop_front();
			auto reply = OM::parseJson(json);
			ASSERT_TRUE(reply) << json;
			request.handler(std::move(*reply));
		}

		/** Answer the oldest request with a detailed reply for its key holding `result` */
		void answerResult(std::string_view result, size_t next = 0)
		{
			ASSERT_FALSE(requests.empty());
			const Request& request = requests.front();
			answer(fmt::format(R"({{"key":"{:s}","flags":"{:s}","result":{:s},"next":{:d}}})",
							   request.key,
							   request.flags,
							   result,
							   next));
		}

		/** Fail the oldest request */
		void fail()
		{
			ASSERT_FALSE(requests.empty());
			Request request = std::move(requests.front());
			requests.pop_front();
			request.handler(std::unexpected(Comm::ReplyError{.message = "timed out"}));
		}

		std::deque<Request> requests; // Waiting for answers, oldest first
		int responsesFetched = 0;	  // How often fetchResponses() was called
	};

	/** A live model reply with `seqs` */
	std::string live(std::string_view seqs, std::string_view more = "")
	{
		return fmt::format(R"({{"key":"","flags":"d99f","result":{{"seqs":{{{:s}}}{:s}}}}})", seqs, more);
	}
} // namespace

class TestModelSync : public ::testing::Test
{
  protected:
	FakeLink link;				// Where the model is asked for
	OM::Tree tree;				// The model kept
	ModelSync sync{link, tree}; // What is tested
};

TEST_F(TestModelSync, OnlyWantedKeysAreFetchedOneAfterAnother)
{
	tree.want({"heat.heaters[0].current", "state.status", "fans"});
	sync.tick(Clock::now());
	EXPECT_EQ(link.waiting(), std::vector<std::string>{" d99f"}) << "nothing in detail before the first live model";

	link.answer(live(R"("heat":1,"state":2,"fans":3,"tools":4,"reply":0)"));
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"state vn"}) << "the most needed wanted key first";

	link.answerResult(R"({"status":"idle"})");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"heat vo"}) << "the next follows at once, without a poll";
	link.answerResult(R"({"heaters":[{"current":20}]})");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"fans v"});
	link.answerResult(R"([{"actualValue":0.5}])");
	EXPECT_TRUE(link.waiting().empty()) << "tools isn't wanted";
	EXPECT_TRUE(sync.idle());

	EXPECT_EQ(tree.root().at("state.status").text(), "idle");
	EXPECT_EQ(tree.root().at("heat.heaters[0].current").integer(), 20);
	EXPECT_DOUBLE_EQ(tree.root().at("fans[0].actualValue").number(), 0.5);
	EXPECT_EQ(tree.root().at("tools").kind(), OM::Node::Kind::missing);
}

TEST_F(TestModelSync, AKeyIsFetchedAgainWhenItsSeqChanges)
{
	tree.want({"heat", "fans"});
	sync.pollNow();
	link.answer(live(R"("heat":1,"fans":1)"));
	link.answerResult("{}");
	link.answerResult("[]");
	ASSERT_TRUE(sync.idle());

	sync.pollNow();
	link.answer(live(R"("heat":1,"fans":1)"));
	EXPECT_TRUE(link.waiting().empty()) << "nothing changed";

	sync.pollNow();
	link.answer(live(R"("heat":1,"fans":2)"));
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"fans v"});
	link.answerResult("[]");

	// A key that changes while it is fetched is fetched again
	sync.pollNow();
	link.answer(live(R"("heat":2,"fans":2)"));
	ASSERT_EQ(link.waiting(), std::vector<std::string>{"heat vo"});
	sync.pollNow();
	link.answer(live(R"("heat":3,"fans":2)"));
	link.answerResult("{}");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"heat vo"});
}

TEST_F(TestModelSync, AKeyWantedLaterIsFetchedThen)
{
	sync.pollNow();
	link.answer(live(R"("global":4)"));
	EXPECT_TRUE(link.waiting().empty());

	const size_t want = tree.want({"global.myValue"});
	sync.tick(Clock::now());
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"global v"}) << "fetched as soon as it is wanted";
	link.answerResult(R"({"myValue":7})");
	EXPECT_EQ(tree.root().at("global.myValue").integer(), 7);

	tree.unwant(want);
	sync.pollNow();
	link.answer(live(R"("global":5)"));
	EXPECT_TRUE(link.waiting().empty()) << "no longer wanted";
}

TEST_F(TestModelSync, KeysTheFirmwareDoesntNumberAreFetchedOnce)
{
	tree.want({"limits"});
	sync.pollNow();
	link.answer(live(R"("heat":1)"));
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"limits v"});
	link.answerResult(R"({"axes":10})");
	sync.pollNow();
	link.answer(live(R"("heat":2)"));
	EXPECT_TRUE(link.waiting().empty());
}

TEST_F(TestModelSync, SplitArraysAreFetchedOnWhereTheyStopped)
{
	tree.want({"tools"});
	sync.pollNow();
	link.answer(live(R"("tools":1)"));
	link.answer(R"({"key":"tools","flags":"v","result":[{"number":0},{"number":1}],"next":2})");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"tools va2"});
	link.answer(R"({"key":"tools","flags":"va2","result":[{"number":2}],"next":0})");
	EXPECT_TRUE(sync.idle());
	ASSERT_EQ(tree.root().at("tools").size(), 3u);
	EXPECT_EQ(tree.root().at("tools[2].number").integer(), 2);
}

TEST_F(TestModelSync, AxesBeyondThoseInMoveAreFetchedFromMoveAxes)
{
	tree.want({"move"});
	sync.pollNow();
	link.answer(live(R"("move":1)"));
	link.answerResult(R"({"axes":[{"letter":"X"},{"letter":"Y"},{"letter":"Z"},{"letter":"U"},{"letter":"V"}]})");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"move.axes vna5"});
	link.answer(R"({"key":"move.axes","flags":"vna5","result":[{"letter":"W"},{"letter":"A"}],"next":0})");
	EXPECT_TRUE(sync.idle());
	ASSERT_EQ(tree.root().at("move.axes").size(), 7u);
	EXPECT_EQ(tree.root().at("move.axes[6].letter").text(), "A");
}

TEST_F(TestModelSync, AFailedFetchIsTriedAgainAfterTheNextLiveModel)
{
	tree.want({"fans"});
	sync.pollNow();
	link.answer(live(R"("fans":1)"));
	link.fail();
	EXPECT_TRUE(link.waiting().empty()) << "not at once, so a printer that doesn't answer isn't flooded";
	sync.pollNow();
	link.answer(live(R"("fans":1)"));
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"fans v"});
}

TEST_F(TestModelSync, TheLiveModelIsAskedForEachPollInterval)
{
	sync.setPollInterval(std::chrono::milliseconds(250));
	const Clock::time_point start = Clock::now();
	sync.tick(start);
	ASSERT_EQ(link.waiting().size(), 1u);
	sync.tick(start + std::chrono::seconds(1));
	EXPECT_EQ(link.waiting().size(), 1u) << "not again while it is unanswered";
	link.answer(live(""));
	sync.tick(start + std::chrono::milliseconds(1100));
	EXPECT_EQ(link.waiting().size(), 1u);
	link.answer(live(""));
	sync.tick(start + std::chrono::milliseconds(1200));
	EXPECT_TRUE(link.waiting().empty()) << "not before the interval has passed";
	sync.tick(start + std::chrono::milliseconds(1350));
	EXPECT_EQ(link.waiting().size(), 1u);
}

TEST_F(TestModelSync, ResponsesAreFetchedWhenTheReplySeqChanges)
{
	sync.pollNow();
	link.answer(live(R"("reply":3)"));
	EXPECT_EQ(link.responsesFetched, 1);
	sync.pollNow();
	link.answer(live(R"("reply":3)"));
	EXPECT_EQ(link.responsesFetched, 1);
	sync.pollNow();
	link.answer(live(R"("reply":4)"));
	EXPECT_EQ(link.responsesFetched, 2);
}

TEST_F(TestModelSync, AReplyAskedForBeforeAResetIsIgnored)
{
	tree.want({"fans"});
	sync.pollNow();
	link.answer(live(R"("fans":1)"));
	sync.reset();
	link.answerResult(R"([{"name":"old"}])");
	EXPECT_EQ(tree.root().at("fans").kind(), OM::Node::Kind::missing);
	EXPECT_TRUE(sync.idle());
}

TEST_F(TestModelSync, AStatusReportNobodyAskedForIsMerged)
{
	tree.want({"fans"});
	sync.pollNow();
	link.answer(live(R"("fans":1)"));
	link.answerResult("[]");
	auto report = OM::parseJson(live(R"("fans":2)", R"(,"state":{"status":"busy"})"));
	ASSERT_TRUE(report);
	sync.receiveUnasked(*OM::Patch::fromReply(*report));
	EXPECT_EQ(tree.root().at("state.status").text(), "busy");
	EXPECT_EQ(link.waiting(), std::vector<std::string>{"fans v"}) << "its seqs count too";
}

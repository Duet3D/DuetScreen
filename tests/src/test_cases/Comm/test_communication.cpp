/*
 * test_communication.cpp
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#include "Comm/Communication.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "test_utils/TestSuite.h"
#include <gtest/gtest.h>
#include <vector>

class TestCommunication : public TestSuite
{
  public:
	TestCommunication() {}
};

TEST_F(TestCommunication, SendNext)
{
	/* Need to set uart mode since it the duet needs to be "connected" for sendNext to work. Uart mode fakes this in
	 * SIMULATION */
	Comm::DUET.SetCommunicationType(Comm::CommunicationType::uart);

	auto startTime = TimeHelper::getRunningTime();
	while (!Comm::DUET.IsConnected())
	{
		ASSERT_LT(TimeHelper::getTimeSince(startTime), std::chrono::seconds(5))
			<< "Timed out waiting for Duet to connect in UART mode";
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	std::vector<const char*> expectedOrder = {
		"", "network", "", "boards", "", "move", "", "heat",	"", "tools", "", "spindles", "", "directories",
		"", "fans",	   "", "inputs", "", "job",	 "", "sensors", "", "state", "", "volumes"};

	// Verify strict ordering by simulating completion after each sent request.
	for (const char* expectedKey : expectedOrder)
	{
		EXPECT_TRUE(Comm::sendNext());
		ASSERT_NE(Comm::g_currentReqSeq, nullptr);
		EXPECT_EQ(Comm::g_currentReqSeq->state, Comm::SeqState::SeqStateRequested);
		EXPECT_STREQ(Comm::g_currentReqSeq->key, expectedKey);

		// Simulate that this request has started/finished receiving so the scheduler can move on.
		Comm::g_currentReqSeq->state = Comm::SeqState::SeqStateOk;
	}

	// If the previous request has not started receiving yet (still requested), don't advance.
	EXPECT_TRUE(Comm::sendNext());
	ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	EXPECT_EQ(Comm::g_currentReqSeq->state, Comm::SeqState::SeqStateRequested);
	const char* inProgressKey = Comm::g_currentReqSeq->key;

	EXPECT_FALSE(Comm::sendNext());
	ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	EXPECT_STREQ(Comm::g_currentReqSeq->key, inProgressKey);

	// Release the in-progress seq so we can validate update/error scheduling behavior.
	Comm::g_currentReqSeq->state = Comm::SeqState::SeqStateOk;

	auto setSeqState = [](const char* key, Comm::SeqState state)
	{
		auto* seq = Comm::FindSeqByKey(key);
		ASSERT_NE(seq, nullptr);
		seq->state = state;
	};

	// Normalize all known seqs to Ok so the following checks are deterministic.
	setSeqState("", Comm::SeqState::SeqStateOk);
	setSeqState("network", Comm::SeqState::SeqStateOk);
	setSeqState("boards", Comm::SeqState::SeqStateOk);
	setSeqState("move", Comm::SeqState::SeqStateOk);
	setSeqState("heat", Comm::SeqState::SeqStateOk);
	setSeqState("tools", Comm::SeqState::SeqStateOk);
	setSeqState("spindles", Comm::SeqState::SeqStateOk);
	setSeqState("directories", Comm::SeqState::SeqStateOk);
	setSeqState("fans", Comm::SeqState::SeqStateOk);
	setSeqState("inputs", Comm::SeqState::SeqStateOk);
	setSeqState("job", Comm::SeqState::SeqStateOk);
	setSeqState("sensors", Comm::SeqState::SeqStateOk);
	setSeqState("state", Comm::SeqState::SeqStateOk);
	setSeqState("volumes", Comm::SeqState::SeqStateOk);

	// SeqStateUpdate: updated seq should be the next regular seq chosen.
	setSeqState("move", Comm::SeqState::SeqStateUpdate);
	EXPECT_TRUE(Comm::sendNext());
	ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	if (Comm::g_currentReqSeq->key[0] == '\0')
	{
		Comm::g_currentReqSeq->state = Comm::SeqState::SeqStateOk;
		EXPECT_TRUE(Comm::sendNext());
		ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	}
	EXPECT_STREQ(Comm::g_currentReqSeq->key, "move");
	Comm::g_currentReqSeq->state = Comm::SeqState::SeqStateOk;

	// SeqStateError: errored seq should be reset to Init and skipped this cycle.
	auto* boardsSeq = Comm::FindSeqByKey("boards");
	ASSERT_NE(boardsSeq, nullptr);
	setSeqState("boards", Comm::SeqState::SeqStateError);
	setSeqState("tools", Comm::SeqState::SeqStateUpdate);

	EXPECT_TRUE(Comm::sendNext());
	ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	if (Comm::g_currentReqSeq->key[0] == '\0')
	{
		Comm::g_currentReqSeq->state = Comm::SeqState::SeqStateOk;
		EXPECT_TRUE(Comm::sendNext());
		ASSERT_NE(Comm::g_currentReqSeq, nullptr);
	}

	EXPECT_STREQ(Comm::g_currentReqSeq->key, "tools");
	EXPECT_EQ(boardsSeq->state, Comm::SeqState::SeqStateInit);
}

TEST_F(TestCommunication, GetInteger)
{
	int32_t val = 0;

	EXPECT_FALSE(Comm::GetInteger(nullptr, val));
	EXPECT_EQ(val, 0);

	EXPECT_FALSE(Comm::GetInteger("", val));
	EXPECT_EQ(val, 0);

	/* Test that values beyond the int32_t range are not accepted */
	EXPECT_TRUE(Comm::GetInteger("2147483647", val));
	EXPECT_EQ(val, 2147483647);
	EXPECT_FALSE(Comm::GetInteger("2147483648", val));
	EXPECT_FALSE(Comm::GetInteger("123456789012", val));
	EXPECT_TRUE(Comm::GetInteger("-2147483648", val));
	EXPECT_EQ(val, -2147483648);
	EXPECT_FALSE(Comm::GetInteger("-2147483649", val));

	EXPECT_TRUE(Comm::GetInteger("123", val));
	EXPECT_EQ(val, 123);

	EXPECT_TRUE(Comm::GetInteger("-123", val));
	EXPECT_EQ(val, -123);

	/* Test that leading whitespace is ignored */
	EXPECT_TRUE(Comm::GetInteger("   123", val));
	EXPECT_EQ(val, 123);

	EXPECT_TRUE(Comm::GetInteger("   -123", val));
	EXPECT_EQ(val, -123);

	/* Test that trailing whitespace is not accepted */
	EXPECT_FALSE(Comm::GetInteger("123   ", val));

	EXPECT_FALSE(Comm::GetInteger("-123   ", val));

	/* Test that non-numeric strings are not accepted */
	EXPECT_FALSE(Comm::GetInteger("123abc", val));

	EXPECT_FALSE(Comm::GetInteger("-123abc", val));

	/* Test that floating point numbers are rounded */
	EXPECT_TRUE(Comm::GetInteger("   123.45", val));
	EXPECT_EQ(val, 123);

	EXPECT_TRUE(Comm::GetInteger("   123.54", val));
	EXPECT_EQ(val, 124);

	EXPECT_TRUE(Comm::GetInteger("   -123.45", val));
	EXPECT_EQ(val, -123);

	EXPECT_TRUE(Comm::GetInteger("   -123.54", val));
	EXPECT_EQ(val, -124);

	/* Test scientific format */
	EXPECT_TRUE(Comm::GetInteger("1.2345e2", val));
	EXPECT_EQ(val, 123);
}

TEST_F(TestCommunication, GetUnsignedInteger)
{
	uint32_t val = 0;

	EXPECT_FALSE(Comm::GetUnsignedInteger(nullptr, val));
	EXPECT_EQ(val, 0);

	EXPECT_FALSE(Comm::GetUnsignedInteger("", val));
	EXPECT_EQ(val, 0);

	/* Test that values beyond the uint32_t range are not accepted */
	EXPECT_TRUE(Comm::GetUnsignedInteger("4294967295", val));
	EXPECT_EQ(val, 4294967295U);
	EXPECT_FALSE(Comm::GetUnsignedInteger("4294967296", val));
	EXPECT_FALSE(Comm::GetUnsignedInteger("12345678901", val));
	EXPECT_FALSE(Comm::GetUnsignedInteger("-1", val));

	EXPECT_TRUE(Comm::GetUnsignedInteger("123", val));
	EXPECT_EQ(val, 123);

	/* Test that leading whitespace is ignored */
	EXPECT_TRUE(Comm::GetUnsignedInteger("   123", val));
	EXPECT_EQ(val, 123);

	/* Test that trailing whitespace is not accepted */
	EXPECT_FALSE(Comm::GetUnsignedInteger("123   ", val));

	/* Test that floating point numbers are rounded */
	EXPECT_TRUE(Comm::GetUnsignedInteger("   123.45", val));
	EXPECT_EQ(val, 123);

	EXPECT_TRUE(Comm::GetUnsignedInteger("   123.54", val));
	EXPECT_EQ(val, 124);

	/* Test scientific format */
	EXPECT_TRUE(Comm::GetUnsignedInteger("1.2345e2", val));
	EXPECT_EQ(val, 123);
}

TEST_F(TestCommunication, GetBool)
{
	bool val = false;

	EXPECT_FALSE(Comm::GetBool(nullptr, val));

	EXPECT_FALSE(Comm::GetBool("", val));

	EXPECT_TRUE(Comm::GetBool("true", val));
	EXPECT_EQ(val, true);

	EXPECT_TRUE(Comm::GetBool("false", val));
	EXPECT_EQ(val, false);

	EXPECT_TRUE(Comm::GetBool("True", val));
	EXPECT_EQ(val, true);

	EXPECT_TRUE(Comm::GetBool("FALSE", val));
	EXPECT_EQ(val, false);

	EXPECT_TRUE(Comm::GetBool("yes", val));
	EXPECT_EQ(val, false);
}

TEST_F(TestCommunication, GetFloat)
{
	float val = 0.0f;

	EXPECT_FALSE(Comm::GetFloat(nullptr, val));
	EXPECT_EQ(val, 0.0f);

	EXPECT_FALSE(Comm::GetFloat("", val));
	EXPECT_EQ(val, 0.0f);

	EXPECT_TRUE(Comm::GetFloat("123.45", val));
	EXPECT_FLOAT_EQ(val, 123.45f);

	EXPECT_TRUE(Comm::GetFloat("-123.45", val));
	EXPECT_FLOAT_EQ(val, -123.45f);

	/* Test that leading whitespace is ignored */
	EXPECT_TRUE(Comm::GetFloat("   123.45", val));
	EXPECT_FLOAT_EQ(val, 123.45f);

	EXPECT_TRUE(Comm::GetFloat("   -123.45", val));
	EXPECT_FLOAT_EQ(val, -123.45f);

	/* Test that trailing whitespace is not accepted */
	EXPECT_FALSE(Comm::GetFloat("123.45   ", val));

	EXPECT_FALSE(Comm::GetFloat("-123.45   ", val));

	/* Test that non-numeric strings are not accepted */
	EXPECT_FALSE(Comm::GetFloat("123.45abc", val));

	EXPECT_FALSE(Comm::GetFloat("-123.45abc", val));

	/* Test scientific format */
	EXPECT_TRUE(Comm::GetFloat("1.2345e2", val));
	EXPECT_FLOAT_EQ(val, 123.45f);
}
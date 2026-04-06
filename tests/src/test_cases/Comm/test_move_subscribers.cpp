/*
 * test_move_subscribers.cpp
 *
 *  Created on: 2026-04-06
 */

#include "ObjectModel/MotionSystem.h"
#include "test_utils/TestSuite.h"

class TestMoveSubscribers : public TestSuite
{
  public:
	TestMoveSubscribers() {}
};

TEST_F(TestMoveSubscribers, MotionSystemsSubscribersPopulateNestedFields)
{
	constexpr std::string_view payload =
		R"json({"key":"move","flags":"vn","result":{"motionSystems":[{"currentMove":{"acceleration":1234.5,"deceleration":2345.5,"distance":97.96,"duration":0.33,"extrusionRate":5.1,"laserPwm":0.75,"requestedSpeed":300,"topSpeed":325},"currentObject":7,"currentTool":1,"nextTool":2,"previousTool":0,"printingAcceleration":50000,"rotation":{"angle":180,"centre":[168,0]},"speedFactor":1.2,"travelAcceleration":52000,"userPosition":[-103.292,3.002,0.25],"virtualEPos":70560.36,"workplaceNumber":2}],"next":0},"next":0})json";

	ASSERT_TRUE(load_model_data(payload));

	auto motionSystem = OM::Move::GetMotionSystem(0);
	ASSERT_NE(motionSystem, nullptr);

	EXPECT_FLOAT_EQ(motionSystem->currentMove.acceleration, 1234.5f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.deceleration, 2345.5f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.distance, 97.96f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.duration, 0.33f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.extrusionRate, 5.1f);
	ASSERT_TRUE(motionSystem->currentMove.laserPwm.has_value());
	EXPECT_FLOAT_EQ(*motionSystem->currentMove.laserPwm, 0.75f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.requestedSpeed, 300.0f);
	EXPECT_FLOAT_EQ(motionSystem->currentMove.topSpeed, 325.0f);

	ASSERT_TRUE(motionSystem->currentObject.has_value());
	EXPECT_EQ(*motionSystem->currentObject, 7);
	EXPECT_EQ(motionSystem->currentTool, 1);
	EXPECT_EQ(motionSystem->nextTool, 2);
	EXPECT_EQ(motionSystem->previousTool, 0);
	EXPECT_FLOAT_EQ(motionSystem->printingAcceleration, 50000.0f);
	EXPECT_FLOAT_EQ(motionSystem->rotation.angle, 180.0f);
	EXPECT_FLOAT_EQ(motionSystem->rotation.centre[0], 168.0f);
	EXPECT_FLOAT_EQ(motionSystem->rotation.centre[1], 0.0f);
	EXPECT_FLOAT_EQ(motionSystem->speedFactor, 1.2f);
	EXPECT_FLOAT_EQ(motionSystem->travelAcceleration, 52000.0f);
	EXPECT_FLOAT_EQ(motionSystem->userPosition[0], -103.292f);
	EXPECT_FLOAT_EQ(motionSystem->userPosition[1], 3.002f);
	EXPECT_FLOAT_EQ(motionSystem->userPosition[2], 0.25f);
	EXPECT_FLOAT_EQ(motionSystem->virtualEPos, 70560.36f);
	EXPECT_EQ(motionSystem->workplaceNumber, 2);
}

TEST_F(TestMoveSubscribers, MotionSystemsSubscribersClearOptionalFieldsFromNull)
{
	constexpr std::string_view populatedPayload =
		R"json({"key":"move","flags":"vn","result":{"motionSystems":[{"currentMove":{"laserPwm":0.42},"currentObject":3}],"next":0},"next":0})json";
	constexpr std::string_view clearedPayload =
		R"json({"key":"move","flags":"vn","result":{"motionSystems":[{"currentMove":{"laserPwm":null},"currentObject":null}],"next":0},"next":0})json";

	ASSERT_TRUE(load_model_data(populatedPayload));
	auto motionSystem = OM::Move::GetMotionSystem(0);
	ASSERT_NE(motionSystem, nullptr);
	ASSERT_TRUE(motionSystem->currentObject.has_value());
	ASSERT_TRUE(motionSystem->currentMove.laserPwm.has_value());

	ASSERT_TRUE(load_model_data(clearedPayload));

	motionSystem = OM::Move::GetMotionSystem(0);
	ASSERT_NE(motionSystem, nullptr);
	EXPECT_FALSE(motionSystem->currentObject.has_value());
	EXPECT_FALSE(motionSystem->currentMove.laserPwm.has_value());
}
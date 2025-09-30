/*
 * test_move_view.cpp
 *
 *  Created on: 2025-09-04
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Screens/Move/MoveView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestMoveView : public UiTestSuite
{
  public:
	TestMoveView() {}

	MoveView move_view{screen};
};

TEST_F(TestMoveView, Basic)
{
	EXPECT_EQUAL_SCREENSHOT("move_view_basic.png");
}

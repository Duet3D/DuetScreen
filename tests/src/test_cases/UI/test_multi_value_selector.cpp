/*
 * test_multi_value_selector.cpp
 *
 *  Created on: 2025-11-16
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/Input/MultiValueSelector.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestMultiValueSelector : public UiTestSuite
{
  public:
	TestMultiValueSelector() {}
};

TEST_F(TestMultiValueSelector, Basic)
{
	MultiValueSelector mvs("multiValueSelector", screen);

	mvs.setValues({0.1f, 0.5f, 1.0f, 5.0f, 10.0f});

	EXPECT_EQUAL_SCREENSHOT("multi_value_selector/basic.png");
}
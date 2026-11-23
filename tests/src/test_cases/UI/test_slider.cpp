/*
 * test_slider.cpp
 *
 *  Created on: 2025-10-22
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Widgets/Inputs/Slider.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestSlider : public UiTestSuite
{
  public:
	TestSlider() {}
};

TEST_F(TestSlider, CreateSlider)
{
	Slider slider("test_slider", screen);
	slider.setLabel("Header");
	slider.setRange(0.0f, 100.0f);
	slider.setValue(50.0f);
	slider.setIncrementValue(5.0f);

	EXPECT_FLOAT_EQ(slider.getValue(), 50.0f);
	EXPECT_FLOAT_EQ(slider.getMin(), 0.0f);
	EXPECT_FLOAT_EQ(slider.getMax(), 100.0f);

	slider.setSize(LV_PCT(50), LV_SIZE_CONTENT);

	EXPECT_EQUAL_SCREENSHOT("slider/basic.png");
}
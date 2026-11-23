/*
 * test_bar.cpp
 *
 *  Created on: 2026-01-09
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Widgets/Display/Bar.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestBar : public UiTestSuite
{
  public:
	TestBar() {}
};

TEST_F(TestBar, Basic)
{
	LvContainer container("container", screen);
	container.setSize(220, LV_SIZE_CONTENT);
	Bar bar("bar", screen);
	bar.setSize(LV_PCT(100), 30);

	Bar bar2("bar2", screen);
	bar2.setSize(LV_PCT(100), 30);
	bar2.setValue(50, LV_ANIM_OFF);

	Bar bar3("bar3", screen);
	bar3.setSize(LV_PCT(100), 30);
	bar3.setValue(100, LV_ANIM_OFF);

	Bar bar4("bar4", screen);
	bar4.setSize(LV_PCT(100), 50);
	bar4.setValue(100, LV_ANIM_OFF);
	bar4.setMaxValue(200);

	Bar bar5("bar5", screen);
	bar5.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	bar5.setRange(140, 200);
	bar5.setValue(150, LV_ANIM_OFF);

	Bar bar6("bar6", screen);
	bar6.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	bar6.setRange(-50, 50);
	bar6.setValue(-20, LV_ANIM_OFF);
	bar6.setLabelFormat("Value\n{}");

	screen.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("bar/bar.png");
}

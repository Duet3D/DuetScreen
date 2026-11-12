/*
 * test_app_drawer.cpp
 *
 *  Created on: 2025-08-18
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/AppDrawer/AppDrawer.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestAppDrawer : public UiTestSuite
{
  public:
	TestAppDrawer()
		: app_drawer("app_drawer", screen)
	{
		app_drawer.init();
	}

	UI::AppDrawer app_drawer;
};

TEST_F(TestAppDrawer, SizeContent)
{
	app_drawer.setSize(LV_SIZE_CONTENT, LV_PCT(100));

	EXPECT_EQUAL_SCREENSHOT("app_drawer/content.png");
}

TEST_F(TestAppDrawer, FixedSize)
{
	app_drawer.setSize(500, 400);

	EXPECT_EQUAL_SCREENSHOT("app_drawer/fixed.png");
}

/*
 * test_app_drawer.cpp
 *
 *  Created on: 2025-08-18
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/AppDrawer/AppDrawer.h"
#include "UI/Screens/Home/HomeView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestAppDrawer : public UiTestSuite
{
  public:
	TestAppDrawer()
		: app_drawer("app_drawer", screen)
	{
		UI::HomeView::setInstance(&home);
		home.hide();

		app_drawer.init();
	}
	virtual ~TestAppDrawer() { UI::HomeView::setInstance(nullptr); }

	UI::HomeView home;
	UI::AppDrawer app_drawer;
};

TEST_F(TestAppDrawer, SizeContent)
{
	app_drawer.setSize(LV_SIZE_CONTENT, LV_PCT(100));

	EXPECT_EQUAL_SCREENSHOT("app_drawer_content.png");
}

TEST_F(TestAppDrawer, FixedSize)
{
	app_drawer.setSize(500, 400);

	EXPECT_EQUAL_SCREENSHOT("app_drawer_fixed.png");
}

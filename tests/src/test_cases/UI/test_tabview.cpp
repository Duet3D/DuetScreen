/*
 * test_tabview.cpp
 *
 *  Created on: 2026-02-13
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Widgets/Layout/TabView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestTabview : public UiTestSuite
{
  public:
	TestTabview()
	{
		lv_style_set_bg_color(bg_red, lv_palette_main(LV_PALETTE_RED));
		lv_style_set_bg_opa(bg_red, LV_OPA_COVER);

		lv_style_set_bg_color(bg_green, lv_palette_main(LV_PALETTE_GREEN));
		lv_style_set_bg_opa(bg_green, LV_OPA_COVER);

		lv_style_set_bg_color(bg_blue, lv_palette_main(LV_PALETTE_BLUE));
		lv_style_set_bg_opa(bg_blue, LV_OPA_COVER);
	}

	Themes::Style bg_red;
	Themes::Style bg_green;
	Themes::Style bg_blue;
};

// ─── Construction ───────────────────────────────────────────────────────────

TEST_F(TestTabview, Empty)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));

	EXPECT_EQ(tv.getTabCount(), 0u);
	// m_currentTabIndex is initialised to -1 (SIZE_MAX), so no active tab
	EXPECT_EQ(tv.getActiveTabIndex(), static_cast<size_t>(-1));

	EXPECT_EQUAL_SCREENSHOT("tabview/empty.png");
}

// ─── Adding Tabs ────────────────────────────────────────────────────────────

TEST_F(TestTabview, AddSingleTab)
{
	ZoneScoped;
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab = tv.addTab("Tab 1");

	tab.addStyle(bg_red);
	{
		ZoneScopedN("Assertions");
		EXPECT_EQ(tv.getTabCount(), 1u);
		// First tab should be auto-activated
		EXPECT_EQ(tv.getActiveTabIndex(), 0u);
		EXPECT_TRUE(tab.isVisible());

		EXPECT_EQUAL_SCREENSHOT("tabview/single_tab.png");
	}
}

TEST_F(TestTabview, AddMultipleTabs)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("First");
	LvContainer& tab1 = tv.addTab("Second");
	LvContainer& tab2 = tv.addTab("Third");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQ(tv.getTabCount(), 3u);
	// First tab stays active
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);
	EXPECT_TRUE(tab0.isVisible());
	EXPECT_FALSE(tab1.isVisible());
	EXPECT_FALSE(tab2.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/multiple_tabs.png");
}

TEST_F(TestTabview, LongTabNames)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("First");
	LvContainer& tab1 = tv.addTab("Second tab with very long name that probably overflows");
	LvContainer& tab2 = tv.addTab("Third");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQ(tv.getTabCount(), 3u);
	// First tab stays active
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);
	EXPECT_TRUE(tab0.isVisible());
	EXPECT_FALSE(tab1.isVisible());
	EXPECT_FALSE(tab2.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/long_tab_names.png");
}

// ─── Tab switching ──────────────────────────────────────────────────────────

TEST_F(TestTabview, SetActiveTab)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("A");
	LvContainer& tab1 = tv.addTab("B");
	LvContainer& tab2 = tv.addTab("C");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	tv.setActiveTab(1);

	EXPECT_EQ(tv.getActiveTabIndex(), 1u);
	EXPECT_FALSE(tab0.isVisible());
	EXPECT_TRUE(tab1.isVisible());
	EXPECT_FALSE(tab2.isVisible());

	tv.setActiveTab(2);

	EXPECT_EQ(tv.getActiveTabIndex(), 2u);
	EXPECT_FALSE(tab0.isVisible());
	EXPECT_FALSE(tab1.isVisible());
	EXPECT_TRUE(tab2.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/set_active_last.png");
}

TEST_F(TestTabview, SetActiveTabSameIndexIsNoop)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");
	tv.addTab("B");

	tv.setActiveTab(0);
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);

	// Calling again with same index should be a no-op
	tv.setActiveTab(0);
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);
}

TEST_F(TestTabview, SetActiveTabOutOfRange)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("Only");

	// Out-of-range index keeps current tab active
	tv.setActiveTab(5);
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);
}

TEST_F(TestTabview, TabButtonCheckedState)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");
	tv.addTab("B");
	tv.addTab("C");

	// Initially tab 0 is active
	EXPECT_TRUE(tv.getTabButton(0)->hasState(LV_STATE_CHECKED));
	EXPECT_FALSE(tv.getTabButton(1)->hasState(LV_STATE_CHECKED));
	EXPECT_FALSE(tv.getTabButton(2)->hasState(LV_STATE_CHECKED));

	tv.setActiveTab(2);

	EXPECT_FALSE(tv.getTabButton(0)->hasState(LV_STATE_CHECKED));
	EXPECT_FALSE(tv.getTabButton(1)->hasState(LV_STATE_CHECKED));
	EXPECT_TRUE(tv.getTabButton(2)->hasState(LV_STATE_CHECKED));
}

// ─── getActiveTab ───────────────────────────────────────────────────────────

TEST_F(TestTabview, GetActiveTabReturnsCorrectContainer)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("A");
	LvContainer& tab1 = tv.addTab("B");

	EXPECT_EQ(&tv.getActiveTab(), &tab0);

	tv.setActiveTab(1);
	EXPECT_EQ(&tv.getActiveTab(), &tab1);
}

// ─── getTab / getTabButton ──────────────────────────────────────────────────

TEST_F(TestTabview, GetTabByIndex)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("A");
	LvContainer& tab1 = tv.addTab("B");

	EXPECT_EQ(tv.getTab(0), &tab0);
	EXPECT_EQ(tv.getTab(1), &tab1);
	EXPECT_EQ(tv.getTab(2), nullptr);
}

TEST_F(TestTabview, GetTabButtonByIndex)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("First");
	tv.addTab("Second");

	TabView::TabButton* btn0 = tv.getTabButton(0);
	TabView::TabButton* btn1 = tv.getTabButton(1);

	EXPECT_NE(btn0, nullptr);
	EXPECT_NE(btn1, nullptr);

	EXPECT_EQ(btn0->getText(), "First");
	EXPECT_EQ(btn1->getText(), "Second");
}

TEST_F(TestTabview, GetTabButtonOutOfRange)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("Only");

	EXPECT_EQ(tv.getTabButton(5), nullptr);
}

// ─── renameTab ──────────────────────────────────────────────────────────────

TEST_F(TestTabview, RenameTab)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	auto& tab = tv.addTab("Old Name");

	tab.addStyle(bg_red);

	EXPECT_TRUE(tv.renameTab(0, "New Name"));
	EXPECT_EQ(tv.getTabButton(0)->getText(), "New Name");

	EXPECT_EQUAL_SCREENSHOT("tabview/renamed_tab.png");
}

TEST_F(TestTabview, RenameTabOutOfRange)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("Tab");

	EXPECT_FALSE(tv.renameTab(5, "Nope"));
}

// ─── disableTab ─────────────────────────────────────────────────────────────

TEST_F(TestTabview, DisableInactiveTab)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	auto& tab0 = tv.addTab("A");
	auto& tab1 = tv.addTab("B");
	auto& tab2 = tv.addTab("C");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_TRUE(tv.disableTab(1, true));
	EXPECT_TRUE(tv.getTabButton(1)->hasState(LV_STATE_DISABLED));
	// Active tab unchanged
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);

	EXPECT_EQUAL_SCREENSHOT("tabview/disabled_inactive_tab.png");
}

TEST_F(TestTabview, DisableActiveTabSwitchesToNext)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("A");
	LvContainer& tab1 = tv.addTab("B");
	LvContainer& tab2 = tv.addTab("C");

	tv.setActiveTab(0);
	EXPECT_TRUE(tv.disableTab(0, true));

	// Should switch to the next available tab (index 1)
	EXPECT_EQ(tv.getActiveTabIndex(), 1u);
	EXPECT_FALSE(tab0.isVisible());
	EXPECT_TRUE(tab1.isVisible());
	EXPECT_FALSE(tab2.isVisible());
}

TEST_F(TestTabview, DisableActiveTabWrapsAround)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");
	tv.addTab("B");
	tv.addTab("C");

	tv.setActiveTab(2);
	EXPECT_TRUE(tv.disableTab(2, true));

	// Should wrap and switch to tab 0
	EXPECT_EQ(tv.getActiveTabIndex(), 0u);
}

TEST_F(TestTabview, DisableAllTabs)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	LvContainer& tab0 = tv.addTab("A");
	LvContainer& tab1 = tv.addTab("B");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);

	EXPECT_TRUE(tv.disableTab(1, true));
	EXPECT_TRUE(tv.disableTab(0, true));

	// No active tab when all are disabled
	EXPECT_EQ(tv.getActiveTabIndex(), static_cast<size_t>(-1));
	EXPECT_FALSE(tab0.isVisible());
	EXPECT_FALSE(tab1.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/all_disabled.png");
}

TEST_F(TestTabview, ReenableTab)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");
	tv.addTab("B");

	EXPECT_TRUE(tv.disableTab(1, true));
	EXPECT_TRUE(tv.getTabButton(1)->hasState(LV_STATE_DISABLED));

	EXPECT_TRUE(tv.disableTab(1, false));
	EXPECT_FALSE(tv.getTabButton(1)->hasState(LV_STATE_DISABLED));
}

TEST_F(TestTabview, DisableAlreadyDisabledIsIdempotent)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");

	EXPECT_TRUE(tv.disableTab(0, true));
	// Second call should return true (already in desired state)
	EXPECT_TRUE(tv.disableTab(0, true));
}

TEST_F(TestTabview, EnableAlreadyEnabledIsIdempotent)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");

	// Tab starts enabled; enabling again should succeed
	EXPECT_TRUE(tv.disableTab(0, false));
}

TEST_F(TestTabview, DisableTabOutOfRange)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");

	EXPECT_FALSE(tv.disableTab(5, true));
}

// ─── Tab bar position ───────────────────────────────────────────────────────

TEST_F(TestTabview, TabBarTop)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.setTabBarPosition(LV_DIR_TOP);
	auto& tab0 = tv.addTab("Tab 1");
	auto& tab1 = tv.addTab("Tab 2");
	auto& tab2 = tv.addTab("Tab 3");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQUAL_SCREENSHOT("tabview/bar_top.png");
}

TEST_F(TestTabview, TabBarBottom)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.setTabBarPosition(LV_DIR_BOTTOM);
	auto& tab0 = tv.addTab("Tab 1");
	auto& tab1 = tv.addTab("Tab 2");
	auto& tab2 = tv.addTab("Tab 3");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQUAL_SCREENSHOT("tabview/bar_bottom.png");
}

TEST_F(TestTabview, TabBarLeft)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.setTabBarPosition(LV_DIR_LEFT);
	auto& tab0 = tv.addTab("Tab 1");
	auto& tab1 = tv.addTab("Tab 2 - long name");
	auto& tab2 = tv.addTab("Tab 3");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQUAL_SCREENSHOT("tabview/bar_left.png");
}

TEST_F(TestTabview, TabBarRight)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.setTabBarPosition(LV_DIR_RIGHT);
	auto& tab0 = tv.addTab("Tab 1");
	auto& tab1 = tv.addTab("Tab 2");
	auto& tab2 = tv.addTab("Tab 3");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);
	tab2.addStyle(bg_blue);

	EXPECT_EQUAL_SCREENSHOT("tabview/bar_right.png");
}

// ─── Tab content ────────────────────────────────────────────────────────────

TEST_F(TestTabview, TabContentIsolation)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));

	LvContainer& tab0 = tv.addTab("Panel A");
	LvContainer& tab1 = tv.addTab("Panel B");

	tab0.addStyle(bg_red);
	tab1.addStyle(bg_green);

	Button btn_a("btn_a", tab0, "Button in A");
	Button btn_b("btn_b", tab1, "Button in B");

	// Only tab0 content should be visible
	tv.setActiveTab(0);
	EXPECT_TRUE(tab0.isVisible());
	EXPECT_FALSE(tab1.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/content_tab_a.png");

	tv.setActiveTab(1);
	EXPECT_FALSE(tab0.isVisible());
	EXPECT_TRUE(tab1.isVisible());

	EXPECT_EQUAL_SCREENSHOT("tabview/content_tab_b.png");
}

// ─── Switching between many tabs ────────────────────────────────────────────

TEST_F(TestTabview, CycleThroughTabs)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	constexpr size_t count = 5;
	for (size_t i = 0; i < count; i++)
	{
		tv.addTab(fmt::format("Tab {}", i));
	}
	EXPECT_EQ(tv.getTabCount(), count);

	for (size_t i = 0; i < count; i++)
	{
		tv.setActiveTab(i);
		EXPECT_EQ(tv.getActiveTabIndex(), i);
		EXPECT_TRUE(tv.getTab(i)->isVisible());

		// All other tabs hidden
		for (size_t j = 0; j < count; j++)
		{
			if (j != i)
			{
				EXPECT_FALSE(tv.getTab(j)->isVisible());
			}
		}
	}
}

// ─── Disable active tab with adjacent tabs also disabled ────────────────────

TEST_F(TestTabview, DisableActiveSkipsDisabledNeighbors)
{
	TabView tv("tv", screen);
	tv.setSize(LV_PCT(100), LV_PCT(100));
	tv.addTab("A");
	tv.addTab("B");
	tv.addTab("C");
	tv.addTab("D");

	tv.setActiveTab(1);

	// Disable tab 2 (the next one)
	EXPECT_TRUE(tv.disableTab(2, true));

	// Now disable the active tab 1 — should skip disabled tab 2 and land on 3
	EXPECT_TRUE(tv.disableTab(1, true));
	EXPECT_EQ(tv.getActiveTabIndex(), 3u);
}

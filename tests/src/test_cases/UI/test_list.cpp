/*
 * test_list.cpp
 *
 *  Created on: 2025-08-16
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/List/List.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestList : public UiTestSuite
{
  public:
	TestList() {}
};

TEST_F(TestList, VerticalWrap)
{
	UI::List<UI::LvContainer> list("list", screen);
	list.addStyle(UI::Themes::getLvglStyles().border);
	list.getListContainer().addStyle(UI::Themes::getLvglStyles().border);
	list.setSize(LV_PCT(100), LV_PCT(100));
	list.setListGrow(1);
	list.setListFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	// list.setListSize(LV_PCT(100), 400);

	list.setTitle("list vertical wrap");
	list.setItemCount(10,
					  [](size_t index, UI::LvObj& parent)
					  {
						  auto item = std::make_shared<UI::LvContainer>(fmt::format("", index), parent);
						  item->setSize(100, 100);
						  item->addStyle(UI::Themes::getLvglStyles().bg_color_primary);
						  return item;
					  });

	EXPECT_EQUAL_SCREENSHOT("list_vertical_wrap.png");
}
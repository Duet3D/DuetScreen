/*
 * test_button.cpp
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/Buttons/Button.h"
#include "UI/Widgets/Layout/List.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestButton : public UiTestSuite
{
};

TEST_F(TestButton, Basic)
{
	UI::Button btn("btn", screen);
	EXPECT_EQUAL_SCREENSHOT("button/basic.png");
}

TEST_F(TestButton, WithText)
{
	UI::Button btn("btn", screen);
	btn.setText("Click Me");
	EXPECT_EQUAL_SCREENSHOT("button/with_text.png");
}

TEST_F(TestButton, Symbol)
{
	UI::Button btn("btn", screen);
	btn.setText(LV_SYMBOL_PLUS);
	btn.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("button/with_symbol.png");
}

TEST_F(TestButton, LabelSize)
{
	UI::Button btn("btn", screen);
	btn.setWidth(100);
	btn.setText("Wrap should this text");
	EXPECT_EQUAL_SCREENSHOT("button/label_size.png");
}

TEST_F(TestButton, LongText)
{
	UI::LvContainer cont("cont", screen);
	cont.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	cont.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);

	// Btn not given any size. Text does not wrap
	UI::Button btn("btn", cont);
	btn.setText("This is a long text that might overflow");

	UI::Button btn2("btn2", cont);
	btn2.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	btn2.setText("Not overflow should this text");

	UI::Button btn3("btn3", cont);
	btn3.setSize(100, 50);
	btn3.setText("Truncate should this text");

	UI::Button btn4("btn4", cont);
	btn4.setWidth(100);
	btn4.setText("Wrap should this text - some extra text to make it wrap");

	UI::List<UI::Button> button_ver_list("button_ver_list", cont);
	button_ver_list.setSize(100, LV_SIZE_CONTENT);
	button_ver_list.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	button_ver_list.setItemCount(5,
								 [](size_t index, UI::LvObj& parent)
								 {
									 auto btn =
										 std::make_unique<UI::Button>(fmt::format("button_ver_list_{}", index), parent);
									 btn->setText("Truncate should this text");
									 btn->setFlexGrow(1);
									 btn->setSize(LV_PCT(100), LV_SIZE_CONTENT);
									 return btn;
								 });

	UI::List<UI::Button> button_ver_list_no_grow("button_ver_list_no_grow", cont);
	button_ver_list_no_grow.setSize(100, LV_SIZE_CONTENT);
	button_ver_list_no_grow.setItemCount(2,
										 [](size_t index, UI::LvObj& parent)
										 {
											 auto btn = std::make_unique<UI::Button>(
												 fmt::format("button_ver_list_no_grow_{}", index), parent);
											 btn->setText("Wrap should this text");
											 btn->setSize(LV_PCT(100), LV_SIZE_CONTENT);
											 return btn;
										 });

	UI::List<UI::Button> button_hor_list("button_hor_list", cont);
	button_hor_list.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	button_hor_list.setSize(300, LV_SIZE_CONTENT);
	button_hor_list.setListFlow(LV_FLEX_FLOW_ROW);
	button_hor_list.setItemCount(5,
								 [](size_t index, UI::LvObj& parent)
								 {
									 auto btn =
										 std::make_unique<UI::Button>(fmt::format("button_hor_list_{}", index), parent);
									 btn->setText("Wrap should this text"); // since the list height is LV_SIZE_CONTENT
									 btn->setFlexGrow(1);
									 btn->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
									 return btn;
								 });

	UI::List<UI::Button> button_hor_list_fixed_height("button_hor_list_fixed_height", cont);
	button_hor_list_fixed_height.setSize(300, 100);
	button_hor_list_fixed_height.setListFlow(LV_FLEX_FLOW_ROW);
	button_hor_list_fixed_height.setListGrow(1);
	button_hor_list_fixed_height.setItemCount(
		5,
		[](size_t index, UI::LvObj& parent)
		{
			auto btn = std::make_unique<UI::Button>(fmt::format("button_hor_list_fixed_height_{}", index), parent);
			btn->setText("Truncate should this text"); // Since list height is smaller
													   // than required label height
			btn->setHeight(LV_PCT(100));
			btn->setFlexGrow(1);
			return btn;
		});

	EXPECT_EQUAL_SCREENSHOT("button/long_text.png");
}

TEST_F(TestButton, SetIconBmp)
{
	UI::Button btn("btn", screen);
	btn.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.bmp"));
	btn.getIcon().enableRecolor(false);
	EXPECT_EQUAL_SCREENSHOT("button/with_bmp.png");
}

TEST_F(TestButton, SetIconPng)
{
	UI::Button btn("btn", screen);
	btn.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	EXPECT_EQUAL_SCREENSHOT("button/with_png.png");
}

TEST_F(TestButton, IconScaling)
{
	UI::LvContainer cont("cont", screen);
	cont.setSize(LV_PCT(100), LV_PCT(100));
	cont.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);

	UI::Button btn1("btn1", cont);
	btn1.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));

	UI::Button btn2("btn2", cont);
	btn2.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn2.setWidth(200);

	UI::Button btn3("btn3", cont);
	btn3.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn3.setHeight(200);

	UI::Button btn4("btn4", cont);
	btn4.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn4.setSize(200, 200);

	UI::Button btn5("btn5", cont, "Button 5");
	btn5.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	btn5.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));

	UI::Button btn6("btn6", cont, "Button 6");
	btn6.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn6.setWidth(200);

	UI::Button btn7("btn7", cont, "Button 7");
	btn7.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn7.setHeight(200);

	UI::Button btn8("btn8", cont, "Button 8");
	btn8.setFixedIcon(UI::Themes::getFixedIconPath("examples", "example.png"));
	btn8.setSize(200, 200);

	EXPECT_EQUAL_SCREENSHOT("button/icon_scaling.png");
}

TEST_F(TestButton, SetIconBadPath)
{
	UI::Button btn("btn", screen);
	btn.setIcon("bad_path.bmp");
	EXPECT_EQ(btn.getIcon().getSrc(), nullptr);
	EXPECT_EQUAL_SCREENSHOT("button/with_bad_path_icon.png");
}
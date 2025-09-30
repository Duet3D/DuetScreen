/*
 * test_messagebox.cpp
 *
 *  Created on: 2025-09-02
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

static std::string_view body_text =
	R"(This is a long example body text intended to test word wrapping, sizing, scrolling and layout behavior inside the MessageBox component. It contains punctuation, numbers (1234567890), and varying line lengths. The quick brown fox jumps over the lazy dog. Multiple sentences should ensure proper wrapping at different widths. End of sample.)";

class TestMessagebox : public UiTestSuite
{
  public:
	TestMessagebox() {}

	MessageBox message_box{"message_box", screen, layout_t(0, 0, 50, 50)};
};

TEST_F(TestMessagebox, Basic)
{
	message_box.setTitle("title");
	message_box.setText(body_text);
	EXPECT_EQUAL_SCREENSHOT("message_box_basic.png");
}

TEST_F(TestMessagebox, WithImage)
{
	message_box.setTitle("title");
}

TEST_F(TestMessagebox, HeightSizeContent)
{
	message_box.setTitle("title");
	message_box.setText(body_text);
	message_box.setHeight(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("message_box_height_size_content.png");
}

TEST_F(TestMessagebox, WidthSizeContent)
{
	message_box.setTitle("title");
	message_box.setText(body_text);
	message_box.setWidth(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("message_box_width_size_content.png");
}
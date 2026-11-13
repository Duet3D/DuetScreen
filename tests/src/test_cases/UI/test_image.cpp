/*
 * test_image.cpp
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Lvgl/LvImage.h"
#include "UI/Styles/Styles.h"
#include "test_utils/UiTestSuite.h"
#include <fstream>
#include <gtest/gtest.h>

class TestImage : public UiTestSuite
{
  public:
	TestImage()
		: img("image", screen)
	{
		screen.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
	}

	UI::LvImage img;
};

TEST_F(TestImage, Bmp)
{
	img.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img.setSrc(UI::Themes::getFixedIconPath("examples", "example.bmp").c_str());
	EXPECT_EQUAL_SCREENSHOT("image/bmp.png");
}

TEST_F(TestImage, Png)
{
	img.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img.setSrc(UI::Themes::getFixedIconPath("examples", "example_full_color.png").c_str());
	EXPECT_EQUAL_SCREENSHOT("image/png.png");
}

TEST_F(TestImage, PngRecolor)
{
	screen.setStyleBgColor(lv_palette_main(LV_PALETTE_GREY), 0);
	screen.setStyleBgOpa(LV_OPA_COVER, 0);

	img.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img.setSrc(UI::Themes::getFixedIconPath("examples", "example.png").c_str());

	UI::LvImage img_white("recolor_white", screen);
	img_white.setSrc(UI::Themes::getFixedIconPath("examples", "example.png").c_str());
	img_white.setStyleRecolor(lv_color_white(), 0);
	img_white.setStyleRecolorOpa(LV_OPA_COVER, 0);

	UI::LvImage img_blue("recolor_blue", screen);
	img_blue.setSrc(UI::Themes::getFixedIconPath("examples", "example.png").c_str());
	img_blue.setStyleRecolor(lv_palette_main(LV_PALETTE_BLUE), 0);
	img_blue.setStyleRecolorOpa(LV_OPA_COVER, 0);

	EXPECT_EQUAL_SCREENSHOT("image/png_recolor.png");
}

#if LV_USE_SVG
TEST_F(TestImage, Svg)
{
	img.setSize(100, 100);
	img.setSrc(UI::Themes::getFixedIconPath("examples", "example.svg").c_str());
	img.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
	img.addStyle(UI::Themes::getLvglStyles().bg_light);
	// lv_obj_set_style_image_recolor(img, lv_color_white(), 0);
	// lv_obj_set_style_image_opa(img, LV_OPA_COVER, 0);
	// lv_obj_set_style_recolor(img, lv_color_white(), 0);
	// lv_obj_set_style_recolor_opa(img, LV_OPA_COVER, 0);
	EXPECT_EQUAL_SCREENSHOT("image/svg.png");
}
#endif

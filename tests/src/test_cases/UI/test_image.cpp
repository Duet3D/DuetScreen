/*
 * test_image.cpp
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/LVGL/LvImage.h"
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
	img.setSrc(IMAGE_ASSET("examples/example.bmp"));
	EXPECT_EQUAL_SCREENSHOT("image_bmp.png");
}

TEST_F(TestImage, Png)
{
	img.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img.setSrc(IMAGE_ASSET("examples/example_full_color.png"));
	EXPECT_EQUAL_SCREENSHOT("image_png.png");
}

TEST_F(TestImage, PngRecolor)
{
	lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREY), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

	img.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img.setSrc(IMAGE_ASSET("examples/example.png"));

	UI::LvImage img_white("recolor_white", screen);
	img_white.setSrc(IMAGE_ASSET("examples/example.png"));
	img_white.setStyleRecolor(lv_color_white(), 0);
	img_white.setStyleRecolorOpa(LV_OPA_COVER, 0);

	UI::LvImage img_blue("recolor_blue", screen);
	img_blue.setSrc(IMAGE_ASSET("examples/example.png"));
	img_blue.setStyleRecolor(lv_palette_main(LV_PALETTE_BLUE), 0);
	img_blue.setStyleRecolorOpa(LV_OPA_COVER, 0);

	EXPECT_EQUAL_SCREENSHOT("image_png_recolor.png");
}

#if LV_USE_SVG
TEST_F(TestImage, Svg)
{
	img.setSize(100, 100);
	img.setSrc(IMAGE_ASSET("examples/example.svg"));
	img.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
	img.addStyle(UI::Themes::getLvglStyles().bg_light);
	// lv_obj_set_style_image_recolor(img, lv_color_white(), 0);
	// lv_obj_set_style_image_opa(img, LV_OPA_COVER, 0);
	// lv_obj_set_style_recolor(img, lv_color_white(), 0);
	// lv_obj_set_style_recolor_opa(img, LV_OPA_COVER, 0);
	EXPECT_EQUAL_SCREENSHOT("image_svg.png");
}
#endif

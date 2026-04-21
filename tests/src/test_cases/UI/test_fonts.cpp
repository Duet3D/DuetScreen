/*
 * test_fonts.cpp
 *
 *  Created on: 2025-10-17
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Styles/Font.h"
#include "test_utils/UiTestSuite.h"
#include <filesystem>
#include <gtest/gtest.h>

using namespace UI;

class TestFonts : public UiTestSuite
{
  public:
	TestFonts() {}
};

TEST_F(TestFonts, FontManager)
{
	FontManager::Font font1 = FontManager::createFont("OpenSans", 14, LV_FREETYPE_FONT_STYLE_NORMAL);
	ASSERT_NE(font1.get(), LV_FONT_DEFAULT);

	ASSERT_EQ(FontManager::createFont("bad_font_name", 14).get(), LV_FONT_DEFAULT);
	ASSERT_EQ(FontManager::createFont("", 14).get(), LV_FONT_DEFAULT);

	FontManager::Font fallback_font = FontManager::createFont("bad_font_name,OpenSans", 14);
	ASSERT_NE(fallback_font.get(), LV_FONT_DEFAULT);
}

TEST_F(TestFonts, FontStyle)
{
	struct LabelDsc
	{
		std::string_view description;
		LvLabel label;
		lv_font_t font;
		struct
		{
			uint32_t size;
			uint32_t style;
		} fontConfig;
	};

	std::array label_dscs{
		LabelDsc{.description = "regular",
				 .label = LvLabel{"label_regular", screen},
				 .fontConfig = {14, LV_FREETYPE_FONT_STYLE_NORMAL}},
		LabelDsc{.description = "bold",
				 .label = LvLabel{"label_bold", screen},
				 .fontConfig = {14, LV_FREETYPE_FONT_STYLE_BOLD}},
		LabelDsc{.description = "thin",
				 .label = LvLabel{"label_thin", screen},
				 .fontConfig = {14, LV_FREETYPE_FONT_STYLE_WEIGHT(200)}},
		LabelDsc{.description = "italic",
				 .label = LvLabel{"label_italic", screen},
				 .fontConfig = {14, LV_FREETYPE_FONT_STYLE_ITALIC}},
		LabelDsc{.description = "boldItalic",
				 .label = LvLabel{"label_boldItalic", screen},
				 .fontConfig = {14, LV_FREETYPE_FONT_STYLE_BOLD | LV_FREETYPE_FONT_STYLE_ITALIC}},
		LabelDsc{.description = "large",
				 .label = LvLabel{"label_large", screen},
				 .fontConfig = {24, LV_FREETYPE_FONT_STYLE_NORMAL}},
	};

	for (auto& dsc : label_dscs)
	{
		dsc.label.setText(fmt::format("{:s}: The quick brown fox jumps over the lazy dog", dsc.description));
		dsc.label.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		dsc.label.setStyleFont(&dsc.font);
	}

	const auto& loadedFonts = FontManager::getLoadedFontNames();
	for (const auto& fontName : loadedFonts)
	{
		for (auto& dsc : label_dscs)
		{
			dsc.font = *FontManager::createFont(fontName, dsc.fontConfig.size, dsc.fontConfig.style).get();
			lv_obj_report_style_change(NULL);
		}
		EXPECT_EQUAL_SCREENSHOT(fmt::format("fonts/{:s}.png", fontName).c_str());
	}
}
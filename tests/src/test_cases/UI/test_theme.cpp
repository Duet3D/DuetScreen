/*
 * test_theme.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "UI/Components/Input/Slider.h"
#include "UI/Styles/Theme.h"
#include "UI/Styles/ThemeFile.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <fstream>
#include <gtest/gtest.h>

using namespace UI;

class TestTheme : public UiTestSuite
{
  public:
	TestTheme() {}

	~TestTheme()
	{
		Themes::Theme* default_theme = Themes::getThemeByName(StorageHelper::getData(ID_THEME));
		if (default_theme == nullptr)
		{
			return;
		}
		default_theme->setThemeActive();
	}
};

TEST_F(TestTheme, DefaultTheme)
{
	constexpr uint16_t primaryHue = 210;
	constexpr uint16_t secondaryHue = 50;
	constexpr float chroma = 0.02f;
	constexpr bool darkMode = true;

	// A customisable theme with an all-zero light palette and fonts without weights
	std::string error;
	auto file = Themes::parseThemeFile(R"(
		<theme name="test_theme" extends="base">
			<palette primary_hue="0" secondary_hue="0" chroma="0" dark="false"/>
			<fonts>
				<font name="header" size="18" weight="none" style="bold"/>
				<font name="normal" size="14" weight="none" style="normal"/>
				<font name="emphasis" size="14" weight="none" style="bold"/>
				<font name="subdued" size="12" weight="none" style="normal"/>
			</fonts>
		</theme>)",
									   "test_theme.xml",
									   error);
	ASSERT_TRUE(file.has_value()) << error;
	ASSERT_EQ(file->fonts["header"].weight, 0u) << "none is weight 0: the face's own weight";

	// Load it alongside the built-in themes
	std::vector<Themes::ThemeFile> files;
	std::string text;
	for (const char* name : {"base", "duetscreen"})
	{
		std::ifstream in(fmt::format("assets/ui/themes/{:s}.xml", name));
		text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		files.push_back(*Themes::parseThemeFile(text, name, error));
	}
	files.push_back(std::move(*file));
	Themes::setThemeFiles(std::move(files), nullptr);

	Themes::Theme* theme = Themes::getThemeByName("test_theme");
	ASSERT_NE(theme, nullptr);
	ASSERT_TRUE(theme->isCustomisable());
	theme->resetPalette();
	theme->setThemeActive();

	Xml::XmlView themePreview("theme_preview", screen, "theme_preview");
	themePreview.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	themePreview.show();
	EXPECT_TRUE(Themes::getCurrentTheme() == theme);

	// Changed as the user would: the sliders keep the values within their ranges
	auto set = [&](const char* name, float value)
	{
		auto& slider = static_cast<Slider&>(*LvObj::fromPtr(lv_obj_find_by_name(themePreview.getRootPtr(), name)));
		slider.setValue(value);
	};
	auto setDark = [&](bool dark)
	{
		lv_obj_t* toggle = lv_obj_find_by_name(themePreview.getRootPtr(), "dark_mode");
		lv_obj_set_state(toggle, LV_STATE_CHECKED, dark);
		lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	};
	auto change = [&](bool dark, uint16_t primary, uint16_t secondary, float chromaValue)
	{
		setDark(dark);
		set("primary_hue_slider", primary);
		set("secondary_hue_slider", secondary);
		set("chroma_slider", chromaValue);
	};

	change(darkMode, primaryHue, secondaryHue, chroma);
	EXPECT_EQUAL_SCREENSHOT("theme/example_1.png");

	change(false, 25, 150, 0.5f);
	EXPECT_EQUAL_SCREENSHOT("theme/example_2.png");

	change(true, 300, 50, 0.5f);
	EXPECT_EQUAL_SCREENSHOT("theme/example_3.png");
}

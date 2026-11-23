/*
 * test_diacritic.cpp
 *
 * Created on: 2026-02-23
 * Author: automated-test
 */

#include "Debug.h"
#include "UI/Lvgl/LvLabel.h"
#include "test_utils/UiTestSuite.h"
#include <cstring>
#include <gtest/gtest.h>
#include <string>

using namespace UI;

class TestDiacritic : public UiTestSuite
{
  public:
	TestDiacritic() {}
};

TEST_F(TestDiacritic, CombinedVsCombining)
{
	// Create two labels on the test screen
	LvLabel label_precomposed("label_pre", screen);
	LvLabel label_decomposed("label_decomp", screen);

	// Examples: pairs of (precomposed, decomposed)
	// This list covers common diacritics used across European languages.
	struct Pair
	{
		const char* pre;
		const char* decomp;
		const char* desc;
	} pairs[] = {
		{"\u00E1", "a\u0301", "a + acute -> á"}, // á
		{"\u00E0", "a\u0300", "a + grave -> à"}, // à
		{"\u00E2", "a\u0302", "a + circumflex -> â"},
		{"\u00E3", "a\u0303", "a + tilde -> ã"},
		{"\u00E4", "a\u0308", "a + diaeresis -> ä"},
		{"\u00E5", "a\u030A", "a + ring -> å"},
		{"\u0105", "a\u0328", "a + ogonek -> ą"},

		{"\u00E9", "e\u0301", "e + acute -> é"}, // é
		{"\u00E8", "e\u0300", "e + grave -> è"}, // è
		{"\u00EA", "e\u0302", "e + circumflex -> ê"},
		{"\u00EB", "e\u0308", "e + diaeresis -> ë"},
		{"\u0119", "e\u0328", "e + ogonek -> ę"},
		{"\u011B", "e\u030C", "e + caron -> ě"},

		{"\u00ED", "i\u0301", "i + acute -> í"},
		{"\u00EC", "i\u0300", "i + grave -> ì"},
		{"\u00EE", "i\u0302", "i + circumflex -> î"},
		{"\u00EF", "i\u0308", "i + diaeresis -> ï"},
		{"\u012B", "i\u0304", "i + macron -> ī"},

		{"\u00F3", "o\u0301", "o + acute -> ó"},
		{"\u00F2", "o\u0300", "o + grave -> ò"},
		{"\u00F4", "o\u0302", "o + circumflex -> ô"},
		{"\u00F5", "o\u0303", "o + tilde -> õ"},
		{"\u00F6", "o\u0308", "o + diaeresis -> ö"},
		{"\u00F8", "o\u0338", "o + stroke (approx) -> ø"},

		{"\u00F1", "n\u0303", "n + tilde -> ñ"},
		{"\u0144", "n\u0301", "n + acute -> ń"},

		{"\u00E7", "c\u0327", "c + cedilla -> ç"},
		{"\u0107", "c\u0301", "c + acute -> ć"},
		{"\u010D", "c\u030C", "c + caron -> č"},

		{"\u015B", "s\u0301", "s + acute -> ś"},
		{"\u0161", "s\u030C", "s + caron -> š"},

		{"\u017A", "z\u0301", "z + acute -> ź"},
		{"\u017C", "z\u0307", "z + dot -> ż"},
		{"\u017E", "z\u030C", "z + caron -> ž"},

		{"\u0142", "l\u0335", "l + stroke (approx) -> ł"},
		{"\u0159", "r\u030C", "r + caron -> ř"},
		{"\u0165", "t\u030C", "t + caron -> ť"},

		{"\u0117", "e\u0307", "e + dot -> ė"},
		{"\u012F", "i\u0328", "i + ogonek -> į"},
		{"\u0173", "u\u0328", "u + ogonek -> ų"},

		{"\u00FC", "u\u0308", "u + diaeresis -> ü"},
		{"\u016B", "u\u0304", "u + macron -> ū"},
	};

	// Create labels for each pair and verify the stored text matches what we set
	for (size_t i = 0; i < std::size(pairs); ++i)
	{
		std::string name_pre = std::string("pre_") + std::to_string(i);
		std::string name_de = std::string("de_") + std::to_string(i);

		LvLabel lp(name_pre, screen);
		LvLabel ld(name_de, screen);

		lp.setText(pairs[i].pre);
		ld.setText(pairs[i].decomp);

		ASSERT_STREQ(lp.getText(), pairs[i].pre) << "precomposed failed for " << pairs[i].desc;
		ASSERT_STREQ(ld.getText(), pairs[i].decomp) << "decomposed failed for " << pairs[i].desc;

		// Precomposed and decomposed byte lengths will commonly differ
		EXPECT_NE(std::strlen(lp.getText()), std::strlen(ld.getText())) << pairs[i].desc;
	}

	// Compose all pairs into single strings for a combined display on the two top-level labels
	std::string combined_pre;
	std::string combined_decomp;
	for (size_t i = 0; i < std::size(pairs); ++i)
	{
		if (!combined_pre.empty())
		{
			combined_pre += " ";
			combined_decomp += " ";
		}
		combined_pre += pairs[i].pre;
		combined_decomp += pairs[i].decomp;
	}

	// Use the labels created at the top of the test to show the combined strings
	label_precomposed.setText(combined_pre);
	label_decomposed.setText(combined_decomp);

	EXPECT_EQUAL_SCREENSHOT("diacritic/combined_vs_combining.png");
}

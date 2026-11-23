/*
 * test_file_browser.cpp
 *
 * The names the file browser shows (src/UI/Xml/Data/FilesSource.h). Browsing on screen, and the shared file dialog,
 * are covered by test_home_view.cpp.
 *
 *  Created on: 2026-03-12
 *      Author: Andy Everitt
 */

#include "UI/Xml/Data/FilesSource.h"
#include <gtest/gtest.h>

using UI::Xml::FilesSource;

TEST(TestFileBrowserPresenter, MacroDisplayNameRemovesNumericPrefix)
{
	EXPECT_EQ(FilesSource::shownName("12_Preheat.g", true), "Preheat.g");
	EXPECT_EQ(FilesSource::shownName("001_Home All.g", true), "Home All.g");
	EXPECT_EQ(FilesSource::shownName("Preheat.g", true), "Preheat.g");
	EXPECT_EQ(FilesSource::shownName("12Preheat.g", true), "12Preheat.g");
	EXPECT_EQ(FilesSource::shownName("12_Preheat.g", false), "12_Preheat.g");
}

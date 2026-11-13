/*
 * test_file_browser.cpp
 *
 * The file browser's presenter (src/UI/Screens/File/FileBrowserPresenter.h). Browsing on screen, and the dialogs, are
 * covered by test_home_view.cpp.
 *
 *  Created on: 2026-03-12
 *      Author: Andy Everitt
 */

#include "UI/Screens/File/FileBrowserPresenter.h"
#include <gtest/gtest.h>

using namespace UI;

TEST(TestFileBrowserPresenter, MacroDisplayNameRemovesNumericPrefix)
{
	using Folder = FileBrowserPresenter::BaseFolder;
	EXPECT_EQ(FileBrowserPresenter::getDisplayName("12_Preheat.g", Folder::MACROS), "Preheat.g");
	EXPECT_EQ(FileBrowserPresenter::getDisplayName("001_Home All.g", Folder::MACROS), "Home All.g");
	EXPECT_EQ(FileBrowserPresenter::getDisplayName("Preheat.g", Folder::MACROS), "Preheat.g");
	EXPECT_EQ(FileBrowserPresenter::getDisplayName("12Preheat.g", Folder::MACROS), "12Preheat.g");
	EXPECT_EQ(FileBrowserPresenter::getDisplayName("12_Preheat.g", Folder::GCODES), "12_Preheat.g");
}

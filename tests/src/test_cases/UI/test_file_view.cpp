/*
 * test_file_view.cpp
 *
 *  Created on: 2025-09-04
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Screens/File/FileView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestFileView : public UiTestSuite
{
  public:
	TestFileView() {}

	FileView file_view{screen};
};

TEST_F(TestFileView, Basic)
{
	EXPECT_EQUAL_SCREENSHOT("file_view_basic.png");
}
/*
 * UiTestSuite.h
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Screen/Screen.h"
#include "test_utils/utils.h"
#include <gtest/gtest.h>

class UiTestSuite : public ::testing::Test
{
  protected:
	UiTestSuite();

	virtual ~UiTestSuite();

	static void SetUpTestSuite();

	static void TearDownTestSuite();

	static bool load_model_data_from_file(std::string_view filename);
	static bool load_model_data(std::string_view data);

	UI::Screen screen{"test_screen"};
};

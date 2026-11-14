/*
 * TestSuite.h
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#pragma once

#include "test_utils/utils.h"
#include <filesystem>
#include <gtest/gtest.h>

class TestSuite : public ::testing::Test
{
  protected:
	TestSuite();
	virtual ~TestSuite();

	static bool load_model_data_from_file(std::string_view filename);
	static bool load_model_data(std::string_view data);

	/**
	 * The settings file the test stores its settings in: one of its own, in the system's temporary folder, so tests
	 * run side by side (`ctest -j`) don't store into each other's
	 */
	static std::filesystem::path configFile();
};

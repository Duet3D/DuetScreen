/*
 * TestSuite.h
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#pragma once

#include "test_utils/utils.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#if DUET_SIMULATOR
#  include "Simulator/Config.h"
#endif

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

#if DUET_SIMULATOR
	/**
	 * Connect to the simulated Duet as `config`, without noise on its temperatures, run by the test: nothing happens
	 * until pollSimulator() or simulate(). Every test starts disconnected from it.
	 */
	static void connectSimulator(const Sim::Config& config = Sim::defaultConfig());

	/** Poll the simulated Duet as the screen does, `rounds` times, each answered at once: every key is read */
	static void pollSimulator(int rounds = 40);

	/** Send `gcode` to the simulated Duet, as the screen does, and let it answer */
	static void sendToSimulator(std::string_view gcode);

	/** Let `time` pass for the simulated Duet, then poll it */
	static void simulate(std::chrono::milliseconds time);
#endif
};

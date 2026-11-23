/*
 * TestSuite.h
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#pragma once

#include "test_utils/ScriptedPrinter.h"
#include "test_utils/utils.h"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>
#if DUET_SIMULATOR
#  include "Simulator/Config.h"
#endif

class TestSuite : public ::testing::Test
{
  protected:
	TestSuite();
	virtual ~TestSuite();

	/**
	 * Connect to a printer the test plays (see ScriptedPrinter.h): it has answered the first live model request with
	 * nothing, so the screen is connected and keeps the object model it has
	 */
	static ScriptedPrinter& connectScriptedPrinter();

	/**
	 * Have `printer` give the file `file` a thumbnail, the example one (300 pixels square), as the screen asks for it:
	 * the file cache asks for the large thumbnail, and `printer` sends the file's information and then the thumbnail,
	 * which the cache keeps; anything the cache asked about other files first is answered as missing
	 */
	static void giveThumbnail(ScriptedPrinter& printer, const std::string& file);

	static bool load_model_data_from_file(std::string_view filename);
	static bool load_model_data(std::string_view data);

	/**
	 * The settings file the test stores its settings in: one of its own, in the system's temporary folder, so tests
	 * run side by side (`ctest -j`) don't store into each other's
	 */
	static std::filesystem::path configFile();

	/** The file cache's folders for the test: its own, in the system's temporary folder */
	static std::filesystem::path cacheFolder();

	/**
	 * Plug in USB drive `number` (0 to 7): an empty folder of the test's own, standing in for /media/usb<number>,
	 * which the USB monitor finds at once (its events are posted, for deliverModelEvents()). Every test starts with
	 * none.
	 */
	static std::filesystem::path plugInUsbDrive(int number = 0);

	/** Take out USB drive `number`, and everything on it */
	static void takeOutUsbDrive(int number = 0);

	/**
	 * Write a tar.gz archive at `path` holding `files` (name, contents), as scripts/create_upgrade.py does: a name
	 * longer than a tar header holds goes in a GNU long name entry before it
	 */
	static void writeArchive(const std::filesystem::path& path,
							 const std::vector<std::pair<std::string, std::string>>& files);

	/** How many exchanges with the simulated Duet one poll may take, at most (a Duet that doesn't answer takes all) */
	static constexpr int maxExchanges = 100;

	/** What OM::Tree::want() gave for the whole object model, while a test is connected to the simulated Duet */
	static inline size_t s_wantEverything = 0;

#if DUET_SIMULATOR
	/**
	 * Connect to the simulated Duet as `config`, without noise on its temperatures, run by the test: nothing happens
	 * until pollSimulator() or simulate(). Every test starts disconnected from it.
	 */
	static void connectSimulator(const Sim::Config& config = Sim::defaultConfig());

	/**
	 * Poll the simulated Duet as the screen does, `rounds` times, each answered at once along with every request it
	 * leads to: every key that changed is read
	 */
	static void pollSimulator(int rounds = 2);

	/** Send `gcode` to the simulated Duet, as the screen does, and let it answer */
	static void sendToSimulator(std::string_view gcode);

	/** Let `time` pass for the simulated Duet, then poll it */
	static void simulate(std::chrono::milliseconds time);
#endif
};

/*
 * TestSuite.cpp
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#include "TestSuite.h"
#include "Comm/Connection.h"
#include "Comm/FileInfoCache.h"
#include "Comm/LineFramer.h"
#include "Comm/Replies.h"
#include "Comm/UsbMode.h"
#include "Configuration.h"
#include "DeadlockDetector.h"
#include "Debug.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Followers.h"
#include "ObjectModel/Tree/Json.h"
#include "ObjectModel/Tree/Tree.h"
#include "ObjectModel/Utils.h"
#include "test_utils/utils.h"
#include "utils/StorageHelper.h"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <unistd.h>
#if DUET_SIMULATOR
#  include "Simulator/Simulator.h"
#endif

TestSuite::TestSuite()
{
	ZoneScoped;
	/* Run at start of each test */
	std::filesystem::remove(configFile());
	StorageHelper::setConfigFile(configFile().string());
	// Log::SetDebugLevel(Log::DebugLevel::Debug);
	Log::Init();

	Comm::Connection::get().setManual(true); // runs only when a test says, on its thread
	OM::Followers::start();
	Comm::usbInit(); // the USB mode the settings show

	DeadlockDetector::getInstance().allowThreadToTakeMultipleLocks(Log::GetThreadId(), true);

#if DUET_SIMULATOR
	// As the default preset, whatever an earlier test in the same process kept
	Sim::Simulator::get().reset();
	Sim::Simulator::get().setManual(true);
	Sim::Simulator::get().setConfig(Sim::defaultConfig());
#endif

	OM::RemoveAll();

	// The file cache's folders are the test's own: nothing kept from running the program, nor shared with tests
	// running side by side
	std::filesystem::remove_all(cacheFolder());
	Comm::FileInfoCache::get().setFolders(cacheFolder() / "kept", cacheFolder() / "thumbnails");
	Comm::FileInfoCache::get().forget();
}

TestSuite::~TestSuite()
{
	ZoneScoped;
	/* Run at end of each test */
	Comm::Connection::get().stop();
	if (s_wantEverything != 0)
	{
		OM::Tree::get().unwant(s_wantEverything);
		s_wantEverything = 0;
	}
#if DUET_SIMULATOR
	Sim::Simulator::get().reset();
#endif
	OM::RemoveAll();
	Comm::FileInfoCache::get().forget();
	std::filesystem::remove_all(cacheFolder());
	std::filesystem::remove(configFile());
}

std::filesystem::path TestSuite::cacheFolder()
{
	return std::filesystem::temp_directory_path() / fmt::format("duetscreen-tests-{:d}-cache", ::getpid());
}

std::filesystem::path TestSuite::configFile()
{
	return std::filesystem::temp_directory_path() / fmt::format("duetscreen-tests-{:d}.json", ::getpid());
}

ScriptedPrinter& TestSuite::connectScriptedPrinter()
{
	auto port = std::make_unique<ScriptedPrinter>();
	ScriptedPrinter& printer = *port;
	Comm::Connection::get().connectThrough(std::move(port));
	EXPECT_TRUE(printer.answer("M409", R"({"key":"","flags":"d99f","result":{}})"));
	EXPECT_TRUE(Comm::Connection::get().isConnected());
	return printer;
}

bool TestSuite::load_model_data_from_file(std::string_view filename)
{
	ZoneScoped;
	std::ifstream file(filename.data());

	EXPECT_TRUE(file.is_open());

	std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return load_model_data(data);
}

bool TestSuite::load_model_data(std::string_view data)
{
	ZoneScoped;
	// Each line is a reply, as if the printer had sent it unasked, or (a folder listing) to the request waiting for it
	Comm::LineFramer framer(
		[](std::string_view line)
		{
			auto reply = OM::parseJson(line);
			if (!reply)
			{
				LOG_WARN("Not JSON: {:s}", line);
				return;
			}
			if ((*reply)["dir"].exists())
			{
				OM::FileSystem::FileListArrived(std::string((*reply)["dir"].text()), Comm::FileList::read(*reply));
				return;
			}
			Comm::Connection::get().receiveUnasked(*reply);
		});
	framer.push(data);
	framer.push("\n"); // a recorded reply may not end its line
	return true;
}
#if DUET_SIMULATOR
void TestSuite::connectSimulator(const Sim::Config& config)
{
	Sim::Simulator& simulator = Sim::Simulator::get();
	simulator.setManual(true);
	simulator.setConfig(config);
	Sim::Behaviour behaviour = simulator.behaviour();
	behaviour.noise = 0.0f;
	simulator.setBehaviour(behaviour);

	// Tests read the whole object model, as a UI binding to all of it would
	if (s_wantEverything == 0)
	{
		s_wantEverything = OM::Tree::get().want({"boards",
												 "directories",
												 "fans",
												 "global",
												 "heat",
												 "inputs",
												 "job",
												 "ledStrips",
												 "limits",
												 "move",
												 "network",
												 "sensors",
												 "spindles",
												 "state",
												 "tools",
												 "volumes"});
	}

	Comm::Connection& connection = Comm::Connection::get();
	connection.setType(Comm::CommunicationType::simulator);
	connection.process(); // opens the port and asks for the live model
	simulator.process();
	connection.process(); // its answer: connected
	ASSERT_TRUE(connection.isConnected());
}

void TestSuite::pollSimulator(int rounds)
{
	Comm::Connection& connection = Comm::Connection::get();
	for (int i = 0; i < rounds; i++)
	{
		connection.pollNow();
		// Until everything asked is answered (or the simulated Duet doesn't answer)
		for (int exchange = 0; exchange < maxExchanges; exchange++)
		{
			connection.process();
			Sim::Simulator::get().process();
			connection.process();
			if (!connection.busy())
			{
				break;
			}
		}
	}
}

void TestSuite::sendToSimulator(std::string_view gcode)
{
	Comm::Connection::get().sendGcode(gcode);
	Comm::Connection::get().process(); // sends it
	Sim::Simulator::get().process();
	Comm::Connection::get().process(); // takes the replies
}

void TestSuite::simulate(std::chrono::milliseconds time)
{
	Sim::Simulator::get().advance(time);
	pollSimulator();
}
#endif

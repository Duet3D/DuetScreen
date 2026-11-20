/*
 * TestSuite.cpp
 *
 *  Created on: 2025-10-07
 *      Author: Andy Everitt
 */

#include "TestSuite.h"
#include "Comm/Communication.h"
#include "Comm/LineFramer.h"
#include "Comm/Receive.h"
#include "Configuration.h"
#include "DeadlockDetector.h"
#include "Debug.h"
#include "Hardware/Duet.h"
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

	Comm::init();

	DeadlockDetector::getInstance().allowThreadToTakeMultipleLocks(Log::GetThreadId(), true);

#if DUET_SIMULATOR
	// As the default preset, whatever an earlier test in the same process kept
	Sim::Simulator::get().reset();
	Sim::Simulator::get().setManual(true);
	Sim::Simulator::get().setConfig(Sim::defaultConfig());
#endif

	OM::RemoveAll();

	std::filesystem::remove_all("/tmp/thumbnails");
	std::filesystem::create_directories("/tmp/thumbnails");
}

TestSuite::~TestSuite()
{
	ZoneScoped;
	/* Run at end of each test */
#if DUET_SIMULATOR
	if (Comm::DUET.GetCommunicationType() == Comm::CommunicationType::simulator)
	{
		Comm::DUET.Disconnect();
	}
	Sim::Simulator::get().reset();
#endif
	OM::RemoveAll();
	std::filesystem::remove_all("/tmp/thumbnails");
	std::filesystem::remove(configFile());
}

std::filesystem::path TestSuite::configFile()
{
	return std::filesystem::temp_directory_path() / fmt::format("duetscreen-tests-{:d}.json", ::getpid());
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
	Comm::LineFramer framer(Comm::receiveLine);
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
	if (Comm::DUET.GetCommunicationType() == Comm::CommunicationType::simulator)
	{
		Comm::DUET.Connect();
	}
	else
	{
		Comm::DUET.SetCommunicationType(Comm::CommunicationType::simulator);
	}
	ASSERT_TRUE(Comm::DUET.IsConnected());
	simulator.process(); // the replies to connecting
}

void TestSuite::pollSimulator(int rounds)
{
	for (int i = 0; i < rounds; i++)
	{
		Comm::sendNext();
		Sim::Simulator::get().process();
	}
}

void TestSuite::sendToSimulator(std::string_view gcode)
{
	Comm::DUET.SendGcode(gcode);
	Sim::Simulator::get().process();
}

void TestSuite::simulate(std::chrono::milliseconds time)
{
	Sim::Simulator::get().advance(time);
	pollSimulator();
}
#endif

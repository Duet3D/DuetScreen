/*
 * test_simulator.cpp
 *
 * The simulated Duet (src/Simulator): the machine on its own (G-code, time passing, its object model and replies),
 * then as a connection, its replies decoded into the screen's object model as a real Duet's are.
 */

#if DUET_SIMULATOR

#  include "Comm/Communication.h"
#  include "Hardware/Duet.h"
#  include "ObjectModel/Tree/Tree.h"
#  include "Simulator/Machine.h"
#  include "Simulator/Simulator.h"
#  include "test_utils/TestSuite.h"
#  include <algorithm>
#  include <fmt/format.h>
#  include <gtest/gtest.h>
#  include <set>
#  include <string>

using Sim::Json;
using Sim::Machine;

class TestSimulatedMachine : public TestSuite
{
  protected:
	/** A machine as the default preset, without noise on its temperatures */
	TestSimulatedMachine() { machine.setNoise(0.0f); }

	/** Run `gcode` on the machine; @return the text it replies, if any */
	std::string run(std::string_view gcode)
	{
		std::string text;
		for (const Json& reply : machine.execute(gcode))
		{
			if (reply.contains("resp"))
			{
				text += reply["resp"].get<std::string>();
			}
		}
		return text;
	}

	/** The machine's object model at `path`, e.g. "heat/heaters/0/current" */
	Json model(const std::string& path) const { return machine.objectModel()[Json::json_pointer("/" + path)]; }

	/** M409's reply for `key` with `flags` */
	Json request(std::string_view key, std::string_view flags)
	{
		const auto replies = machine.execute(fmt::format("M409 K\"{:s}\" F\"{:s}\"", key, flags));
		return replies.empty() ? Json() : replies.front();
	}

	Machine machine{Sim::defaultConfig()}; // A single tool printer: bed is heater 0, the hotend heater 1
};

TEST_F(TestSimulatedMachine, EveryPresetReportsWhatItIs)
{
	for (const Sim::Preset& preset : Sim::presets())
	{
		const Sim::Config config = preset.make();
		const Machine machine(config);
		const Json model = machine.objectModel();
		EXPECT_EQ(model["heat"]["heaters"].size(), config.heaters.size()) << preset.key;
		EXPECT_EQ(model["tools"].size(), config.tools.size()) << preset.key;
		EXPECT_EQ(model["move"]["extruders"].size(), config.extruders.size()) << preset.key;
		EXPECT_EQ(model["move"]["axes"].size(), config.axes.size()) << preset.key;
		EXPECT_EQ(model["fans"].size(), config.fans.size()) << preset.key;
		EXPECT_EQ(model["network"]["name"], config.name) << preset.key;
		EXPECT_EQ(model["state"]["status"], "idle") << preset.key;
	}
}

TEST_F(TestSimulatedMachine, ConfigurationSurvivesJson)
{
	for (const Sim::Preset& preset : Sim::presets())
	{
		const nlohmann::json json = preset.make();
		const Sim::Config back = json.get<Sim::Config>();
		EXPECT_EQ(nlohmann::json(back), json) << preset.key;
	}
	// What is missing takes its default
	const Sim::Config partial = nlohmann::json::parse(R"({"name":"Mine","axes":[{"letter":"X"}]})").get<Sim::Config>();
	EXPECT_EQ(partial.name, "Mine");
	ASSERT_EQ(partial.axes.size(), 1u);
	EXPECT_EQ(partial.axes[0].max, 300.0f);
	EXPECT_TRUE(partial.heaters.empty());
}

TEST_F(TestSimulatedMachine, HeatersTrendTowardsTheirTargets)
{
	run("M140 S60");
	EXPECT_EQ(model("heat/heaters/0/state"), "active");
	EXPECT_EQ(model("heat/heaters/0/active"), 60.0);

	machine.advance(10.0);
	const double warming = model("heat/heaters/0/current").get<double>();
	EXPECT_GT(warming, 25.0);
	EXPECT_LT(warming, 60.0);

	machine.advance(300.0);
	EXPECT_NEAR(model("heat/heaters/0/current").get<double>(), 60.0, 0.5);

	// Off, it cools again
	run("M140 S-273.1");
	EXPECT_EQ(model("heat/heaters/0/state"), "off");
	machine.advance(600.0);
	EXPECT_NEAR(model("heat/heaters/0/current").get<double>(), 21.0, 1.0);
}

TEST_F(TestSimulatedMachine, ReadingsAreNoisyWhenAsked)
{
	machine.setNoise(0.5f);
	std::set<double> readings;
	for (int i = 0; i < 10; i++)
	{
		machine.advance(0.1);
		readings.insert(model("heat/heaters/0/current").get<double>());
	}
	EXPECT_GT(readings.size(), 5u);
	for (const double reading : readings)
	{
		EXPECT_NEAR(reading, 21.0, 3.0);
	}
}

TEST_F(TestSimulatedMachine, ReadingsAreSmoothWhileHeatingOrCooling)
{
	machine.setNoise(0.5f);
	run("M140 S60");
	double last = model("heat/heaters/0/current").get<double>();
	for (int i = 0; i < 100; i++) // heating, until 2 °C short
	{
		machine.advance(0.1);
		const double reading = model("heat/heaters/0/current").get<double>();
		if (reading > 58.0)
		{
			break;
		}
		EXPECT_GT(reading, last) << i;
		last = reading;
	}
	machine.advance(120.0);
	std::set<double> holding;
	for (int i = 0; i < 10; i++)
	{
		machine.advance(0.1);
		holding.insert(model("heat/heaters/0/current").get<double>());
	}
	EXPECT_GT(holding.size(), 5u); // noisy once there
}

TEST_F(TestSimulatedMachine, TemperaturesAboveTheLimitAreRefused)
{
	EXPECT_NE(run("M104 S400").find("Error"), std::string::npos);
	EXPECT_EQ(model("heat/heaters/1/active"), 0.0);
}

TEST_F(TestSimulatedMachine, AToolsHeatersFollowTheToolInUse)
{
	run("G10 P0 S210 R150");
	EXPECT_EQ(model("heat/heaters/1/state"), "off");
	EXPECT_EQ(model("tools/0/active/0"), 210.0);
	EXPECT_EQ(model("tools/0/standby/0"), 150.0);

	run("T0");
	EXPECT_EQ(model("state/currentTool"), 0);
	EXPECT_EQ(model("tools/0/state"), "active");
	EXPECT_EQ(model("heat/heaters/1/state"), "active");
	EXPECT_EQ(model("state/status"), "changingTool");
	machine.advance(1.0);
	EXPECT_EQ(model("state/status"), "idle");

	run("T-1");
	EXPECT_EQ(model("state/currentTool"), -1);
	EXPECT_EQ(model("tools/0/state"), "standby");
	EXPECT_EQ(model("heat/heaters/1/state"), "standby");
	EXPECT_EQ(model("state/previousTool"), 0);
}

TEST_F(TestSimulatedMachine, HomingAndDisablingTheMotors)
{
	EXPECT_FALSE(model("move/axes/0/homed").get<bool>());
	run("G28 X");
	EXPECT_TRUE(model("move/axes/0/homed").get<bool>());
	EXPECT_FALSE(model("move/axes/1/homed").get<bool>());

	run("G28");
	for (int axis = 0; axis < 3; axis++)
	{
		EXPECT_TRUE(model(fmt::format("move/axes/{:d}/homed", axis)).get<bool>()) << axis;
	}

	run("M18 Y");
	EXPECT_TRUE(model("move/axes/0/homed").get<bool>());
	EXPECT_FALSE(model("move/axes/1/homed").get<bool>());

	run("M18");
	EXPECT_FALSE(model("move/axes/0/homed").get<bool>());
	EXPECT_FALSE(model("move/axes/2/homed").get<bool>());
}

TEST_F(TestSimulatedMachine, AxesTravelOverTime)
{
	EXPECT_NE(run("G1 X10").find("insufficient axes homed"), std::string::npos);

	run("G28");
	machine.advance(1.0);
	run("G1 X100 F6000"); // 100 mm/s
	EXPECT_EQ(model("state/status"), "busy");
	machine.advance(0.5);
	EXPECT_NEAR(model("move/axes/0/machinePosition").get<double>(), 50.0, 0.1);
	machine.advance(1.0);
	EXPECT_NEAR(model("move/axes/0/machinePosition").get<double>(), 100.0, 0.001);
	EXPECT_EQ(model("state/status"), "idle");

	// Relative moves (jogging), kept within the axis's limits
	run("M120\nG91\nG1 X-30 F6000\nM121");
	machine.advance(1.0);
	EXPECT_NEAR(model("move/axes/0/userPosition").get<double>(), 70.0, 0.001);
	run("G91");
	run("G1 X-500");
	machine.advance(5.0);
	EXPECT_NEAR(model("move/axes/0/machinePosition").get<double>(), 0.0, 0.001);
	run("G90");

	// A workplace's offsets move what the user sees, not the machine
	run("G1 X50");
	machine.advance(5.0);
	run("G10 L20 P1 X0");
	EXPECT_NEAR(model("move/axes/0/userPosition").get<double>(), 0.0, 0.001);
	EXPECT_NEAR(model("move/axes/0/machinePosition").get<double>(), 50.0, 0.001);
	run("G53 G1 X60");
	machine.advance(5.0);
	EXPECT_NEAR(model("move/axes/0/machinePosition").get<double>(), 60.0, 0.001);
}

TEST_F(TestSimulatedMachine, AJobPrintsPausesAndIsCancelled)
{
	run("M32 \"0:/gcodes/benchy.gcode\"");
	EXPECT_EQ(model("state/status"), "processing");
	EXPECT_EQ(model("job/file/fileName"), "0:/gcodes/benchy.gcode");
	EXPECT_EQ(model("job/file/numLayers"), 240);
	EXPECT_TRUE(model("job/layer").is_null());			 // still warming up
	EXPECT_TRUE(model("move/axes/0/homed").get<bool>()); // homed first, as its start G-code would
	EXPECT_EQ(model("heat/heaters/0/active"), 60.0);
	EXPECT_EQ(model("heat/heaters/1/active"), 210.0);

	machine.advance(600.0); // warm
	ASSERT_FALSE(model("job/layer").is_null());
	machine.advance(600.0);
	const int layer = model("job/layer").get<int>();
	EXPECT_GT(layer, 1);
	EXPECT_GT(model("job/filePosition").get<int64_t>(), 0);
	EXPECT_GT(model("move/axes/2/machinePosition").get<double>(), 0.2);

	run("M25");
	EXPECT_EQ(model("state/status"), "pausing");
	machine.advance(1.5);
	EXPECT_EQ(model("state/status"), "paused");
	machine.advance(100.0);
	EXPECT_EQ(model("job/layer").get<int>(), layer);

	run("M24");
	EXPECT_EQ(model("state/status"), "resuming");
	machine.advance(1.5);
	EXPECT_EQ(model("state/status"), "processing");

	run("M25");
	machine.advance(2.0);
	run("M0");
	EXPECT_EQ(model("state/status"), "cancelling");
	machine.advance(2.0);
	EXPECT_EQ(model("state/status"), "idle");
	EXPECT_EQ(model("job/lastFileName"), "0:/gcodes/benchy.gcode");
	EXPECT_EQ(model("heat/heaters/1/state"), "off");
}

TEST_F(TestSimulatedMachine, AJobFinishes)
{
	run("M32 \"first layer test.gcode\"");
	machine.advance(600.0);
	EXPECT_EQ(model("state/status"), "idle");
	EXPECT_EQ(model("job/lastFileName"), "0:/gcodes/first layer test.gcode");
	EXPECT_TRUE(model("job/file/fileName").is_null());
}

TEST_F(TestSimulatedMachine, ObjectsCanBeCancelled)
{
	run("M32 \"benchy.gcode\"");
	ASSERT_EQ(model("job/build/objects").size(), 3u);
	run("M486 P1");
	EXPECT_TRUE(model("job/build/objects/1/cancelled").get<bool>());
	EXPECT_FALSE(model("job/build/objects/0/cancelled").get<bool>());
}

TEST_F(TestSimulatedMachine, MessageBoxes)
{
	run("M291 P\"Pick one\" R\"Colour\" S4 K{\"Red\",\"Blue\"} J1");
	const Json box = model("state/messageBox");
	EXPECT_EQ(box["mode"], 4);
	EXPECT_EQ(box["message"], "Pick one");
	EXPECT_EQ(box["title"], "Colour");
	EXPECT_EQ(box["choices"], Json::array({"Red", "Blue"}));
	EXPECT_TRUE(box["cancelButton"].get<bool>());

	run(fmt::format("M292 S{:d}", box["seq"].get<int>() + 1)); // another box's answer
	EXPECT_FALSE(model("state/messageBox").is_null());
	run(fmt::format("M292 R{{0}} S{:d}", box["seq"].get<int>()));
	EXPECT_TRUE(model("state/messageBox").is_null());

	for (int mode = 0; mode <= 7; mode++)
	{
		machine.showMessageBox(mode);
		EXPECT_EQ(model("state/messageBox/mode"), mode);
	}
}

TEST_F(TestSimulatedMachine, EmergencyStopHaltsUntilReset)
{
	run("G28\nT0\nM104 S200");
	run("M112");
	EXPECT_EQ(model("state/status"), "halted");
	EXPECT_EQ(model("heat/heaters/1/state"), "off");
	EXPECT_NE(run("G28").find("halted"), std::string::npos);

	run("M999");
	EXPECT_EQ(model("state/status"), "idle");
	EXPECT_FALSE(model("move/axes/0/homed").get<bool>());

	// As the screen's E-stop sends it: both at once
	run("G28");
	run("M112 M999");
	EXPECT_EQ(model("state/status"), "idle");
	EXPECT_FALSE(model("move/axes/0/homed").get<bool>());
}

TEST_F(TestSimulatedMachine, ForcedStatus)
{
	machine.forceStatus("off");
	EXPECT_EQ(model("state/status"), "off");
	machine.forceStatus(std::nullopt);
	EXPECT_EQ(model("state/status"), "idle");
}

TEST_F(TestSimulatedMachine, FactorsFansAndSpindles)
{
	run("M220 S150\nM221 D0 S90\nM106 P0 S0.5\nM106 P1 S255");
	EXPECT_EQ(model("move/speedFactor"), 1.5);
	EXPECT_EQ(model("move/extruders/0/factor"), 0.9);
	EXPECT_EQ(model("fans/0/requestedValue"), 0.5);
	EXPECT_EQ(model("fans/1/requestedValue"), 0.0); // thermostatic: follows the hotend, still cold
	machine.advance(0.1);
	EXPECT_EQ(model("fans/1/actualValue"), 0.0);

	Machine mill(Sim::presets()[3].make()); // CNC
	mill.execute("T0\nM568 P0 F12000\nM3");
	mill.advance(5.0);
	const Json model = mill.objectModel();
	EXPECT_EQ(model["spindles"][0]["state"], "forward");
	EXPECT_EQ(model["spindles"][0]["current"], 12000);
	EXPECT_EQ(model["spindles"][1]["state"], "unconfigured");
	EXPECT_EQ(model["state"]["machineMode"], "CNC");
}

TEST_F(TestSimulatedMachine, LiveRequestsHaveOnlyWhatChangesOften)
{
	const Json reply = request("", "d99f");
	EXPECT_EQ(reply["key"], "");
	const Json& result = reply["result"];
	EXPECT_TRUE(result["heat"]["heaters"][0].contains("current"));
	EXPECT_FALSE(result["heat"]["heaters"][0].contains("max"));
	EXPECT_TRUE(result["state"].contains("status"));
	EXPECT_FALSE(result["state"].contains("machineMode"));
	EXPECT_TRUE(result["seqs"].contains("heat"));
	EXPECT_FALSE(result.contains("network"));
}

TEST_F(TestSimulatedMachine, SeqsCountChangesToWhatIsNotLive)
{
	const auto seq = [this](const char* key) { return request("", "d99f")["result"]["seqs"][key].get<int>(); };
	const int heat = seq("heat");
	const int state = seq("state");
	machine.advance(5.0);
	EXPECT_EQ(seq("heat"), heat); // temperatures are live: no new seq
	EXPECT_EQ(seq("state"), state);

	Sim::Config config = machine.config();
	config.heaters[1].max = 250.0f;
	machine.reconfigure(config);
	EXPECT_EQ(seq("heat"), heat + 1);

	run("M117 \"Hello\"");
	EXPECT_EQ(seq("state"), state + 1);
}

TEST_F(TestSimulatedMachine, RequestsForAKey)
{
	const Json heat = request("heat", "vo");
	EXPECT_EQ(heat["key"], "heat");
	EXPECT_EQ(heat["result"]["heaters"].size(), 2u);
	EXPECT_EQ(heat["result"]["bedHeaters"][0], 0);

	const Json tools = request("tools", "v");
	EXPECT_TRUE(tools["result"].is_array());
	EXPECT_EQ(tools["next"], 0);

	// Without n, no nulls; with it, nulls
	EXPECT_FALSE(request("job", "v")["result"].contains("build"));
	EXPECT_TRUE(request("job", "vn")["result"]["build"].is_null());

	// move holds five axes at most; the rest come as move.axes from the sixth
	Sim::Config config = machine.config();
	for (const char* letter : {"U", "V", "W", "A"})
	{
		config.axes.push_back({.letter = letter});
	}
	machine.reconfigure(config);
	EXPECT_EQ(request("move", "vn")["result"]["axes"].size(), 5u);
	const Json more = request("move.axes", "vna5");
	ASSERT_EQ(more["result"].size(), 2u);
	EXPECT_EQ(more["result"][1]["letter"], "A");
}

TEST_F(TestSimulatedMachine, FilesAndTheirInformation)
{
	const auto replies = machine.execute("M20 S3 P\"0:/gcodes/\" R0");
	ASSERT_EQ(replies.size(), 1u);
	EXPECT_EQ(replies[0]["dir"], "0:/gcodes/");
	EXPECT_EQ(replies[0]["files"].size(), machine.config().files.size());
	// In the order the screen's decoder expects
	EXPECT_EQ(replies[0].dump().find("\"dir\""), 1u);

	const auto info = machine.execute("M36 \"0:/gcodes/benchy.gcode\"");
	EXPECT_EQ(info[0]["err"], 0);
	EXPECT_EQ(info[0]["numLayers"], 240);
	EXPECT_EQ(machine.execute("M36 \"nothing.gcode\"")[0]["err"], 1);

	const auto contents = machine.execute("M36.2 P\"0:/macros/Park.g\" S0");
	const std::string text = contents[0].dump();
	EXPECT_LT(text.find("\"next\""), text.find("\"err\"")); // err ends the fragment, so comes last

	machine.execute("M30 \"0:/gcodes/benchy.gcode\"");
	EXPECT_TRUE(machine.takeConfigChange());
	EXPECT_EQ(machine.execute("M20 S3 P\"0:/gcodes\" R0")[0]["files"].size(), machine.config().files.size());
	EXPECT_EQ(machine.config().files.size(), Sim::defaultConfig().files.size() - 1);
}

TEST_F(TestSimulatedMachine, ReconfiguringKeepsWhatStillApplies)
{
	run("G28\nM140 S60");
	machine.advance(30.0);
	const double bed = model("heat/heaters/0/current").get<double>();

	Sim::Config config = machine.config();
	config.heaters.push_back({.name = "Dryer", .role = Sim::HeaterRole::other});
	config.heaters[1].fault = true;
	machine.reconfigure(config);
	EXPECT_EQ(model("heat/heaters").size(), 3u);
	EXPECT_EQ(model("heat/heaters/0/current").get<double>(), bed);
	EXPECT_EQ(model("heat/heaters/1/state"), "fault");
	EXPECT_TRUE(model("move/axes/0/homed").get<bool>());
	EXPECT_EQ(model("sensors/analog/2/name"), "Dryer");
}

TEST_F(TestSimulatedMachine, HeatersHeatAndCoolAtTheirRates)
{
	run("T0\nM104 S210");
	machine.advance(5.0); // 10 °C/s, until close
	EXPECT_NEAR(model("heat/heaters/1/current").get<double>(), 71.0, 1.0);
	machine.advance(60.0);
	EXPECT_NEAR(model("heat/heaters/1/current").get<double>(), 210.0, 0.5);

	run("M104 S-273.1\nT-1\nM568 P0 A0");
	machine.advance(5.0); // 4 °C/s at most
	EXPECT_NEAR(model("heat/heaters/1/current").get<double>(), 190.0, 1.0);
}

TEST_F(TestSimulatedMachine, ToolsHaveTheExtrudersConfigured)
{
	EXPECT_EQ(model("move/extruders/0/filament"), "PLA"); // loaded when it starts
	Sim::Config config = machine.config();
	config.extruders.push_back({});
	config.tools[0].extruders = {0, 1};
	machine.reconfigure(config);
	EXPECT_EQ(model("move/extruders").size(), 2u);
	EXPECT_EQ(model("tools/0/extruders"), Json::array({0, 1}));
	EXPECT_EQ(model("tools/0/mix"), Json::array({1.0, 0.0}));
	EXPECT_EQ(model("tools/0/filamentExtruder"), 0);

	run("T0\nM702\nM701 S\"PETG\"");
	EXPECT_EQ(model("move/extruders/0/filament"), "PETG"); // its first takes the filament
	EXPECT_EQ(model("move/extruders/1/filament"), "");

	config.tools[0].extruders = {};
	machine.reconfigure(config);
	EXPECT_EQ(model("tools/0/filamentExtruder"), -1);
	EXPECT_NE(run("M701 S\"PLA\"").find("Error"), std::string::npos);
}

TEST_F(TestSimulatedMachine, HeightmapsAreOnTheSdCard)
{
	const auto list = machine.execute("M20 S3 P\"0:/sys/\" R0")[0]["files"];
	std::vector<std::string> names;
	for (const Json& file : list)
	{
		names.push_back(file["name"]);
	}
	for (const char* name : {"heightmap.csv", "tilted.csv", "warped.csv", "partial.csv"})
	{
		EXPECT_TRUE(std::ranges::contains(names, name)) << name;
	}

	const std::string tilted = machine.execute("M36.2 P\"0:/sys/tilted.csv\" S0")[0]["fragment"]["data"];
	EXPECT_TRUE(tilted.starts_with("RepRapFirmware height map file v2"));
	EXPECT_NE(tilted.find("X,Y,10.00,290.00,10.00,290.00,-1.00,28.00,28.00,11,11"), std::string::npos);

	// Not probed points are written 0, as RepRapFirmware writes them
	const std::string partial = machine.execute("M36.2 P\"0:/sys/partial.csv\" S0")[0]["fragment"]["data"];
	EXPECT_NE(partial.find("      0,"), std::string::npos);
	EXPECT_EQ(tilted.find("      0,"), std::string::npos);
}

TEST_F(TestSimulatedMachine, ProbingMakesAndUsesAHeightmap)
{
	EXPECT_EQ(model("move/compensation/type"), "none");
	EXPECT_NE(run("G29").find("insufficient axes homed"), std::string::npos);

	run("G28");
	machine.advance(2.0);
	run("G29");
	EXPECT_EQ(model("state/status"), "busy");
	EXPECT_EQ(model("move/compensation/type"), "mesh");
	EXPECT_EQ(model("move/compensation/file"), "0:/sys/heightmap.csv");
	EXPECT_EQ(model("move/compensation/liveGrid/mins"), Json::array({10.0f, 10.0f}));

	run("G29 S1 P\"warped.csv\"");
	EXPECT_EQ(model("move/compensation/file"), "0:/sys/warped.csv");
	EXPECT_NE(run("G29 S1 P\"none.csv\"").find("not found"), std::string::npos);

	run("G29 S2");
	EXPECT_EQ(model("move/compensation/type"), "none");
}

TEST_F(TestSimulatedMachine, JobsHaveThumbnails)
{
	const Json info = machine.execute("M36 \"0:/gcodes/benchy.gcode\"")[0];
	ASSERT_EQ(info["thumbnails"].size(), 2u);
	const Json& large = info["thumbnails"][1];
	EXPECT_EQ(large["format"], "qoi");
	EXPECT_EQ(large["width"], 160);
	EXPECT_EQ(machine.execute("M36 \"0:/gcodes/first layer test.gcode\"")[0]["thumbnails"].size(), 0u);

	// Read in turn, as the screen does, its parts make the whole: a QOI image
	std::string base64;
	uint32_t offset = large["offset"];
	while (offset != 0)
	{
		const Json part =
			machine.execute(fmt::format("M36.1 P\"0:/gcodes/benchy.gcode\" S{:d}", offset))[0]["thumbnail"];
		ASSERT_EQ(part["err"], 0);
		base64 += part["data"].get<std::string>();
		offset = part["next"];
	}
	EXPECT_EQ(base64.size(), large["size"].get<size_t>());
	EXPECT_TRUE(base64.starts_with("cW9pZg")); // "qoif"

	EXPECT_EQ(machine.execute("M36.1 P\"0:/gcodes/benchy.gcode\" S1")[0]["thumbnail"]["err"], 1);
}

/** The simulated Duet as the screen's connection */
class TestSimulator : public TestSuite
{
  protected:
	/** The screen's object model at `path`, e.g. "heat.heaters[0].current" */
	static const OM::Node& om(std::string_view path) { return OM::Tree::get().root().at(path); }
};

TEST_F(TestSimulator, TheScreenReadsItsObjectModel)
{
	connectSimulator();
	pollSimulator();
	EXPECT_EQ(om("network.name").text(), "Simulated FFF");
	EXPECT_EQ(om("heat.heaters").size(), 2u);
	EXPECT_EQ(om("tools[0].name").text(), "Hotend");
	EXPECT_EQ(om("move.axes").size(), 3u);
	EXPECT_EQ(om("state.status").text(), "idle");
	EXPECT_EQ(om("directories.gCodes").text(), "0:/gcodes/");
}

TEST_F(TestSimulator, ChangesReachTheScreen)
{
	connectSimulator();
	pollSimulator();
	sendToSimulator("G28\nM140 S60");
	simulate(std::chrono::seconds(300));
	EXPECT_TRUE(om("move.axes[0].homed").boolean());
	EXPECT_EQ(om("heat.heaters[0].state").text(), "active");
	EXPECT_NEAR(om("heat.heaters[0].current").number(), 60.0, 0.5);

	// A change to what isn't live (a new seq) is asked for again
	Sim::Config config = Sim::Simulator::get().config();
	config.tools[0].name = "Renamed";
	Sim::Simulator::get().setConfig(config);
	pollSimulator();
	EXPECT_EQ(om("tools[0].name").text(), "Renamed");
}

TEST_F(TestSimulator, MoreThanFiveAxesAreAllRead)
{
	Sim::Config config = Sim::defaultConfig();
	for (const char* letter : {"U", "V", "W", "A"})
	{
		config.axes.push_back({.letter = letter});
	}
	connectSimulator(config);
	pollSimulator();
	ASSERT_EQ(om("move.axes").size(), 7u);
	EXPECT_EQ(om("move.axes[6].letter").text(), "A");
}

TEST_F(TestSimulator, UnpluggedItCantBeConnectedTo)
{
	Sim::Simulator& simulator = Sim::Simulator::get();
	Sim::Behaviour behaviour = simulator.behaviour();
	behaviour.pluggedIn = false;
	simulator.setBehaviour(behaviour);
	Comm::DUET.SetCommunicationType(Comm::CommunicationType::simulator);
	EXPECT_FALSE(Comm::DUET.IsConnected());

	behaviour.pluggedIn = true;
	simulator.setBehaviour(behaviour);
	EXPECT_TRUE(Comm::DUET.Connect());
	EXPECT_TRUE(Comm::DUET.IsConnected());
}

TEST_F(TestSimulator, NotRespondingItAnswersNothing)
{
	connectSimulator();
	Sim::Simulator& simulator = Sim::Simulator::get();
	Sim::Behaviour behaviour = simulator.behaviour();
	behaviour.responding = false;
	simulator.setBehaviour(behaviour);
	pollSimulator();
	EXPECT_FALSE(om("network.name").exists());

	behaviour.responding = true;
	simulator.setBehaviour(behaviour);
	Comm::Reconnect(); // as the screen does when it times out
	simulator.process();
	pollSimulator();
	EXPECT_EQ(om("network.name").text(), "Simulated FFF");
}

TEST_F(TestSimulator, ItsConfigurationIsKept)
{
	Sim::Config config = Sim::presets()[3].make();
	Sim::Simulator::get().setConfig(config);
	Sim::Simulator::get().reset(); // as the program starting again does
	EXPECT_EQ(Sim::Simulator::get().config().name, config.name);
	EXPECT_EQ(Sim::Simulator::get().config().spindles.size(), 1u);
}

#endif

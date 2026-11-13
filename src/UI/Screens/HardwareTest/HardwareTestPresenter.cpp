/*
 * HardwareTestPresenter.cpp
 */

#include "HardwareTestPresenter.h"
#include "BuildDate.h"
#include "Debug.h"
#include "Hardware/Usb.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "nameof.hpp"
#include "utils/NetworkHelper.h"
#include "utils/SystemHelper.h"
#include "version.h"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <thread>
#include <unistd.h>

#ifndef HARDWARE_TEST_SERIAL_NUMBER
#  define HARDWARE_TEST_SERIAL_NUMBER ""
#endif

namespace UI
{
	namespace
	{
#if SIMULATION
		constexpr const char* LOG_FOLDER = "./logs/"; // Where the logs are written
#else
		constexpr const char* LOG_FOLDER = "/boot/logs/"; // Where the logs are written
#endif
		constexpr int32_t TOUCH_TOLERANCE = 50;			 // How far, in pixels, a tap may be from its target
		constexpr int32_t TARGET_INSET = 30;			 // How far the corner targets are from the edges
		constexpr auto SETTLE = std::chrono::seconds(2); // Time for the USB monitor to see a change

		/** What `command` writes, run in a shell */
		std::string runCommand(const std::string& command)
		{
			ZoneScoped;
			std::string result;
			FILE* pipe = ::popen(command.c_str(), "r");
			if (pipe == nullptr)
			{
				return result;
			}
			char buffer[256];
			while (std::fgets(buffer, sizeof(buffer), pipe))
			{
				result.append(buffer);
			}
			::pclose(pipe);
			return result;
		}

		/** The MAC address `ip link` lists for a network interface, or empty */
		std::string extractMacAddress(const std::string& input)
		{
			static const std::regex macRegex("link/ether ((?:[0-9a-fA-F]{2}[:-]){5}(?:[0-9a-fA-F]{2}))");
			std::smatch match;
			return std::regex_search(input, match, macRegex) ? match.str(1) : std::string();
		}

		/** `json` as indented text */
		std::string dump(const nlohmann::json& json)
		{
			return json.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
		}
	} // namespace

	const std::vector<HardwareTestPresenter::TestColour> HardwareTestPresenter::COLOURS = {
		{lv_color_hex(0xFF0000), "Red"},
		{lv_color_hex(0x00FF00), "Green"},
		{lv_color_hex(0x0000FF), "Blue"},
		{lv_color_hex(0xFFFFFF), "White"},
	};

	TestProcedure::TestProcedure(TestId id,
								 std::function<void(TestProcedure&)> start,
								 std::function<bool(TestProcedure&)> judge,
								 std::function<void(TestProcedure&)> cleanup)
		: m_id(id)
		, m_start(std::move(start))
		, m_judge(std::move(judge))
		, m_cleanup(std::move(cleanup))
	{
	}

	void TestProcedure::start()
	{
		output.clear();
		if (m_start)
		{
			m_start(*this);
		}
	}

	bool TestProcedure::finish()
	{
		const bool passed = m_judge ? m_judge(*this) : true;
		m_failed = !passed;
		return passed;
	}

	void TestProcedure::cleanup()
	{
		if (m_cleanup)
		{
			m_cleanup(*this);
		}
	}

	void TestResultRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "The test, e.g. \"UsbTest\"");
		f.field("output", output, "What it found, as JSON");
		f.field("passed", passed, "1 if it passed");
	}

	HardwareTestPresenter::HardwareTestPresenter()
		: XmlPresenter("hardware_test")
		, m_stage("serial")
	{
#if USE_FIXED_TEST_STRINGS
		m_version.set(_("settings.build_time", "vX.Y.Z", "YYYY-MM-DD", "HH:mm::ss"));
#else
		m_version.set(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));
#endif
		action("hardware_test.serial",
			   "Use the serial number typed in the box (`ready`) and start the tests, else say it is invalid",
			   [this](const Xml::ActionArgs& args) { m_serialInvalid.set(setSerialNumber(args.text()) ? 0 : 1); });
		action("hardware_test.touched",
			   "The screen was tapped during the touch test: record where (`clicked`)",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto point = args.point())
				   {
					   touched(point->x, point->y);
				   }
			   });
		action("hardware_test.start_colours",
			   "Fill the screen with each test colour in turn",
			   [this](const Xml::ActionArgs&) { startDeadPixelTest(); });
		action("hardware_test.colour_tapped",
			   "The screen was tapped while showing a colour (`clicked`): the right half for no dead pixels, the left "
			   "for some",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto point = args.point();
				   if (point && m_colourShown.get() != 0)
				   {
					   colourChecked(point->x > lv_display_get_horizontal_resolution(nullptr) / 2);
				   }
			   });
		action("hardware_test.usb_continue",
			   "The user has done what the USB test asked",
			   [this](const Xml::ActionArgs&)
			   {
				   if (auto next = std::move(m_onUsbButton))
				   {
					   m_onUsbButton = nullptr;
					   next(*this);
				   }
			   });
		action("hardware_test.play_buzzer", "Sound the buzzer again", [this](const Xml::ActionArgs&) { playBuzzer(); });
		action("hardware_test.buzzer_heard",
			   "Whether the buzzer was heard: `arg` 1 for yes, 0 for no",
			   [this](const Xml::ActionArgs& args)
			   { soundChecked(TestId::BuzzerTest, args.argInt().value_or(0) != 0); });
		action("hardware_test.play_speaker",
			   "Play a tone on the speaker; then it can be said whether it was heard",
			   [this](const Xml::ActionArgs&)
			   {
				   playSpeaker();
				   m_speakerAnswerable.set(1);
			   });
		action("hardware_test.speaker_heard",
			   "Whether the speaker was heard: `arg` 1 for yes, 0 for no",
			   [this](const Xml::ActionArgs& args)
			   { soundChecked(TestId::SpeakerTest, args.argInt().value_or(0) != 0); });
		action("hardware_test.restart",
			   "Run every test again, from the serial number",
			   [this](const Xml::ActionArgs&) { restartTests(); });
		action("hardware_test.exit",
			   "Leave the hardware test",
			   [this](const Xml::ActionArgs&)
			   {
				   if (m_onExit)
				   {
					   m_onExit();
				   }
			   });
	}

	void HardwareTestPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("stage",
				m_stage,
				"The stage shown: `serial`, `touch`, `colours`, `command`, `usb`, `buzzer`, `speaker` or `results`");
		f.field("version", m_version, "The firmware version and build date");
		f.field("serial", m_serial, "The serial number to start with");
		f.field("serial_invalid", m_serialInvalid, "1 if the serial number typed isn't valid");
		f.field("target_x", m_targetX, "Left of the touch target, to be centred on the point to tap");
		f.field("target_y", m_targetY, "Top of the touch target");
		f.field("colour_shown", m_colourShown, "1 while the screen is filled with a test colour");
		f.field("colour", m_colour, "The test colour");
		f.field("message", m_message, "What the memory, WiFi, USB or speaker test is doing or asks for");
		f.field("output", m_output, "What the memory, WiFi or speaker test's commands wrote");
		f.field("usb_drives", m_usbDrives, "The mounted USB drives and the one in use");
		f.field("usb_button_shown", m_usbButtonShown, "1 while the USB test waits for the user");
		f.field("usb_button", m_usbButton, "What the USB test's button says");
		f.field("speaker_answerable", m_speakerAnswerable, "1 once the speaker has played, so it can be answered");
		f.list("passed", m_passed, "The tests that passed, each with `name` and `output`");
		f.list("failed", m_failed, "The tests that failed, each with `name` and `output`");
		f.field("exit_shown", m_exitShown, "1 if the results can be left with Exit");
	}

	void HardwareTestPresenter::setOnExit(std::function<void()> callback)
	{
		m_onExit = std::move(callback);
		m_exitShown.set(m_onExit ? 1 : 0);
	}

	void HardwareTestPresenter::startWithoutSerialNumber()
	{
		m_serialNumber = "unknown";
		createLogFile();
		nextTest();
	}

	void HardwareTestPresenter::onInit()
	{
		ZoneScoped;
		readUid();
		std::filesystem::create_directories(LOG_FOLDER);

		// The drive in use: the one mounted now, then as drives come and go
		registerEventListener<EventType::UsbDrive>(this, &HardwareTestPresenter::usbDriveChanged);
		const auto mounts = USB::UsbMonitor::getInstance().getMountedDrives();
		m_usbMountPath = mounts.empty() ? std::string() : mounts.front();

		addTest(
			TestId::TouchCalibration,
			[this](TestProcedure&) { startTouchTest(); },
			[this](TestProcedure& test) { return judgeTouch(test); });
		addTest(
			TestId::DeadPixelTest,
			[this](TestProcedure&) { showStage("colours"); },
			[](TestProcedure& test)
			{
				for (const auto& colour : test.output["color_test"])
				{
					if (!colour["result"].get<bool>())
					{
						return false;
					}
				}
				return true;
			});
		addTest(
			TestId::MemoryTest,
			[this](TestProcedure&) { testMemory(); },
			[](TestProcedure& test) { return test.output.value("result", false); });
		addTest(
			TestId::WifiTest,
			[this](TestProcedure&) { testWifi(); },
			// `mac_address` is missing if WiFi couldn't be turned on
			[](TestProcedure& test) { return !test.output.value("mac_address", std::string()).empty(); });
		addTest(
			TestId::UsbTest,
			[this](TestProcedure&) { testUsb(); },
			[](TestProcedure& test)
			{
				bool passed = true;
				std::vector<std::string> lsusbOutputs;
				for (const char* key : {"test_start", "usb_a", "usb_c_side", "usb_c_rear", "usb_c_device"})
				{
					if (!test.output.contains(key))
					{
						continue;
					}
					const auto& value = test.output[key];
					if (value.contains("result"))
					{
						passed = passed && value["result"].get<bool>();
					}
					else
					{
						LOG_ERROR("Missing 'result' for {}", key);
					}
					if (value.contains("lsusb"))
					{
						lsusbOutputs.push_back(value["lsusb"].get<std::string>());
					}
					else
					{
						LOG_ERROR("Missing 'lsusb' for {}", key);
					}
					if (value.contains("error_message"))
					{
						test.output["error_message"] = value["error_message"].get<std::string>();
					}
				}

				// Each port must change what is on the bus
				std::sort(lsusbOutputs.begin(), lsusbOutputs.end());
				const bool unique = std::adjacent_find(lsusbOutputs.begin(), lsusbOutputs.end()) == lsusbOutputs.end();
				if (!unique)
				{
					passed = false;
					LOG_ERROR("Duplicate lsusb output detected");
					test.output["error_message"] = "Duplicate lsusb output detected";
				}
				test.output["lsusb_unique"] = unique;
				test.output["result"] = passed;
				return passed;
			});
		addTest(
			TestId::BuzzerTest,
			[this](TestProcedure&)
			{
				showStage("buzzer");
				playBuzzer();
			},
			[](TestProcedure& test) { return test.output.value("result", false); });
		addTest(
			TestId::SpeakerTest,
			[this](TestProcedure&) { testSpeaker(); },
			[](TestProcedure& test) { return test.output.value("result", false); });
	}

	void HardwareTestPresenter::onActivate()
	{
		ZoneScoped;
		if (isActive())
		{
			return; // Reconnecting to the printer: carry on
		}
		m_usbMode = Comm::getUsbMode();
		showUsbDrives();
		restartTests();
	}

	void HardwareTestPresenter::onDeactivate()
	{
		ZoneScoped;
		Comm::setUsbMode(m_usbMode);
	}

	bool HardwareTestPresenter::setSerialNumber(std::string_view serialNumber)
	{
		ZoneScoped;
		static const std::regex serialRegex("^A[C-Z]-[0-9]{2}-[0-9]{2}_[0-9]{4}$");
		if (!std::regex_match(serialNumber.begin(), serialNumber.end(), serialRegex))
		{
			return false;
		}
		m_serialNumber = serialNumber;
		createLogFile();
		nextTest();
		return true;
	}

	void HardwareTestPresenter::restartTests()
	{
		ZoneScoped;
		m_serialNumber.clear();
		m_testIndex = 0;
		m_currentTest = nullptr;

		// Targets in each corner and the middle of the screen
		const int32_t width = lv_display_get_horizontal_resolution(nullptr);
		const int32_t height = lv_display_get_vertical_resolution(nullptr);
		const int32_t left = TARGET_INSET;
		const int32_t right = width - TARGET_INSET;
		const int32_t top = TARGET_INSET;
		const int32_t bottom = height - TARGET_INSET;
		m_touchPoints = {{{left, top}, {0, 0}},
						 {{right, top}, {0, 0}},
						 {{left, bottom}, {0, 0}},
						 {{right, bottom}, {0, 0}},
						 {{width / 2, height / 2}, {0, 0}}};

		m_serial.set(HARDWARE_TEST_SERIAL_NUMBER);
		m_serialInvalid.set(0);
		m_colourShown.set(0);
		m_speakerAnswerable.set(0);
		m_usbButtonShown.set(0);
		m_onUsbButton = nullptr;
		showStage("serial");
	}

	void HardwareTestPresenter::testFinished(TestId id)
	{
		ZoneScoped;
		if (m_currentTest == nullptr)
		{
			LOG_ERROR("Unexpected test finished: no current test");
			return;
		}
		if (m_currentTest->getId() != id)
		{
			LOG_ERROR("Unexpected test finished: expected {:s}, got {:s}",
					  nameof::nameof_enum(m_currentTest->getId()),
					  nameof::nameof_enum(id));
			return;
		}

		const bool passed = m_currentTest->finish();
		writeToLogFile(fmt::format("Test '{:s}' {:s}:\n{:s}",
								   nameof::nameof_enum(id),
								   passed ? "passed" : "failed",
								   dump(m_currentTest->output)));
		m_currentTest->cleanup();
		m_testIndex++;
		nextTest();
	}

	void HardwareTestPresenter::nextTest()
	{
		ZoneScoped;
		if (m_testIndex < m_tests.size())
		{
			m_currentTest = &m_tests[m_testIndex];
			writeToLogFile("\n----------------------\n");
			writeToLogFile(fmt::format("Starting test '{:s}'", nameof::nameof_enum(m_currentTest->getId())));
			m_currentTest->start();
			return;
		}

		// All done: the results
		m_currentTest = nullptr;
		std::vector<const TestProcedure*> passed;
		std::vector<const TestProcedure*> failed;
		std::string summary = "\n----------------------\n\nTest summary:";
		for (const auto& test : m_tests)
		{
			(test.getFailed() ? failed : passed).push_back(&test);
			summary += fmt::format(
				"\n  {:s}: {:s}", nameof::nameof_enum(test.getId()), test.getFailed() ? "FAILED" : "passed");
		}
		auto show = [](Xml::List<TestResultRow>& list, const std::vector<const TestProcedure*>& tests)
		{
			list.sync(tests.size(),
					  [&tests](TestResultRow& row, size_t i)
					  {
						  row.name.set(nameof::nameof_enum(tests[i]->getId()));
						  row.output.set(dump(tests[i]->output));
						  row.passed.set(tests[i]->getFailed() ? 0 : 1);
					  });
		};
		show(m_passed, passed);
		show(m_failed, failed);
		showStage("results");
		writeToLogFile(summary);
	}

	void HardwareTestPresenter::showStage(std::string_view stage)
	{
		m_stage.set(stage);
	}

	bool HardwareTestPresenter::running(TestId id) const
	{
		return m_currentTest != nullptr && m_currentTest->getId() == id;
	}

	void HardwareTestPresenter::later(const std::weak_ptr<XmlPresenter>& handle,
									  const std::function<void(HardwareTestPresenter&)>& step)
	{
		if (auto presenter = handle.lock())
		{
			UI_LOCK();
			step(static_cast<HardwareTestPresenter&>(*presenter));
		}
	}

	void HardwareTestPresenter::addTest(TestId id,
										std::function<void(TestProcedure&)> start,
										std::function<bool(TestProcedure&)> judge,
										std::function<void(TestProcedure&)> cleanup)
	{
		m_tests.emplace_back(id, std::move(start), std::move(judge), std::move(cleanup));
	}

	// Touch

	void HardwareTestPresenter::startTouchTest()
	{
		ZoneScoped;
		showStage("touch");
		m_touchIndex = 0;
		showNextTarget();
	}

	void HardwareTestPresenter::showNextTarget()
	{
		if (m_touchIndex >= m_touchPoints.size())
		{
			testFinished(TestId::TouchCalibration);
			return;
		}
		// The target is drawn centred on the point (touch_target.png is 50 pixels square)
		const lv_point_t& target = m_touchPoints[m_touchIndex].target;
		m_targetX.set(target.x - 25);
		m_targetY.set(target.y - 25);
	}

	void HardwareTestPresenter::touched(int32_t x, int32_t y)
	{
		ZoneScoped;
		if (!running(TestId::TouchCalibration) || m_touchIndex >= m_touchPoints.size())
		{
			return;
		}
		auto& point = m_touchPoints[m_touchIndex];
		point.recorded = {x, y};
		m_currentTest->output["touch_point"][m_touchIndex] = {
			{"target", {{"x", point.target.x}, {"y", point.target.y}}},
			{"recorded", {{"x", x}, {"y", y}}},
		};
		m_touchIndex++;
		showNextTarget();
	}

	bool HardwareTestPresenter::judgeTouch(TestProcedure& test)
	{
		bool passed = true;
		for (size_t i = 0; i < m_touchPoints.size(); ++i)
		{
			const auto& [target, recorded] = m_touchPoints[i];
			test.output["touch_point"][i] = {
				{"target", {{"x", target.x}, {"y", target.y}}},
				{"recorded", {{"x", recorded.x}, {"y", recorded.y}}},
			};
			passed = passed && std::abs(target.x - recorded.x) <= TOUCH_TOLERANCE &&
					 std::abs(target.y - recorded.y) <= TOUCH_TOLERANCE;
		}
		return passed;
	}

	// Dead pixels

	void HardwareTestPresenter::startDeadPixelTest()
	{
		m_colourIndex = 0;
		m_colourShown.set(1);
		showNextColour();
	}

	void HardwareTestPresenter::showNextColour()
	{
		if (m_colourIndex >= COLOURS.size())
		{
			m_colourShown.set(0);
			testFinished(TestId::DeadPixelTest);
			return;
		}
		m_colour.set(COLOURS[m_colourIndex].colour);
	}

	void HardwareTestPresenter::colourChecked(bool passed)
	{
		ZoneScoped;
		if (!running(TestId::DeadPixelTest) || m_colourIndex >= COLOURS.size())
		{
			return;
		}
		const TestColour& colour = COLOURS[m_colourIndex];
		const std::string hex =
			fmt::format("0x{:02x}{:02x}{:02x}", colour.colour.red, colour.colour.green, colour.colour.blue);
		LOG_INFO("Dead pixel check for colour '{:s}' ({:s}) - {:s}", colour.name, hex, passed ? "Passed" : "Failed");
		m_currentTest->output["color_test"][m_colourIndex] = {
			{"color", hex},
			{"name", colour.name},
			{"result", passed},
		};
		m_colourIndex++;
		showNextColour();
	}

	// Memory and WiFi

	void HardwareTestPresenter::testMemory()
	{
		ZoneScoped;
		showStage("command");
		m_message.set("Running memory test...");
		setOutput("");

		std::thread(
			[handle = handle()]()
			{
				// Every partition but ubi, which manages its own bad blocks
				std::string result;
				bool passed = true;
				for ([[maybe_unused]] const auto& [key, partition] :
					 {std::pair{"bootloader", 0}, std::pair{"dtb", 1}, std::pair{"optee", 2}, std::pair{"kernel", 3}})
				{
#if SIMULATION
					const std::string command = fmt::format("echo 'Test {:s} partition'", key);
#else
					const std::string command = fmt::format("nandtest -k /dev/mtd{:d}", partition);
#endif
					FILE* pipe = ::popen(command.c_str(), "r");
					if (pipe == nullptr)
					{
						passed = false;
						continue;
					}
					char buffer[256];
					while (std::fgets(buffer, sizeof(buffer), pipe))
					{
						result.append(buffer);
						later(handle, [&result](HardwareTestPresenter& self) { self.setOutput(result); });
					}
					::pclose(pipe);
					passed = passed && result.find("Finished pass 1 successfully") != std::string::npos;
					later(handle,
						  [key, &result](HardwareTestPresenter& self)
						  {
							  if (self.running(TestId::MemoryTest))
							  {
								  self.m_currentTest->output[key]["result"] = result;
							  }
						  });
				}
				std::this_thread::sleep_for(SETTLE);
				later(handle,
					  [passed](HardwareTestPresenter& self)
					  {
						  if (!self.running(TestId::MemoryTest))
						  {
							  return;
						  }
						  self.m_currentTest->output["result"] = passed;
						  self.testFinished(TestId::MemoryTest);
					  });
			})
			.detach();
	}

	void HardwareTestPresenter::testWifi()
	{
		ZoneScoped;
		showStage("command");
		m_message.set("Running internal WiFi test...");
		setOutput("Setting USB-C MUX to use internal WiFi...\n");
		Comm::setUsbMode(Comm::UsbMode::InternalWiFi);

		addOutput("Enabling internal WiFi...\n");
		NetworkHelper::enable(
			true,
			[self = handle()](bool success)
			{
				if (!success)
				{
					later(self,
						  [](HardwareTestPresenter& presenter)
						  {
							  presenter.addOutput("WiFi enable failed!\n");
							  presenter.testFinished(TestId::WifiTest);
						  });
					return;
				}
				later(self, [](HardwareTestPresenter& presenter) { presenter.addOutput("WiFi enabled.\n"); });

#if SIMULATION
				const std::string result = "1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000\n"
										   "    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00\n"
										   "2: wlan0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq qlen 1000\n"
										   "    link/ether 00:e0:20:2f:87:9d brd ff:ff:ff:ff:ff:ff\n";
#else
				const std::string result = runCommand("ip link");
#endif
				later(self,
					  [&result](HardwareTestPresenter& presenter)
					  {
						  presenter.addOutput(result);
						  if (!presenter.running(TestId::WifiTest))
						  {
							  return;
						  }
						  presenter.m_currentTest->output["result"] = result;
						  presenter.m_currentTest->output["mac_address"] = extractMacAddress(result);
					  });
				std::this_thread::sleep_for(SETTLE);
				later(self, [](HardwareTestPresenter& presenter) { presenter.testFinished(TestId::WifiTest); });
			});
	}

	void HardwareTestPresenter::setOutput(std::string_view text)
	{
		m_output.set(text);
	}

	void HardwareTestPresenter::addOutput(std::string_view text)
	{
		m_output.set(std::string(m_output.get()) + std::string(text));
	}

	// USB

	void HardwareTestPresenter::testUsb()
	{
		ZoneScoped;
		Comm::setUsbMode(Comm::UsbMode::Host);
		showUsbDrives();
		showStage("usb");
		askUsb("Please disconnect all USB flash drives",
			   "Continue",
			   "test_start",
			   false,
			   [](HardwareTestPresenter& self)
			   {
				   self.askUsb("Connect a USB flash drive to the USB-A port and press the button below when ready.",
							   "Device connected",
							   "usb_a",
							   true,
							   [](HardwareTestPresenter& self)
							   {
								   self.askUsb("Connect a USB flash drive to the SIDE USB-C port and press the button "
											   "below when ready.",
											   "Device connected",
											   "usb_c_side",
											   true,
											   [](HardwareTestPresenter& self)
											   {
												   self.askUsb("Connect a USB flash drive to the REAR USB-C port and "
															   "press the button below when ready.",
															   "Device connected",
															   "usb_c_rear",
															   true,
															   [](HardwareTestPresenter& self)
															   { self.askUsbDevice(); });
											   });
							   });
			   });
	}

	void HardwareTestPresenter::askUsb(std::string_view message,
									   std::string_view button,
									   std::string key,
									   bool devicePresent,
									   std::function<void(HardwareTestPresenter&)> next)
	{
		m_message.set(message);
		m_usbButton.set(button);
		m_usbButtonShown.set(1);
		m_onUsbButton = [key = std::move(key), devicePresent, next = std::move(next)](HardwareTestPresenter& self)
		{
			self.m_usbButtonShown.set(0);
			self.m_message.set("Please wait...");
			std::thread(
				[handle = self.handle(), key, devicePresent, next]()
				{
					std::this_thread::sleep_for(SETTLE); // So the USB monitor has looked at least once
					later(handle,
						  [&key, devicePresent, &next](HardwareTestPresenter& presenter)
						  {
							  if (!presenter.running(TestId::UsbTest))
							  {
								  return;
							  }
							  presenter.logUsbData(key, devicePresent);
							  next(presenter);
						  });
				})
				.detach();
		};
	}

	void HardwareTestPresenter::askUsbDevice()
	{
		Comm::setUsbMode(Comm::UsbMode::Device);
#if DEVELOPER_MODE // Enabled with DUETSCREEN_HARDWARE_TEST
		SystemHelper::startService(SystemHelper::Services::ADB);
#endif
		m_message.set("Connect USB-A port to either USB-C port");
		m_usbButton.set("Device connected");
		m_usbButtonShown.set(1);
		m_onUsbButton = [](HardwareTestPresenter& self)
		{
			self.m_usbButtonShown.set(0);
			self.m_message.set("Please wait...");
			std::thread(
				[handle = self.handle()]()
				{
					std::this_thread::sleep_for(SETTLE);
					const std::string lsusb = runCommand("lsusb");
					const std::string dmesg = runCommand("dmesg | grep -E 'usb|ehci'");
					later(handle,
						  [&lsusb, &dmesg](HardwareTestPresenter& presenter)
						  {
							  if (!presenter.running(TestId::UsbTest))
							  {
								  return;
							  }
							  auto& output = presenter.m_currentTest->output;
							  // The screen as a USB device, which needs ADB running
							  const bool found = lsusb.find("1d6b:0105") != std::string::npos;
							  output["usb_c_device"]["lsusb"] = lsusb;
							  output["usb_c_device"]["result"] = found;
							  if (!found)
							  {
								  output["usb_c_device"]["error_message"] = "USB-C device mode not detected";
							  }
							  output["dmesg"] = dmesg;
							  presenter.testFinished(TestId::UsbTest);
						  });
				})
				.detach();
		};
	}

	void HardwareTestPresenter::logUsbData(const std::string& key, bool devicePresent)
	{
		ZoneScoped;
		if (!running(TestId::UsbTest))
		{
			return;
		}
		LOG_INFO("Logging USB data");
		auto& output = m_currentTest->output[key];
		const auto mountedDrives = USB::UsbMonitor::getInstance().getMountedDrives();
		output["lsusb"] = runCommand("lsusb");
		output["mounted_drives"] = mountedDrives;
		output["target_mount"] = m_usbMountPath;

		if (!devicePresent)
		{
			const bool none = mountedDrives.empty();
			output["result"] = none;
			if (!none)
			{
				output["error_message"] = "No USB device should be connected";
			}
			return;
		}

		auto fail = [&output](std::string_view why)
		{
			output["write_successful"] = false;
			output["error_message"] = why;
			output["result"] = false;
		};
		if (m_usbMountPath.empty())
		{
			LOG_INFO("No USB drive mounted");
			fail("No USB drive detected");
			return;
		}

		// Write a file and read it back
		const std::string file = fmt::format("{:s}/{:s}_{:s}.txt", m_usbMountPath, m_uid, key);
		std::ofstream out(file);
		if (!out)
		{
			LOG_ERROR("Failed to open log file");
			writeToLogFile("Failed to open log file");
			fail("Failed to open log file");
			return;
		}
		writeToLogFile(fmt::format("Writing test file to USB drive: {:s}", file));
		const std::string text = fmt::format("DuetScreen USB test file\n{:s}\n", m_uid);
		out << text;
		out.close();
		output["write_successful"] = true;
		output["written_contents"] = text;

		std::string contents;
		USB::ReadFileContents(file, contents);
		output["read_contents"] = contents;
		output["result"] = contents == text;
	}

	void HardwareTestPresenter::showUsbDrives()
	{
		const auto& mounts = USB::UsbMonitor::getInstance().getMountedDrives();
		m_usbDrives.set(fmt::format("Mounted drives:\n  {:s}\nTarget drive: {:s}",
#ifdef __APPLE__
									"",
#else
									fmt::join(mounts, "\n  "),
#endif
									m_usbMountPath));
	}

	void HardwareTestPresenter::usbDriveChanged(const std::string& path, bool mounted)
	{
		ZoneScoped;
		if (mounted)
		{
			m_usbMountPath = path;
		}
		else
		{
			// Another drive, if one is still mounted
			const auto mounts = USB::UsbMonitor::getInstance().getMountedDrives();
			m_usbMountPath = mounts.empty() ? std::string() : mounts.front();
		}
		LOG_INFO("USB drive in use: '{:s}'", m_usbMountPath);
		showUsbDrives();
	}

	// Buzzer and speaker

	void HardwareTestPresenter::playBuzzer()
	{
		ZoneScoped;
		if (std::system("beep 100") != 0 && running(TestId::BuzzerTest))
		{
			m_currentTest->output["result"] = false;
			m_currentTest->output["error_message"] = "Failed to run `beep`";
			testFinished(TestId::BuzzerTest);
		}
	}

	void HardwareTestPresenter::testSpeaker()
	{
		ZoneScoped;
		showStage("speaker");
		m_speakerAnswerable.set(0);
		m_message.set("Please connect a speaker to the screen.");
		setOutput("Configuring audio output...\n");
		for (const char* command : {"amixer sset 'Headphone' on",
									"amixer sset 'Headphone volume' 6",
									"amixer sset 'DAC volume' 170,170",
									"amixer sset 'digital volume' 100"})
		{
			addOutput(fmt::format("> {:s}\n", command));
			addOutput(runCommand(command));
		}
	}

	void HardwareTestPresenter::playSpeaker()
	{
		ZoneScoped;
		if (std::system("speaker-test -c 1 -t sine -f 500 -l 1") != 0 && running(TestId::SpeakerTest))
		{
			m_currentTest->output["result"] = false;
			m_currentTest->output["error_message"] = "Failed to run `speaker-test`";
			testFinished(TestId::SpeakerTest);
		}
	}

	void HardwareTestPresenter::soundChecked(TestId id, bool heard)
	{
		if (!running(id))
		{
			return;
		}
		m_currentTest->output["result"] = heard;
		if (!heard)
		{
			m_currentTest->output["error_message"] = "Sound not heard";
		}
		testFinished(id);
	}

	// Log

	void HardwareTestPresenter::readUid()
	{
		ZoneScoped;
		const std::string dumped =
#if SIMULATION
			"\n0x03006200: 0x93406000 0x4c004814 0x01070a31 0x5c4d1c54";
#else
			runCommand("echo 0x03006200,0x0300620F > /sys/class/sunxi_dump/dump; cat /sys/class/sunxi_dump/dump");
#endif
		// As 93406000_4c004814_01070a31_5c4d1c54
		static const std::regex uidRegex(
			R"(0x03006200:\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+))");
		std::smatch match;
		if (!std::regex_search(dumped, match, uidRegex) || match.size() != 5)
		{
			LOG_FATAL_THROW("Failed to parse UID");
		}
		m_uid = fmt::format("{:s}_{:s}_{:s}_{:s}",
							match[1].str().substr(2),
							match[2].str().substr(2),
							match[3].str().substr(2),
							match[4].str().substr(2));
	}

	void HardwareTestPresenter::createLogFile()
	{
		ZoneScoped;
		std::string path = fmt::format("{:s}{:s}_{:s}.log", LOG_FOLDER, m_serialNumber, m_uid);
#if SIMULATION
		std::filesystem::remove(path);
#endif
		for (size_t number = 1; std::filesystem::exists(path) && number < 10'000; number++)
		{
			path = fmt::format("{:s}{:s}_{:s}_{:04d}.log", LOG_FOLDER, m_serialNumber, m_uid, number);
		}

		std::ofstream out(path);
		if (!out)
		{
			LOG_FATAL_THROW("Failed to create log file");
		}
		m_logFile = path;
		out << "DuetScreen Hardware Test Log - " << m_uid << std::endl;
		out << "Firmware version: " << FIRMWARE_VERSION << std::endl;
		out << "Build date: " << BuildDateText << " " << BuildTimeSuffix << std::endl;
		out.close();

		writeToLogFile(runCommand("dmesg"));
	}

	bool HardwareTestPresenter::writeToLogFile(const std::string& message)
	{
		ZoneScoped;
		if (m_logFile.empty())
		{
			LOG_ERROR("Log file not created");
			return false;
		}
		LOG_INFO("{:s}", message);

		// Written from the UI thread and the tests' threads
		static std::mutex s_logMutex;
		std::lock_guard<std::mutex> lock(s_logMutex);

		FILE* out = std::fopen(m_logFile.c_str(), "a");
		if (out == nullptr)
		{
			LOG_FATAL_THROW("Failed to open log file");
			return false;
		}
		std::fputs(message.c_str(), out);
		std::fputc('\n', out);
		std::fflush(out);
		// The log is on the FAT /boot partition, which is often powered off without being unmounted: without a sync
		// its tail stays in the page cache and is lost
		if (::fsync(::fileno(out)) != 0)
		{
			LOG_ERROR("Failed to sync log file: {:s}", std::strerror(errno));
		}
		std::fclose(out);
		return true;
	}
} // namespace UI

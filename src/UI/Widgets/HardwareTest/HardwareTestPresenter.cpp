/*
 * HardwareTestPresenter.cpp
 *
 *  Created on: 2025-08-22
 *      Author: Andy Everitt
 */

#include "HardwareTestPresenter.h"
#include "BuildDate.h"
#include "Debug.h"
#include "Hardware/Usb.h"
#include "HardwareTest.h"
#include "UI/Screens/Home/HomeView.h"
#include "nameof.hpp"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include "version.h"
#include <filesystem>
#include <fstream>
#include <regex>

namespace UI
{
#if SIMULATION
#  define FOLDER "./logs/"
#else
#  define FOLDER "/boot/logs/"
#endif

#define ENABLE_TOUCH_TEST 1
#define ENABLE_PIXEL_TEST 1
#define ENABLE_MEMORY_TEST 1
#define ENABLE_WIFI_TEST 1
#define ENABLE_USB_TEST 1
#define ENABLE_BUZZER_TEST 1
#define ENABLE_SPEAKER_TEST 1

	static std::string runCommand(const std::string& cmd)
	{
		ZoneScoped;
		std::string result;
		FILE* pipe = ::popen(cmd.c_str(), "r");
		if (!pipe)
			return result;
		char buffer[256];
		while (fgets(buffer, sizeof(buffer), pipe))
		{
			result.append(buffer);
		}
		::pclose(pipe);
		return result;
	}

	void TestProcedure::start()
	{
		ZoneScoped;
		output.clear();
		state = TestState::InProgress;
		// Call the start callback if it exists
		if (start_cb)
		{
			start_cb(*this);
		}
	}
	bool TestProcedure::finish()
	{
		ZoneScoped;
		bool success = true;
		if (finish_cb)
		{
			success = finish_cb(*this);
			state = success ? TestState::Completed : TestState::Failed;
			failed = !success;
		}
		else
		{
			state = TestState::Completed;
		}
		return success;
	}
	void TestProcedure::cleanup()
	{
		ZoneScoped;
		if (cleanup_cb)
		{
			cleanup_cb(*this);
		}
	}

	bool HardwareTestPresenter::setSerialNumber(std::string_view serial_number)
	{
		ZoneScoped;
		// Validate the serial number (example: must be 10 characters long and alphanumeric)
		std::regex serial_regex("^A[C-Z]-[0-9]{2}-[0-9]{2}_[0-9]{4}$");
		if (!std::regex_match(serial_number.begin(), serial_number.end(), serial_regex))
		{
			return false; // Invalid serial number
		}

		m_serialNumber = serial_number;

		createLogFile();

		// Proceed to the next test
		nextTest();

		return true; // Valid serial number
	}

	void HardwareTestPresenter::startTouchCalibration()
	{
		ZoneScoped;
		// Start the touch calibration process
		auto& touchScreenTest = getView()->getTouchScreenTest();
		getView()->showTest(&touchScreenTest);

		m_touchPointIndex = 0;
		showNextTouchPoint();
	}

	void HardwareTestPresenter::touchCalibrationFinished()
	{
		ZoneScoped;

		// Finish the touch calibration process
		nextTest();
	}

	void HardwareTestPresenter::showNextTouchPoint()
	{
		ZoneScoped;
		if (m_touchPointIndex >= m_touchPoints.size())
		{
			testFinished(TestId::TouchCalibration);
			return;
		}

		auto& touchScreenTest = getView()->getTouchScreenTest();
		lv_point_t point = m_touchPoints[m_touchPointIndex].first;
		touchScreenTest.setTouchTargetPosition(point.x, point.y);
	}

	void HardwareTestPresenter::logTouchEvent(int32_t x, int32_t y)
	{
		ZoneScoped;
		if (m_touchPointIndex >= m_touchPoints.size())
		{
			// Not in calibration mode
			return;
		}

		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::TouchCalibration)
		{
			// Not in calibration mode
			return;
		}

		const lv_point_t& target = m_touchPoints[m_touchPointIndex].first;

		m_currentTest->output["touch_point"][m_touchPointIndex]["target"] = {{"x", target.x}, {"y", target.y}};
		m_currentTest->output["touch_point"][m_touchPointIndex]["recorded"] = {{"x", x}, {"y", y}};

		lv_point_t& recorded = m_touchPoints[m_touchPointIndex].second;
		recorded = {x, y};

		m_touchPointIndex++;

		showNextTouchPoint();
	}

	bool HardwareTestPresenter::checkTouchCalibration(TestProcedure& test)
	{
		ZoneScoped;
		const int32_t tolerance = 50; // pixels
		bool success = true;
		for (size_t i = 0; i < m_touchPoints.size(); ++i)
		{
			const lv_point_t& target = m_touchPoints[i].first;
			const lv_point_t& recorded = m_touchPoints[i].second;

			test.output["touch_point"][i] = {
				{"target", {{"x", target.x}, {"y", target.y}}},
				{"recorded", {{"x", recorded.x}, {"y", recorded.y}}},
			};

			if (std::abs(target.x - recorded.x) > tolerance || std::abs(target.y - recorded.y) > tolerance)
			{
				success = false;
			}
		}
		return success;
	}

	void HardwareTestPresenter::startDeadPixelTest()
	{
		ZoneScoped;
		m_colorIndex = 0;
		nextColor();
	}

	void HardwareTestPresenter::nextColor()
	{
		ZoneScoped;
		auto& deadPixelTest = getView()->getDeadPixelTest();

		if (m_colorIndex >= m_colors.size())
		{
			testFinished(TestId::DeadPixelTest);
		}

		lv_color_t color = m_colors[m_colorIndex].color;
		deadPixelTest.setScreenColour(color.red, color.green, color.blue);
	}

	void HardwareTestPresenter::deadPixelCheckPassed(bool passed)
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::DeadPixelTest)
		{
			// Not in dead pixel test
			return;
		}

		color_test_t color_test = m_colors[m_colorIndex];
		LOG_INFO("Dead pixel check for color '{:s}' (0x{:02x}{:02x}{:02x}) - {:s}",
				 color_test.name,
				 color_test.color.red,
				 color_test.color.green,
				 color_test.color.blue,
				 passed ? "Passed" : "Failed");

		m_currentTest->output["color_test"][m_colorIndex] = {
			{"color",
			 fmt::format("0x{:02x}{:02x}{:02x}", color_test.color.red, color_test.color.green, color_test.color.blue)},
			{"name", color_test.name},
			{"result", passed},
		};

		m_colorIndex++;
		nextColor();
	}

	void HardwareTestPresenter::testMemory()
	{
		ZoneScoped;
		auto& commandTest = getView()->getCommandTest();
		getView()->showTest(&commandTest);
		commandTest.setMessage("Running memory test...");

		std::thread(
			[this, &commandTest]()
			{
				std::string result;

				bool passed = true;
				// Test bootloader partition
				std::string cmd =
#if SIMULATION
					"echo 'Test bootloader partition'";
#else
					"nandtest -k /dev/mtd0";
#endif
				FILE* pipe = ::popen(cmd.c_str(), "r");
				if (!pipe)
					return;
				char buffer[256];
				while (fgets(buffer, sizeof(buffer), pipe))
				{
					result.append(buffer);
					commandTest.setOutput(result);
				}
				::pclose(pipe);

				m_currentTest->output["bootloader"]["result"] = result;
				passed = passed && result.find("Finished pass 1 successfully") != std::string::npos;

				// Test dtb partition
				cmd =
#if SIMULATION
					"echo 'Test dtb partition'";
#else
					"nandtest -k /dev/mtd1";
#endif
				pipe = ::popen(cmd.c_str(), "r");
				if (!pipe)
					return;
				while (fgets(buffer, sizeof(buffer), pipe))
				{
					result.append(buffer);
					commandTest.setOutput(result);
				}
				::pclose(pipe);

				m_currentTest->output["dtb"]["result"] = result;
				passed = passed && result.find("Finished pass 1 successfully") != std::string::npos;

				// Test optee partition
				cmd =
#if SIMULATION
					"echo 'Test optee partition'";
#else
					"nandtest -k /dev/mtd2";
#endif
				pipe = ::popen(cmd.c_str(), "r");
				if (!pipe)
					return;
				while (fgets(buffer, sizeof(buffer), pipe))
				{
					result.append(buffer);
					commandTest.setOutput(result);
				}
				::pclose(pipe);

				m_currentTest->output["optee"]["result"] = result;
				passed = passed && result.find("Finished pass 1 successfully") != std::string::npos;

				// Test kernel partition
				cmd =
#if SIMULATION
					"echo 'Test kernel partition'";
#else
					"nandtest -k /dev/mtd3";
#endif
				pipe = ::popen(cmd.c_str(), "r");
				if (!pipe)
					return;
				while (fgets(buffer, sizeof(buffer), pipe))
				{
					result.append(buffer);
					commandTest.setOutput(result);
				}
				::pclose(pipe);

				m_currentTest->output["kernel"]["result"] = result;
				passed = passed && result.find("Finished pass 1 successfully") != std::string::npos;

				// Don't test ubi partition, that one has BBM

				m_currentTest->output["result"] = passed;

				std::this_thread::sleep_for(std::chrono::seconds(2));
				testFinished(TestId::MemoryTest);
			})
			.detach();
	}

	static std::string extractMacAddress(const std::string& input)
	{
		ZoneScoped;
		std::regex mac_regex("link/ether ((?:[0-9a-fA-F]{2}[:-]){5}(?:[0-9a-fA-F]{2}))");
		std::smatch match;
		if (std::regex_search(input, match, mac_regex))
		{
			return match.str(1);
		}
		return "";
	}

	void HardwareTestPresenter::testWifi()
	{
		ZoneScoped;
		auto& commandTest = getView()->getCommandTest();
		getView()->showTest(&commandTest);
		commandTest.setMessage("Running internal WiFi test...");

		commandTest.setOutput("Setting USB-C MUX to use internal WiFi...\n");
		setUsbMode(Comm::UsbMode::InternalWiFi);

		commandTest.appendOutput("Enabling internal WiFi...\n");
		NetworkHelper::enable(true,
							  [this, &commandTest](bool success)
							  {
								  if (!success)
								  {
									  commandTest.appendOutput("WiFi enable failed!\n");
									  testFinished(TestId::WifiTest);
									  return;
								  }
								  commandTest.appendOutput("WiFi enabled.\n");

								  std::string result;
								  std::string cmd =
#if SIMULATION
									  "echo '1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue qlen 1000\n"
									  "    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00\n"
									  "2: wlan0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq qlen 1000\n"
									  "    link/ether 00:e0:20:2f:87:9d brd ff:ff:ff:ff:ff:ff'";
#else
					"ip link";
#endif
								  FILE* pipe = ::popen(cmd.c_str(), "r");
								  if (!pipe)
								  {
									  commandTest.appendOutput("Failed to run `ip link`!\n");
									  testFinished(TestId::WifiTest);
									  return;
								  }
								  char buffer[256];
								  while (fgets(buffer, sizeof(buffer), pipe))
								  {
									  result.append(buffer);
									  commandTest.appendOutput(buffer);
								  }
								  ::pclose(pipe);

								  m_currentTest->output["result"] = result;
								  m_currentTest->output["mac_address"] = extractMacAddress(result);

								  std::this_thread::sleep_for(std::chrono::seconds(2));
								  testFinished(TestId::WifiTest);
							  });
	}

	void HardwareTestPresenter::testUsb()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			// Not in USB-A test
			LOG_ERROR("Not in USB test");
			return;
		}

		updateUsbMounts();

		auto& usbTest = getView()->getUsbTest();

		setUsbMode(Comm::UsbMode::Host);

		getView()->showTest(&usbTest);
		usbTest.setMessage("Please disconnect all USB flash drives");
		usbTest.setButtonText("Continue");
		usbTest.showButton(true);
		usbTest.setButtonCallback(
			[this, &usbTest]()
			{
				usbTest.showButton(false);
				usbTest.setMessage("Please wait...");

				std::thread(
					[this]()
					{
						std::this_thread::sleep_for(
							std::chrono::seconds(2)); // ensure the UsbMonitor thread has run at least once
						logUsbData("test_start", false);
						promptUsbAConnect();
					})
					.detach();
			});
	}

	void HardwareTestPresenter::promptUsbAConnect()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			// Not in USB-A test
			LOG_ERROR("Not in USB test");
			return;
		}

		auto& usbTest = getView()->getUsbTest();

		usbTest.setMessage("Connect a USB flash drive to the USB-A port and press the button below when ready.");
		usbTest.showButton(true);
		usbTest.setButtonCallback(
			[this, &usbTest]
			{
				usbTest.showButton(false);
				usbTest.setMessage("Please wait...");

				std::thread(
					[this]()
					{
						std::this_thread::sleep_for(
							std::chrono::seconds(2)); // ensure the UsbMonitor thread has run at least once
						logUsbData("usb_a", true);
						promptUsbCConnect();
					})
					.detach();
			});
	}

	void HardwareTestPresenter::promptUsbCConnect()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			// Not in USB-C test
			LOG_ERROR("Not in USB test");
			return;
		}

		auto& usbTest = getView()->getUsbTest();

		usbTest.setMessage("Connect a USB flash drive to the SIDE USB-C port and press the button below when ready.");
		usbTest.showButton(true);
		usbTest.setButtonCallback(
			[this, &usbTest]
			{
				usbTest.showButton(false);
				usbTest.setMessage("Please wait...");

				std::thread(
					[this]()
					{
						std::this_thread::sleep_for(
							std::chrono::seconds(2)); // ensure the UsbMonitor thread has run at least once
						logUsbData("usb_c_side", true);
						promptUsbCConnect2();
					})
					.detach();
			});
	}

	void HardwareTestPresenter::promptUsbCConnect2()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			// Not in USB-C test
			LOG_ERROR("Not in USB test");
			return;
		}

		auto& usbTest = getView()->getUsbTest();

		usbTest.setMessage("Connect a USB flash drive to the REAR USB-C port and press the button below when ready.");
		usbTest.showButton(true);
		usbTest.setButtonCallback(
			[this, &usbTest]()
			{
				usbTest.showButton(false);
				usbTest.setMessage("Please wait...");

				std::thread(
					[this]()
					{
						std::this_thread::sleep_for(
							std::chrono::seconds(2)); // ensure the UsbMonitor thread has run at least once
						logUsbData("usb_c_rear", true);

						promptUsbDeviceConnect();
					})
					.detach();
			});
	}

	void HardwareTestPresenter::promptUsbDeviceConnect()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			LOG_ERROR("Not in USB test");
			return;
		}

		setUsbMode(Comm::UsbMode::Device);

#if DEVELOPER_MODE // Enabled when DUETSCREEN_HARDWARE_TEST is enabled
		SystemHelper::startService(SystemHelper::Services::ADB);
#endif

		auto& usbTest = getView()->getUsbTest();
		usbTest.setMessage("Connect USB-A port to either USB-C port");
		usbTest.showButton(true);
		usbTest.setButtonCallback(
			[this, &usbTest]()
			{
				usbTest.showButton(false);
				usbTest.setMessage("Please wait...");

				std::thread(
					[this]()
					{
						std::this_thread::sleep_for(
							std::chrono::seconds(2)); // ensure the UsbMonitor thread has run at least once

						std::string lsusb = runCommand("lsusb");
						m_currentTest->output["usb_c_device"]["lsusb"] = lsusb;

						/* This requires ADB to be enabled */
						bool result = lsusb.find("1d6b:0105") != std::string::npos;

						m_currentTest->output["usb_c_device"]["result"] = result;
						if (!result)
						{
							m_currentTest->output["usb_c_device"]["error_message"] = "USB-C device mode not detected";
						}
						std::string dmesg = runCommand("dmesg | grep -E 'usb|ehci'");
						m_currentTest->output["dmesg"] = dmesg;
						testFinished(TestId::UsbTest);
					})
					.detach();
			});
	}

	void HardwareTestPresenter::logUsbData(std::string_view key, bool device_present)
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::UsbTest)
		{
			// Not in USB-A test
			return;
		}
		LOG_INFO("Logging USB data");

		std::string lsusb = runCommand("lsusb");
		auto mounted_drives = USB::UsbMonitor::getInstance().getMountedDrives();

		m_currentTest->output[key]["lsusb"] = lsusb;
		m_currentTest->output[key]["mounted_drives"] = mounted_drives;
		m_currentTest->output[key]["target_mount"] = m_usbMountPath;

		if (!device_present)
		{
			bool result = mounted_drives.size() == 0;
			m_currentTest->output[key]["result"] = result;
			if (!result)
			{
				m_currentTest->output[key]["error_message"] = "No USB device should be connected";
			}
			return;
		}

		if (m_usbMountPath.empty())
		{
			LOG_INFO("No USB drive mounted");
			m_currentTest->output[key]["write_successful"] = false;
			m_currentTest->output[key]["error_message"] = "No USB drive detected";
			m_currentTest->output[key]["result"] = false;
			return;
		}

		std::string file = fmt::format("{:s}/{:s}_{:s}.txt", m_usbMountPath, m_uid, key);
		std::ofstream out(file);
		if (!out)
		{
			LOG_ERROR("Failed to open log file");
			writeToLogFile("Failed to open log file");
			m_currentTest->output[key]["write_successful"] = false;
			m_currentTest->output[key]["error_message"] = "Failed to open log file";
			m_currentTest->output[key]["result"] = false;
			return;
		}

		writeToLogFile(fmt::format("Writing test file to USB drive: {:s}", file));

		std::string text = fmt::format("DuetScreen USB test file\n{:s}\n", m_uid);
		out << text;
		out.close();
		m_currentTest->output[key]["write_successful"] = true;
		m_currentTest->output[key]["written_contents"] = text;

		std::string contents;
		USB::ReadFileContents(file, contents);

		m_currentTest->output[key]["read_contents"] = contents;

		m_currentTest->output[key]["result"] = contents == text;
	}

	void HardwareTestPresenter::updateUsbMounts()
	{
		ZoneScoped;

		const auto& mounts = USB::UsbMonitor::getInstance().getMountedDrives();
		std::string mount_str = fmt::format("Mounted drives:\n  {:s}\nTarget drive: {:s}",
#ifdef __APPLE__
											""
#else
											fmt::join(mounts, "\n  ")
#endif
											,
											m_usbMountPath);
		getView()->getUsbTest().setOutput(mount_str);
	}

	void HardwareTestPresenter::playBuzzer()
	{
		ZoneScoped;
		if (system("beep 100") != 0)
		{
			m_currentTest->output["result"] = false;
			m_currentTest->output["error_message"] = "Failed to run `beep`";
			testFinished(TestId::BuzzerTest);
		}
	}

	void HardwareTestPresenter::buzzerCheckPassed(bool passed)
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::BuzzerTest)
		{
			// Not in speaker test
			return;
		}

		m_currentTest->output["result"] = passed;
		if (!passed)
		{
			m_currentTest->output["error_message"] = "Sound not heard";
		}
		testFinished(TestId::BuzzerTest);
	}

	void HardwareTestPresenter::testSpeaker()
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::SpeakerTest)
		{
			LOG_ERROR("Not in Speaker test");
			return;
		}

		auto& speakerTest = getView()->getSpeakerTest();

		getView()->showTest(&speakerTest);

		speakerTest.setMessage("Please connect a speaker to the screen.");

		speakerTest.setOutput("Configuring audio output...\n");
		speakerTest.appendOutput("> amixer sset 'Headphone' on\n");
		speakerTest.appendOutput(runCommand("amixer sset 'Headphone' on\n"));

		speakerTest.appendOutput("> amixer sset 'Headphone volume' 6\n");
		speakerTest.appendOutput(runCommand("amixer sset 'Headphone volume' 6\n"));

		speakerTest.appendOutput("> amixer sset 'DAC volume' 170,170\n");
		speakerTest.appendOutput(runCommand("amixer sset 'DAC volume' 170,170\n"));

		speakerTest.appendOutput("> amixer sset 'digital volume' 100\n");
		speakerTest.appendOutput(runCommand("amixer sset 'digital volume' 100\n"));
	}

	void HardwareTestPresenter::playSpeaker()
	{
		ZoneScoped;
		if (system("speaker-test -c 1 -t sine -f 500 -l 1") != 0)
		{
			m_currentTest->output["result"] = false;
			m_currentTest->output["error_message"] = "Failed to run `speaker-test`";
			testFinished(TestId::SpeakerTest);
		}
	}

	void HardwareTestPresenter::speakerCheckPassed(bool passed)
	{
		ZoneScoped;
		if (m_currentTest == nullptr || m_currentTest->getId() != TestId::SpeakerTest)
		{
			// Not in speaker test
			return;
		}

		m_currentTest->output["result"] = passed;
		if (!passed)
		{
			m_currentTest->output["error_message"] = "Sound not heard";
		}
		testFinished(TestId::SpeakerTest);
	}

	void HardwareTestPresenter::restartTests()
	{
		ZoneScoped;
		m_serialNumber.clear();
		m_testIndex = 0;

		getView()->updateLayout();
		const int32_t x_res = getView()->getWidth();
		const int32_t y_res = getView()->getHeight();

		const int32_t x_min = 30;
		const int32_t x_max = x_res - 30;
		const int32_t y_min = 30;
		const int32_t y_max = y_res - 30;

		m_touchPoints = {{{x_min, y_min}, {0, 0}},
						 {{x_max, y_min}, {0, 0}},
						 {{x_min, y_max}, {0, 0}},
						 {{x_max, y_max}, {0, 0}},
						 {{x_res / 2, y_res / 2}, {0, 0}}};

		for (size_t i = 0; i < m_colors.size(); ++i)
		{
			m_colors[i].result = false;
		}

		getView()->showTest(&getView()->getSerialInput());
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

		bool passed = m_currentTest->finish();
		writeToLogFile(fmt::format("Test '{:s}' {:s}:\n{:s}",
								   nameof::nameof_enum(id),
								   passed ? "passed" : "failed",
								   m_currentTest->getOutput().dump(4)));

		m_currentTest->cleanup();

		m_testIndex++;
		nextTest();
	}

	void HardwareTestPresenter::nextTest()
	{
		ZoneScoped;
		if (m_testIndex >= m_tests.size())
		{
			auto& testResults = getView()->getTestResults();
			testResults.clearResults();

			for (size_t i = 0; i < m_tests.size(); ++i)
			{
				auto& test = m_tests[i];
				testResults.addResult(nameof::nameof_enum(test.getId()), test.getOutput().dump(4), !test.getFailed());
			}
			getView()->showResults();
			return;
		}

		TestProcedure* test = &m_tests[m_testIndex];
		m_currentTest = test;
		writeToLogFile("\n----------------------\n");
		writeToLogFile(fmt::format("Starting test '{:s}'", nameof::nameof_enum(test->getId())));
		test->start();
	}

	void HardwareTestPresenter::getUid()
	{
		ZoneScoped;
		std::string uid =
#if SIMULATION
			"\n0x03006200: 0x93406000 0x4c004814 0x01070a31 0x5c4d1c54";
#else
			runCommand("echo 0x03006200,0x0300620F > /sys/class/sunxi_dump/dump; cat /sys/class/sunxi_dump/dump");
#endif

		// Extract and reformat UID to the desired format: 93406000_4c004814_01070a31_5c4d1c54
		std::smatch match;
		std::regex uid_regex(
			R"(0x03006200:\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+)\s*(0x[0-9a-fA-F]+))");
		if (std::regex_search(uid, match, uid_regex) && match.size() == 5)
		{
			m_uid = fmt::format("{:s}_{:s}_{:s}_{:s}",
								match[1].str().substr(2),
								match[2].str().substr(2),
								match[3].str().substr(2),
								match[4].str().substr(2));
		}
		else
		{
			LOG_FATAL_THROW("Failed to parse UID");
		}
	}

	void HardwareTestPresenter::createLogFile()
	{
		ZoneScoped;
		std::string filePath = fmt::format(FOLDER "{:s}_{:s}.log", m_serialNumber, m_uid);

#if SIMULATION
		std::filesystem::remove(filePath);
#endif

		size_t fileNum = 1;
		while (std::filesystem::exists(filePath) && fileNum < 10'000)
		{
			filePath = fmt::format(FOLDER "{:s}_{:s}_{:04d}.log", m_serialNumber, m_uid, fileNum);
			fileNum++;
		}

		std::ofstream out(filePath);
		if (!out)
		{
			LOG_FATAL_THROW("Failed to create log file");
		}

		m_logFile = filePath;

		out << "DuetScreen Hardware Test Log - " << m_uid << std::endl;
		out << "Firmware version: " << FIRMWARE_VERSION << std::endl;
		out << "Build date: " << BuildDateText << " " << BuildTimeSuffix << std::endl;

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
		std::ofstream out(m_logFile, std::ios::app);
		if (!out)
		{
			LOG_FATAL_THROW("Failed to open log file");
			return false;
		}
		out << message << std::endl;
		return true;
	}

	void HardwareTestPresenter::createTestProcedure(TestId id,
													std::function<void(TestProcedure& test)> start_cb,
													std::function<bool(TestProcedure& test)> finish_cb,
													std::function<void(TestProcedure& test)> cleanup_cb)
	{
		ZoneScoped;
		m_tests.emplace_back(id, start_cb, finish_cb, cleanup_cb);
	}

	void HardwareTestPresenter::onInit()
	{
		ZoneScoped;
		getUid();
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		std::filesystem::create_directories(FOLDER);

		USB::UsbMonitor::getInstance().registerCallback(
			[this](const std::string& path, bool mounted)
			{
				const auto& mounts = USB::UsbMonitor::getInstance().getMountedDrives();
				if (mounted)
				{
					m_usbMountPath = path;
				}
				else
				{
					if (mounts.empty())
					{
						LOG_INFO("USB drive unmounted");
						m_usbMountPath.clear();
						goto update_view;
					}
					m_usbMountPath = mounts.front();
				}
				LOG_INFO("USB drive mounted at: {:s}", m_usbMountPath);
			update_view:
				updateUsbMounts();
			},
			true);

/* Create test procedures */
#if ENABLE_TOUCH_TEST
		createTestProcedure(
			TestId::TouchCalibration,
			[this](TestProcedure& /* test */) { startTouchCalibration(); },
			[this](TestProcedure& test) { return checkTouchCalibration(test); });
#endif

#if ENABLE_PIXEL_TEST
		createTestProcedure(
			TestId::DeadPixelTest,
			[this](TestProcedure& /* test */) { getView()->showTest(&getView()->getDeadPixelTest()); },
			[this](TestProcedure& test)
			{
				for (size_t i = 0; i < test.output["color_test"].size(); ++i)
				{
					if (!test.output["color_test"][i]["result"].get<bool>())
					{
						return false;
					}
				}
				return true;
			});
#endif

#if ENABLE_MEMORY_TEST
		createTestProcedure(
			TestId::MemoryTest,
			[this](TestProcedure& /* test */) { testMemory(); },
			[this](TestProcedure& test)
			{
				bool passed = false;
				if (test.output.contains("result"))
				{
					passed = test.output["result"].get<bool>();
				}

				return passed;
			});
#endif

#if ENABLE_WIFI_TEST
		createTestProcedure(
			TestId::WifiTest,
			[this](TestProcedure& /* test */) { testWifi(); },
			[this](TestProcedure& test)
			{
				// `mac_address` is missing if enabling WiFi failed
				std::string mac_addr = test.output.value("mac_address", "");

				// Check `result` for `Finished pass 1 successfully`
				return !mac_addr.empty();
			});
#endif

#if ENABLE_USB_TEST
		createTestProcedure(
			TestId::UsbTest,
			[this](TestProcedure& /* test */) { testUsb(); },
			[this](TestProcedure& test)
			{
				bool passed = true;
				std::string prev_lsusb;
				auto keys = {"test_start", "usb_a", "usb_c_side", "usb_c_rear", "usb_c_device"};
				std::vector<std::string> lsusb_outputs;
				for (const auto& key : keys)
				{
					if (!test.output.contains(key))
					{
						continue;
					}

					auto& value = test.output[key];
					if (value.contains("result"))
					{
						passed &= value["result"].get<bool>();
					}
					else
					{
						LOG_ERROR("Missing 'result' for {}", key);
					}

					if (value.contains("lsusb"))
					{
						std::string lsusb = value["lsusb"].get<std::string>();
						lsusb_outputs.push_back(lsusb);
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

				{
					/* Ensure all lsusb outputs are unique */
					bool lsusb_unique = true;
					for (size_t i = 0; i < lsusb_outputs.size() && lsusb_unique; ++i)
					{
						for (size_t j = i + 1; j < lsusb_outputs.size(); ++j)
						{
							if (lsusb_outputs[i] == lsusb_outputs[j])
							{
								lsusb_unique = false;
								break;
							}
						}
					}
					if (!lsusb_unique)
					{
						passed = false;
						LOG_ERROR("Duplicate lsusb output detected");
						test.output["error_message"] = "Duplicate lsusb output detected";
					}
					test.output["lsusb_unique"] = lsusb_unique;
					test.output["result"] = passed;
				}

				return passed;
			});
#endif

#if ENABLE_BUZZER_TEST
		createTestProcedure(
			TestId::BuzzerTest,
			[this](TestProcedure& /* test */)
			{
				getView()->showTest(&getView()->getBuzzerTest());
				playBuzzer();
			},
			[this](TestProcedure& test)
			{ return test.output.contains("result") && test.output["result"].get<bool>(); });
#endif

#if ENABLE_SPEAKER_TEST
		createTestProcedure(
			TestId::SpeakerTest,
			[this](TestProcedure& /* test */) { testSpeaker(); },
			[this](TestProcedure& test)
			{ return test.output.contains("result") && test.output["result"].get<bool>(); });
#endif
	}

	void HardwareTestPresenter::onActivate()
	{
		ZoneScoped;
		if (isActive())
			return;

		m_usbMode = Comm::getUsbMode();
		updateUsbMounts();
		restartTests();
	}

	void HardwareTestPresenter::onDeactivate()
	{
		ZoneScoped;
		Comm::setUsbMode(m_usbMode);
	}
} // namespace UI

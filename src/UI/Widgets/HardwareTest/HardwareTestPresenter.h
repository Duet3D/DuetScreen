/*
 * HardwareTestPresenter.h
 *
 *  Created on: 2025-08-22
 *      Author: Andy Everitt
 */

#pragma once

#include "Comm/Usb.h"
#include "UI/Core/Presenter.h"
#include <nlohmann/json.hpp>

namespace UI
{
	class HardwareTest;

	enum class TestId
	{
		TouchCalibration,
		DeadPixelTest,
		MemoryTest,
		WifiTest,
		UsbTest,
		BuzzerTest,
		SpeakerTest,
	};

	enum class TestState
	{
		NotStarted,
		InProgress,
		Completed,
		Failed
	};

	class TestProcedure
	{
	  public:
		TestProcedure(TestId id,
					  std::function<void(TestProcedure& test)> start_cb,
					  std::function<bool(TestProcedure& test)> finish_cb,
					  std::function<void(TestProcedure& test)> cleanup_cb)
			: id(id)
			, start_cb(start_cb)
			, finish_cb(finish_cb)
			, cleanup_cb(cleanup_cb)
		{
		}

		TestId getId() const { return id; }
		const nlohmann::json& getOutput() const { return output; }
		void start();
		bool finish();
		void cleanup();
		bool getFailed() const { return failed; }

		nlohmann::json output; // JSON output for the test

	  private:
		TestId id;
		TestState state = TestState::NotStarted;
		std::function<void(TestProcedure& test)> start_cb;	 // Called when starting the test
		std::function<bool(TestProcedure& test)> finish_cb;	 // Called to finish the test, returns true if successful
		std::function<void(TestProcedure& test)> cleanup_cb; // Called before starting the next test
		bool failed = false;
	};

	class HardwareTestPresenter : public Presenter<HardwareTest>
	{
	  public:
		friend class DeveloperSettingsPresenter;

		PRESENTER_CONSTRUCTOR(HardwareTestPresenter, HardwareTest);

		// Setters
		bool setSerialNumber(std::string_view serial_number);

		// Getters

		// Actions
		void restartTests();
		void testFinished(TestId id);

		void startTouchCalibration();
		void touchCalibrationFinished();
		void logTouchEvent(int32_t x, int32_t y);

		void startDeadPixelTest();
		void nextColor();
		void deadPixelCheckPassed(bool passed);

		void testMemory();
		void testWifi();

		void testUsb();
		void logUsbData(std::string_view key, bool device_present);
		void promptUsbAConnect();
		void promptUsbCConnect();
		void promptUsbCConnect2();
		void promptUsbDeviceConnect();

		void playBuzzer();
		void buzzerCheckPassed(bool passed);

		void testSpeaker();
		void playSpeaker();
		void speakerCheckPassed(bool passed);

		// Observers

	  protected:
		void onInit() override;
		void onActivate() override;
		void onDeactivate() override;

		void onConnect() override {}
		void onDisconnect() override {}

		void nextTest();

		void createTestProcedure(TestId id,
								 std::function<void(TestProcedure& test)> start_cb,
								 std::function<bool(TestProcedure& test)> finish_cb,
								 std::function<void(TestProcedure& test)> cleanup_cb = nullptr);
		void getUid();
		void createLogFile();
		bool writeToLogFile(const std::string& message);
		void showNextTouchPoint();
		bool checkTouchCalibration(TestProcedure& test);
		void updateUsbMounts();

		/** A USB drive was mounted or removed: use it, or another one still mounted */
		void usbDriveChanged(const std::string& path, bool mounted);

	  private:
		std::string m_uid;
		std::string m_serialNumber;
		std::string m_logFile;

		std::vector<TestProcedure> m_tests;
		TestProcedure* m_currentTest = nullptr;
		size_t m_testIndex = 0;

		size_t m_touchPointIndex = 0;
		std::vector<std::pair<lv_point_t, lv_point_t>> m_touchPoints;

		size_t m_colorIndex = 0;
		struct color_test_t
		{
			lv_color_t color;
			std::string_view name;
			bool result;
		};
		std::vector<color_test_t> m_colors = {{lv_color_hex(0xFF0000), "Red", false},
											  {lv_color_hex(0x00FF00), "Green", false},
											  {lv_color_hex(0x0000FF), "Blue", false},
											  {lv_color_hex(0xFFFFFF), "White", false}};

		std::string m_usbMountPath;

		Comm::UsbMode m_usbMode; // mode to restore once tests finish
	};
} // namespace UI

/*
 * HardwareTestPresenter.h
 *
 * Behaviour for the factory hardware test (assets/ui/components/hardware_test/hardware_test.xml): after a serial
 * number, it runs each test in turn (touch, dead pixels, memory, WiFi, USB, buzzer and speaker), logs every result to
 * a file, and shows a summary. One stage of the screen is shown at a time, named by `stage`.
 *
 * A HARDWARE_TEST build shows it over the home screen from start-up (main.cpp); the Developer settings tab opens it
 * too, skipping the serial number.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "Comm/Usb.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace UI
{
	/** The tests, in the order they run */
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

	/** One test: how it starts, how its result is judged, and what it leaves tidy */
	class TestProcedure
	{
	  public:
		/**
		 * @param start Starts the test; it finishes later, through HardwareTestPresenter::testFinished()
		 * @param judge Says whether the finished test passed, from its output
		 * @param cleanup Run once it has finished, before the next test, if not null
		 */
		TestProcedure(TestId id,
					  std::function<void(TestProcedure&)> start,
					  std::function<bool(TestProcedure&)> judge,
					  std::function<void(TestProcedure&)> cleanup);

		/** Which test it is */
		TestId getId() const { return m_id; }

		/** Forget any earlier output and start the test */
		void start();

		/** Judge the finished test. @return whether it passed */
		bool finish();

		/** Tidy up after the test */
		void cleanup();

		/** Whether it failed when last judged */
		bool getFailed() const { return m_failed; }

		nlohmann::json output; // What the test found, logged and shown in the results

	  private:
		TestId m_id;									// Which test it is
		std::function<void(TestProcedure&)> m_start;	// Starts it
		std::function<bool(TestProcedure&)> m_judge;	// Says whether it passed
		std::function<void(TestProcedure&)> m_cleanup; // Tidies up after it, or null
		bool m_failed = false;							// Failed when last judged
	};

	/** One finished test in the results */
	class TestResultRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<32> name;	  // The test, e.g. "UsbTest"
		Xml::StringSubject<16384> output; // What it found, as JSON; cut short if longer (the log has it all)
		Xml::IntSubject passed;			  // 1 if it passed
	};

	class HardwareTestPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with every test */
		HardwareTestPresenter();

		/** Declare the screen's fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Run `callback` when the user leaves the results with Exit; without one, Exit isn't shown */
		void setOnExit(std::function<void()> callback);

		/** Start the tests without asking for a serial number, logged as "unknown" (the user won't know it) */
		void startWithoutSerialNumber();

	  protected:
		/** Find the screen's ID and the USB drive in use, and create the tests */
		void onInit() override;

		/** Start again from the serial number, and remember the USB mode to put back */
		void onActivate() override;

		/** Put the USB mode back */
		void onDeactivate() override;

	  private:
		/** Use `serialNumber` if it is valid (e.g. "AC-12-34_5678"), and start the tests. @return whether it was */
		bool setSerialNumber(std::string_view serialNumber);

		/** Ask for the serial number again, ready to run every test */
		void restartTests();

		/** Test `id` has finished: judge and log it, and start the next */
		void testFinished(TestId id);

		/** Start the next test, or show the results if there are none left */
		void nextTest();

		/** Show stage `stage` of the screen, e.g. "touch" (see `stage` in fields()) */
		void showStage(std::string_view stage);

		/** Whether test `id` is the one running */
		bool running(TestId id) const;

		/** A handle on the presenter that the tests' threads can hold: it may be gone by the time they finish */
		std::weak_ptr<XmlPresenter> handle() { return weak_from_this(); }

		/**
		 * Run `step` with the presenter `handle` refers to, under the UI lock, if it still exists. For the tests'
		 * threads, which aren't joined, so may take the lock.
		 */
		static void later(const std::weak_ptr<XmlPresenter>& handle,
						  const std::function<void(HardwareTestPresenter&)>& step);

		/** Add a test to run */
		void addTest(TestId id,
					 std::function<void(TestProcedure&)> start,
					 std::function<bool(TestProcedure&)> judge,
					 std::function<void(TestProcedure&)> cleanup = nullptr);

		// Touch
		/** Show each target in turn */
		void startTouchTest();

		/** Show the next target, or finish if all have been tapped */
		void showNextTarget();

		/** The screen was tapped at (`x`, `y`) during the touch test */
		void touched(int32_t x, int32_t y);

		/** Whether every tap was close enough to its target, recording them in `test` */
		bool judgeTouch(TestProcedure& test);

		// Dead pixels
		/** Fill the screen with each colour in turn */
		void startDeadPixelTest();

		/** Show the next colour, or finish if all have been checked */
		void showNextColour();

		/** The user said whether the colour had dead pixels: `passed` if not */
		void colourChecked(bool passed);

		// Memory and WiFi
		/** Check the flash partitions, on a thread of its own */
		void testMemory();

		/** Turn on the internal WiFi and record its MAC address */
		void testWifi();

		/** Show `text` as the command output, replacing what was there */
		void setOutput(std::string_view text);

		/** Add `text` to the command output */
		void addOutput(std::string_view text);

		// USB
		/** Ask for the USB ports to be tried in turn */
		void testUsb();

		/** Ask the user to do `message`, then, when they press `button`, record `key` (on a thread) and go on */
		void askUsb(std::string_view message, std::string_view button, std::string key, bool devicePresent,
					std::function<void(HardwareTestPresenter&)> next);

		/** Ask for the USB-A port to be connected to a USB-C port, in device mode */
		void askUsbDevice();

		/** Record what is on the USB bus under `key`, and that a drive works if `devicePresent` */
		void logUsbData(const std::string& key, bool devicePresent);

		/** Show the mounted drives and the one in use */
		void showUsbDrives();

		/** A USB drive was mounted or removed: use it, or another one still mounted */
		void usbDriveChanged(const std::string& path, bool mounted);

		// Buzzer and speaker
		/** Sound the buzzer */
		void playBuzzer();

		/** Set up the speaker output */
		void testSpeaker();

		/** Play a tone on the speaker */
		void playSpeaker();

		/** The user said whether test `id`'s sound was heard */
		void soundChecked(TestId id, bool heard);

		// Log
		/** Find the screen's unique ID */
		void readUid();

		/** Create the log file, named after the serial number and the ID */
		void createLogFile();

		/** Add `message` to the log file. @return false if it couldn't be written */
		bool writeToLogFile(const std::string& message);

		std::string m_uid;			// The screen's unique ID
		std::string m_serialNumber; // As entered, or "unknown"
		std::string m_logFile;		// Path of the log file

		std::vector<TestProcedure> m_tests;	   // Every test, in order
		TestProcedure* m_currentTest = nullptr; // The test running, if any
		size_t m_testIndex = 0;				   // Index of the next test to start

		/** A target to tap and where it was tapped */
		struct TouchPoint
		{
			lv_point_t target;	 // Centre of the target
			lv_point_t recorded; // Where it was tapped
		};
		std::vector<TouchPoint> m_touchPoints; // The targets, for this screen's size
		size_t m_touchIndex = 0;				 // The target showing

		/** A colour the screen is filled with */
		struct TestColour
		{
			lv_color_t colour;	   // The colour
			std::string_view name; // Its name, for the log
		};
		static const std::vector<TestColour> COLOURS; // The colours, in order
		size_t m_colourIndex = 0;					  // The colour showing

		std::string m_usbMountPath;							  // The USB drive in use, or empty
		std::function<void(HardwareTestPresenter&)> m_onUsbButton; // What the USB button does now
		Comm::UsbMode m_usbMode = Comm::UsbMode::Host;		  // USB mode to put back afterwards
		std::function<void()> m_onExit;						  // Leaves the screen, or empty

		Xml::StringSubject<16> m_stage;			  // `stage`
		Xml::StringSubject<128> m_version;		  // `version`
		Xml::StringSubject<32> m_serial;		  // `serial`
		Xml::IntSubject m_serialInvalid;		  // `serial_invalid`
		Xml::IntSubject m_targetX;				  // `target_x`
		Xml::IntSubject m_targetY;				  // `target_y`
		Xml::IntSubject m_colourShown;			  // `colour_shown`
		Xml::ColorSubject m_colour;				  // `colour`
		Xml::StringSubject<256> m_message;		  // `message`
		Xml::StringSubject<16384> m_output;		  // `output`
		Xml::StringSubject<512> m_usbDrives;	  // `usb_drives`
		Xml::IntSubject m_usbButtonShown;		  // `usb_button_shown`
		Xml::StringSubject<32> m_usbButton;		  // `usb_button`
		Xml::IntSubject m_speakerAnswerable;	  // `speaker_answerable`
		Xml::List<TestResultRow> m_passed;		  // `passed`
		Xml::List<TestResultRow> m_failed;		  // `failed`
		Xml::IntSubject m_exitShown;			  // `exit_shown`
	};
} // namespace UI

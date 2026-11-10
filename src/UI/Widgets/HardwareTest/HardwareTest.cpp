/*
 * HardwareTest.cpp
 *
 *  Created on: 2025-08-22
 *      Author: Andy Everitt
 */

#include "HardwareTest.h"
#include "BuildDate.h"
#include "Debug.h"
#include "i18n/i18n.h"
#include "version.h"

#ifndef HARDWARE_TEST_SERIAL_NUMBER
#  define HARDWARE_TEST_SERIAL_NUMBER ""
#endif

namespace UI
{
	static const lv_color_t BG_COLOR = lv_color_make(0x11, 0x11, 0x11);

	HardwareTest::HardwareTest()
		: View("hardware_test")
	{
		ZoneScoped;
		setStylePad(0, LV_PART_MAIN, Padding::ALL);
		setSize(LV_PCT(100), LV_PCT(100));
		// Don't use plain black to prevent content from fading away (bug in the SUNXI display or DMA/MMU driver).
		// The panel is RGB666, so the lower 2 bits of each channel are dropped: 0x04 is the smallest value that is
		// not output as black (0x01 is truncated to 0 and only occasionally toggled by FRM dithering).
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);
	}

	HardwareTest::SerialInput::SerialInput(HardwareTest& parent)
		: LvContainer("serial_input_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_version.setText(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));

		m_warning.setText("Invalid serial number");
		m_warning.hide();
		m_warning.setWidth(LV_PCT(100));
		m_warning.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
		m_serialInput.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_serialInput.setOneLine(true);
		m_serialInput.setFlexGrow(1);
		m_serialInput.setCursorClickPos(true);
		m_kb.setSize(LV_PCT(100), LV_PCT(50));
		m_kb.setTextarea(&m_serialInput.getTextarea());
		m_kb.setMode(LV_KEYBOARD_MODE_TEXT_UPPER);

		m_kb.addEventCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<HardwareTest::SerialInput*>(lv_event_get_user_data(e));
				std::string_view serial_number = instance->m_serialInput.getText();
				if (!instance->m_parent.getPresenter()->setSerialNumber(serial_number))
				{
					// Handle invalid serial number
					instance->m_warning.show();
				}
			},
			LV_EVENT_READY,
			this);
	}

	void HardwareTest::SerialInput::onShow()
	{
		ZoneScoped;
		m_warning.hide(true);

		m_serialInput.setText(HARDWARE_TEST_SERIAL_NUMBER);
		m_serialInput.setCursorPos(LV_TEXTAREA_CURSOR_LAST);
		m_serialInput.setPlaceholderText("Enter serial number");
	}

	HardwareTest::TouchScreenTest::TouchScreenTest(HardwareTest& parent)
		: LvContainer("touch_screen_test_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgOpa(LV_OPA_COVER);
		setStylePad(0, LV_PART_MAIN, Padding::ALL);

		setSize(LV_PCT(100), LV_PCT(100));

		setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		addEventCallback(onTouchEvent, LV_EVENT_CLICKED, this);

		m_hint.setX(LV_PCT(10));
		m_hint.setText("Tap the target");
		m_target.setSize(50, 50);
		m_target.setSrc(Themes::getFixedIconPath("hardware_test", "touch_target.png").c_str());
		m_target.setStyleRecolor(lv_color_white());
		m_target.setStyleRecolorOpa(LV_OPA_COVER);
		m_target.setFlag(LV_OBJ_FLAG_FLOATING, true);

		m_messageBox.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_messageBox.setTitle("Touch Calibration Results");
		m_messageBox.setCancelBtnText("Restart calibration");
		m_messageBox.cancelVisible(true);
		m_messageBox.setOkBtnText("Continue");
		m_messageBox.setCancelCallback([this]() { m_parent.getPresenter()->startTouchCalibration(); });
		m_messageBox.setOkCallback([this]() { m_parent.getPresenter()->touchCalibrationFinished(); });
	}

	void HardwareTest::TouchScreenTest::setTouchTargetPosition(int32_t x, int32_t y)
	{
		ZoneScoped;
		// Ensure the target is centered on (x, y)
		const lv_coord_t target_width = m_target.getWidth();
		const lv_coord_t target_height = m_target.getHeight();
		const lv_area_t coords = getCoords();
		m_target.setPos(x - target_width / 2 - coords.x1, y - target_height / 2 - coords.y1);
	}

	void HardwareTest::TouchScreenTest::showResults(bool pass, std::string_view message)
	{
		ZoneScoped;
		m_messageBox.setText(message);
		m_messageBox.okVisible(pass);
		m_messageBox.cancelVisible(!pass);
		m_messageBox.show(true);

		setStyleBgColor(pass ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED));
	}

	void HardwareTest::TouchScreenTest::onTouchEvent(lv_event_t* e)
	{
		ZoneScoped;
		auto* instance = static_cast<TouchScreenTest*>(lv_event_get_user_data(e));
		lv_indev_t* indev = lv_event_get_indev(e);

		lv_point_t point;
		lv_indev_get_point(indev, &point);
		instance->m_parent.getPresenter()->logTouchEvent(point.x, point.y);
		return;
	}

	void HardwareTest::TouchScreenTest::onShow()
	{
		ZoneScoped;
		m_messageBox.hide();
		setStyleBgColor(lv_palette_main(LV_PALETTE_BLUE));
	}

	HardwareTest::DeadPixelTest::DeadPixelTest(HardwareTest& parent)
		: LvContainer("dead_pixel_test_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));

		m_hint.setText("Check for dead pixels");

		m_pass.setText("Pass");
		m_fail.setText("Fail");
		m_start.setText("Start");
		m_start.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_start.setSize(LV_PCT(20), LV_PCT(20));

		m_start.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<DeadPixelTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->startDeadPixelTest();
			},
			this);

		m_messageBox.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_messageBox.setTitle("Dead pixel check");
		m_messageBox.cancelVisible(true);
		m_messageBox.okVisible(true);
		m_messageBox.setCancelBtnText("Yes");
		m_messageBox.setOkBtnText("No");
		m_messageBox.setCancelCallback([this]() { m_parent.getPresenter()->deadPixelCheckPassed(false); });
		m_messageBox.setOkCallback([this]() { m_parent.getPresenter()->deadPixelCheckPassed(true); });
		m_messageBox.setText("Are there dead pixels?");

		addEventCallback(onTouchEvent, LV_EVENT_CLICKED, this);
	}

	void HardwareTest::DeadPixelTest::setScreenColour(uint8_t red, uint8_t green, uint8_t blue)
	{
		ZoneScoped;
		setStyleBgColor(lv_color_make(red, green, blue));
		m_hint.hide();
		m_pass.hide();
		m_fail.hide();
		m_start.hide();
		m_divider.hide();
	}

	void HardwareTest::DeadPixelTest::confirmWithUser()
	{
		ZoneScoped;
		m_messageBox.show();
	}

	void HardwareTest::DeadPixelTest::onTouchEvent(lv_event_t* e)
	{
		ZoneScoped;
		auto* instance = static_cast<DeadPixelTest*>(lv_event_get_user_data(e));
		if (instance->m_start.isVisible())
		{
			return;
		}

		lv_point_t point;
		lv_indev_t* indev = lv_event_get_indev(e);
		lv_indev_get_point(indev, &point);
		lv_coord_t width = instance->getWidth();
		bool passed = point.x > width / 2;
		instance->m_parent.getPresenter()->deadPixelCheckPassed(passed);
		return;
	}

	void HardwareTest::DeadPixelTest::onShow()
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		m_hint.show();
		m_pass.show();
		m_fail.show();
		m_start.show();
		m_divider.show();

		const lv_coord_t width = getWidth();
		const lv_coord_t height = getHeight();
		static lv_point_precise_t line_points[] = {{width / 2, 0}, {width / 2, height}};
		lv_line_set_points(m_divider.getRootPtr(), line_points, 2);
		m_pass.setAlign(LV_ALIGN_RIGHT_MID, -width / 4, 0);
		m_fail.setAlign(LV_ALIGN_LEFT_MID, width / 4, 0);

		m_messageBox.hide();
	}

	HardwareTest::CommandTest::CommandTest(HardwareTest& parent)
		: LvContainer("command_test_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_output.setWidth(LV_PCT(70));
		m_output.setFlexGrow(1);
		m_output.setMaxHeight(LV_PCT(70));
		m_output.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		m_output.setOneLine(false);
		m_output.setText("");
	}

	void HardwareTest::CommandTest::setMessage(std::string_view message)
	{
		ZoneScoped;
		m_message.setText(message);
	}

	void HardwareTest::CommandTest::setOutput(const std::string& output)
	{
		ZoneScoped;
		m_output.setText(output);
	}

	void HardwareTest::CommandTest::appendOutput(std::string_view output)
	{
		ZoneScoped;
		m_output.addText(std::string(output));
	}

	HardwareTest::UsbATest::UsbATest(HardwareTest& parent)
		: CommandTest(parent)
	{
		ZoneScoped;
		setMessage("Connect a device to the USB-A port and press the button below when ready.");
		m_button.setText("Device connected");
		m_button.setSize(LV_PCT(30), LV_SIZE_CONTENT);
		m_button.setFlexGrow(0);
		m_button.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<UsbATest*>(lv_event_get_user_data(e));
				if (instance->m_callback)
				{
					instance->m_callback();
				}
			},
			this);
	}

	HardwareTest::BuzzerTest::BuzzerTest(HardwareTest& parent)
		: LvContainer("buzzer_test_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_label.setText("Did you hear a sound?");

		m_buttons.setSize(LV_PCT(50), LV_PCT(50));

		m_no.setText("No");
		m_no.setHeight(LV_PCT(100));
		m_no.setFlexGrow(1);
		m_no.setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
		lv_obj_set_style_bg_grad_dir(m_no.getRootPtr(), LV_GRAD_DIR_NONE, 0);
		m_no.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<BuzzerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->buzzerCheckPassed(false);
			},
			this);

		m_yes.setText("Yes");
		m_yes.setHeight(LV_PCT(100));
		m_yes.setFlexGrow(1);
		m_yes.setStyleBgColor(lv_palette_main(LV_PALETTE_GREEN));
		lv_obj_set_style_bg_grad_dir(m_yes.getRootPtr(), LV_GRAD_DIR_NONE, 0);
		m_yes.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<BuzzerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->buzzerCheckPassed(true);
			},
			this);

		m_playAgain.setText("Play Again");
		m_playAgain.setSize(LV_PCT(30), LV_PCT(20));
		m_playAgain.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<BuzzerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->playBuzzer();
			},
			this);
	}

	HardwareTest::SpeakerTest::SpeakerTest(HardwareTest& parent)
		: CommandTest(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_output.setFlexGrow(3);
		m_buttons.setFlexGrow(1);
		m_buttons.setWidth(LV_PCT(50));
		m_buttons.setState(LV_STATE_DISABLED, true, true);

		m_no.setText("No");
		m_no.setHeight(LV_PCT(100));
		m_no.setFlexGrow(1);
		m_no.setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
		lv_obj_set_style_bg_grad_dir(m_no.getRootPtr(), LV_GRAD_DIR_NONE, 0);
		m_no.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<SpeakerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->speakerCheckPassed(false);
			},
			this);

		m_yes.setText("Yes");
		m_yes.setHeight(LV_PCT(100));
		m_yes.setFlexGrow(1);
		m_yes.setStyleBgColor(lv_palette_main(LV_PALETTE_GREEN));
		lv_obj_set_style_bg_grad_dir(m_yes.getRootPtr(), LV_GRAD_DIR_NONE, 0);
		m_yes.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<SpeakerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->speakerCheckPassed(true);
			},
			this);

		m_playSound.setText("Play Sound");
		m_playSound.setSize(LV_PCT(30), LV_PCT(20));
		m_playSound.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<SpeakerTest*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->playSpeaker();
				instance->m_buttons.setState(LV_STATE_DISABLED, false, true);
			},
			this);
	}

	HardwareTest::TestResults::TestResults(HardwareTest& parent)
		: LvContainer("test_results_container", parent)
		, m_parent(parent)
	{
		ZoneScoped;
		setStyleBgColor(BG_COLOR);
		setStyleBgOpa(LV_OPA_COVER);

		setSize(LV_PCT(100), LV_PCT(100));
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		m_title.setText("Test Results");
		m_title.setAlign(LV_ALIGN_TOP_MID, 0, 0);

		m_version.setText(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));

		m_passed.setTitle("Passed Tests");
		m_passed.setWidth(LV_PCT(100));
		m_passed.setFlexGrow(1);
		m_passed.setListFlow(LV_FLEX_FLOW_ROW);
		m_passed.setListGrow(1);

		m_failed.setTitle("Failed Tests");
		m_failed.setWidth(LV_PCT(100));
		m_failed.setFlexGrow(1);
		m_failed.setListFlow(LV_FLEX_FLOW_ROW);
		m_failed.setListGrow(1);

		m_buttons.setSize(LV_PCT(50), LV_SIZE_CONTENT);
		m_buttons.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_restart.setText("Restart");
		m_exit.setText("Exit");
		m_restart.setFlexGrow(1);
		m_exit.setFlexGrow(1);

#if HARDWARE_TEST
		m_exit.hide();
#endif

		m_restart.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<TestResults*>(lv_event_get_user_data(e));
				instance->m_parent.getPresenter()->restartTests();
			},
			this);

		m_exit.addClickedCallback(
			[](lv_event_t* e)
			{
				auto* instance = static_cast<TestResults*>(lv_event_get_user_data(e));
				instance->m_parent.hide();
			},
			this);
	}

	void HardwareTest::TestResults::clearResults()
	{
		ZoneScoped;
		m_passed.clear();
		m_failed.clear();
	}

	void HardwareTest::TestResults::addResult(std::string_view name, const std::string& output, bool passed)
	{
		ZoneScoped;
		auto& list = passed ? m_passed : m_failed;
		auto& item = list.addItem();
		item.setName(name);
		item.setOutput(output);
		item.setPassed(passed);
	}

	HardwareTest::TestResults::TestResult::TestResult(size_t index, LvObj& parent)
		: ListItem(index, parent)
	{
		ZoneScoped;
		setHeight(LV_PCT(100));
		setFlexGrow(1);
		setMinWidth(LV_PCT(20));
		setStyleBgOpa(LV_OPA_COVER);

		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

		m_output.setWidth(LV_PCT(100));
		m_output.setFlexGrow(1);
		m_output.setOneLine(false);
		// m_output.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
	}

	void HardwareTest::TestResults::TestResult::setName(std::string_view name)
	{
		ZoneScoped;
		m_name.setText(name);
	}

	void HardwareTest::TestResults::TestResult::setOutput(const std::string& output)
	{
		ZoneScoped;
		m_output.setText(output);
	}

	void HardwareTest::TestResults::TestResult::setPassed(bool passed)
	{
		ZoneScoped;
		setStyleBgColor(passed ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED));
	}

	void HardwareTest::showTest(LvContainer* test)
	{
		ZoneScoped;
		m_testResults.hide();
		for (auto* t : m_tests)
		{
			t->setVisible(t == test, true);
		}
	}

	void HardwareTest::showResults()
	{
		ZoneScoped;
		m_testResults.show(true);
	}

	void HardwareTest::onShow()
	{
		ZoneScoped;
		m_serialInput.setVisible(true);
		m_touchScreenTest.setVisible(false);
		m_deadPixelTest.setVisible(false);
		m_testResults.setVisible(false);
	}

} // namespace UI

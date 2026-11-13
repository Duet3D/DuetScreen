/*
 * test_hardware_test.cpp
 *
 * The factory hardware test (assets/ui/components/hardware_test/hardware_test.xml,
 * src/UI/Screens/HardwareTest/HardwareTestPresenter.h): its first stages, driven by taps.
 */

#include "UI/Screens/HardwareTest/HardwareTestPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <memory>

class TestHardwareTest : public UiTestSuite
{
  protected:
	TestHardwareTest()
	{
		presenter->setOnExit([this]() { exited = true; });
		view = std::make_unique<UI::Xml::XmlView>("hardware_test", screen, "hardware_test", presenter);
		view->show(true);
		lv_obj_update_layout(screen.getRootPtr());
	}

	~TestHardwareTest() override { lv_test_mouse_release(); }

	/** The widget called `name` */
	lv_obj_t* part(const char* name) const { return lv_obj_find_by_name(view->getRootPtr(), name); }

	/** Whether the stage called `name` (e.g. "touch_stage") is the one shown */
	bool showing(const char* name) const { return !lv_obj_has_flag(part(name), LV_OBJ_FLAG_HIDDEN); }

	/** Type `serial` and press the keyboard's OK */
	void enterSerial(const char* serial)
	{
		lv_obj_t* input = part("serial_input");
		lv_textarea_set_text(input, serial);
		lv_obj_send_event(input, LV_EVENT_READY, nullptr);
	}

	/** Tap the middle of the touch target */
	void tapTarget()
	{
		lv_obj_update_layout(screen.getRootPtr());
		lv_area_t area;
		lv_obj_get_coords(part("target"), &area);
		lv_test_mouse_click_at((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
	}

	std::shared_ptr<UI::HardwareTestPresenter> presenter =
		std::make_shared<UI::HardwareTestPresenter>(); // Behind the view
	std::unique_ptr<UI::Xml::XmlView> view;			   // The test, over the whole screen
	bool exited = false;							   // Exit was pressed
};

TEST_F(TestHardwareTest, AsksForAValidSerialNumber)
{
	EXPECT_TRUE(showing("serial_stage"));
	EXPECT_EQUAL_SCREENSHOT("hardware_test/serial.png");

	enterSerial("12345");
	EXPECT_TRUE(showing("serial_stage"));
	EXPECT_FALSE(lv_obj_has_flag(part("warning"), LV_OBJ_FLAG_HIDDEN)) << "says it is invalid";

	enterSerial("AC-12-34_5678");
	EXPECT_TRUE(showing("touch_stage")) << "the touch test starts";
}

TEST_F(TestHardwareTest, TouchThenDeadPixels)
{
	enterSerial("AC-12-34_5678");
	ASSERT_TRUE(showing("touch_stage"));
	EXPECT_EQUAL_SCREENSHOT("hardware_test/touch.png");

	// Five targets: the corners, then the middle
	for (int i = 0; i < 5; i++)
	{
		ASSERT_TRUE(showing("touch_stage")) << "target " << i;
		tapTarget();
	}
	ASSERT_TRUE(showing("colours_stage"));
	EXPECT_EQUAL_SCREENSHOT("hardware_test/colours.png");

	// Each colour, with no dead pixels: tap the right half
	lv_obj_send_event(part("start"), LV_EVENT_CLICKED, nullptr);
	for (int i = 0; i < 4; i++)
	{
		lv_obj_t* colour = part("colour");
		ASSERT_FALSE(lv_obj_has_flag(colour, LV_OBJ_FLAG_HIDDEN)) << "colour " << i;
		EXPECT_EQ(lv_obj_get_style_bg_opa(colour, LV_PART_MAIN), LV_OPA_COVER) << "the colour fills the screen";
		if (i == 0)
		{
			EXPECT_EQUAL_SCREENSHOT("hardware_test/red.png");
		}
		lv_test_mouse_click_at(900, 300);
	}
	EXPECT_TRUE(showing("command_stage")) << "the memory test starts";
}

/*
 * test_extruder.cpp
 *
 * The extruder panel (assets/ui/components/extruder.xml, src/UI/Screens/Temperature/ExtruderPresenter.h).
 */

#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <string_view>

class TestExtruder : public UiTestSuite
{
  protected:
	TestExtruder()
	{
		panel.setSize(400, LV_PCT(80));
		panel.show();
	}

	/** The widget called `name` inside the selector called `selector` */
	lv_obj_t* part(const char* selector, const char* name)
	{
		lv_obj_t* found = lv_obj_find_by_name(panel.getRootPtr(), selector);
		return found ? lv_obj_find_by_name(found, name) : nullptr;
	}

	/** The value the selector called `selector` shows */
	std::string_view valueOf(const char* selector) { return lv_textarea_get_text(part(selector, "value")); }

	UI::Xml::XmlView panel{"extruder", screen, "extruder"};
};

TEST_F(TestExtruder, Basic)
{
	EXPECT_EQUAL_SCREENSHOT("extruder/basic.png");
}

TEST_F(TestExtruder, NeedsAToolToExtrude)
{
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(panel.getRootPtr(), "extrude"), LV_STATE_DISABLED));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(panel.getRootPtr(), "retract"), LV_STATE_DISABLED));
}

TEST_F(TestExtruder, SelectorsStepAndPick)
{
	lv_obj_t* presets = part("distance_selector", "list");
	ASSERT_NE(presets, nullptr);
	ASSERT_EQ(lv_obj_get_child_count(presets), 4u);

	lv_obj_send_event(lv_obj_get_child(presets, 2), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(valueOf("distance_selector"), "5");
	lv_obj_send_event(part("distance_selector", "increment"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(valueOf("distance_selector"), "6");
	lv_obj_send_event(part("distance_selector", "decrement"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(part("distance_selector", "decrement"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(valueOf("distance_selector"), "4");
	EXPECT_EQ(valueOf("feedrate_selector"), "0.1") << "each selector has its own value";

	// Put back as it was, for the screenshots of other tests: the value is stored
	for (int i = 0; i < 4; i++)
	{
		lv_obj_send_event(part("distance_selector", "decrement"), LV_EVENT_CLICKED, nullptr);
	}
	EXPECT_EQ(valueOf("distance_selector"), "0.1") << "no lower than the minimum";
}

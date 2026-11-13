/*
 * test_babystep.cpp
 *
 * The babystepping panel (assets/ui/components/common/babystep.xml, src/UI/Screens/Common/BabyStepPresenter.h).
 */

#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

class TestBabyStep : public UiTestSuite
{
  protected:
	TestBabyStep()
	{
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { sent.emplace_back(gcode); });
		panel.setSize(135, 240);
		panel.show();
	}

	~TestBabyStep() override { UI::Xml::Services::setGcodeSender(nullptr); }

	lv_obj_t* part(const char* name) { return lv_obj_find_by_name(panel.getRootPtr(), name); }

	std::vector<std::string> sent;
	UI::Xml::XmlView panel{"babystep", screen, "babystep"};
};

TEST_F(TestBabyStep, Display)
{
	EXPECT_EQUAL_SCREENSHOT("babystep/panel.png");
}

TEST_F(TestBabyStep, StepsByTheChosenSize)
{
	lv_obj_t* steps = part("value_list");
	ASSERT_GE(lv_obj_get_child_count(steps), 2u);
	EXPECT_TRUE(lv_obj_has_state(lv_obj_get_child(steps, 0), LV_STATE_CHECKED)) << "the first is chosen to start with";

	lv_obj_send_event(lv_obj_get_child(steps, 1), LV_EVENT_CLICKED, nullptr);
	EXPECT_FALSE(lv_obj_has_state(lv_obj_get_child(steps, 0), LV_STATE_CHECKED));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_get_child(steps, 1), LV_STATE_CHECKED));

	lv_obj_send_event(part("increment"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(part("decrement"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(part("reset"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(sent, (std::vector<std::string>{"M290 S0.05\n", "M290 S-0.05\n", "M290 R0 S0\n"}));
}

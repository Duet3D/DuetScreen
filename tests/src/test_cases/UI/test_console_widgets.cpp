/*
 * test_console_widgets.cpp
 *
 * The console's pieces placed as widgets of their own: what is typed is the global source.console.input
 * (src/UI/Xml/Data/ConsoleSource.h), so a G-code reference list and a console input placed apart work together.
 */

#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

class TestConsoleWidgets : public UiTestSuite
{
  protected:
	TestConsoleWidgets()
	{
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { sent.emplace_back(gcode); });
		reference.setSize(400, 300);
		input.setSize(400, 60);
		reference.show();
		input.show();
	}

	~TestConsoleWidgets() override { UI::Xml::Services::setGcodeSender(nullptr); }

	/** The text the input shows */
	std::string typed()
	{
		lv_obj_t* box = lv_obj_find_by_name(input.getRootPtr(), "input");
		return box != nullptr ? lv_textarea_get_text(box) : "(no text box)";
	}

	std::vector<std::string> sent;
	UI::Xml::XmlView reference{"reference", screen, "console_command_list"};
	UI::Xml::XmlView input{"input", screen, "console_input"};
};

TEST_F(TestConsoleWidgets, PickingAGcodeTypesItInAnInputPlacedApart)
{
	lv_obj_t* list = lv_obj_find_by_name(reference.getRootPtr(), "list");
	ASSERT_GT(lv_obj_get_child_count(list), 0u);
	lv_obj_t* code = lv_obj_find_by_name(list, "gcode");
	ASSERT_NE(code, nullptr);
	lv_obj_t* first = lv_obj_get_parent(code);
	const std::string gcode = lv_label_get_text(code);
	ASSERT_FALSE(gcode.empty());

	lv_obj_send_event(first, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(typed(), gcode);

	lv_obj_send_event(lv_obj_find_by_name(input.getRootPtr(), "enter"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(sent, (std::vector<std::string>{gcode + "\n"}));
}

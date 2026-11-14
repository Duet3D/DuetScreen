/*
 * test_jog.cpp
 *
 * The jog distance and speed every view shares: the global source.jog.* (src/UI/Xml/Data/JogSource.h), chosen with
 * the selector actions (src/UI/Xml/View/SelectorActions.h).
 */

#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <gtest/gtest.h>

class TestJog : public UiTestSuite
{
  protected:
	/** The `index`th distance choice in `view` */
	static lv_obj_t* distance(UI::Xml::XmlView& view, uint32_t index)
	{
		return lv_obj_get_child(lv_obj_find_by_name(view.getRootPtr(), "distances"), static_cast<int32_t>(index));
	}
};

TEST_F(TestJog, EveryViewSharesTheChosenDistance)
{
	UI::Xml::XmlView a("a", screen, "move_jog_choices");
	UI::Xml::XmlView b("b", screen, "move_jog_choices");
	a.show();
	b.show();
	deliverModelEvents();
	ASSERT_GE(lv_obj_get_child_count(lv_obj_find_by_name(b.getRootPtr(), "distances")), 2u);

	// Chosen in one view…
	UI::Xml::Actions::dispatch(distance(a, 1), "actions.selector.pick", "");

	// …it is chosen in the other, and every jog goes that far
	EXPECT_TRUE(lv_obj_has_state(distance(b, 1), LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(distance(b, 0), LV_STATE_CHECKED));
	ASSERT_TRUE(UI::Xml::JogSource::distance().has_value());
	EXPECT_FLOAT_EQ(*UI::Xml::JogSource::distance(), 0.5f);
}

TEST_F(TestJog, ThePresetsFollowTheUnitsAndTheMachine)
{
	UI::Xml::XmlView view("jog", screen, "move_jog_choices");
	view.show();
	deliverModelEvents();
	lv_obj_t* root = view.getRootPtr();
	const auto first = [root](const char* list)
	{
		lv_obj_t* choice = lv_obj_get_child(lv_obj_find_by_name(root, list), 0);
		return std::string(lv_label_get_text(lv_obj_find_by_name(choice, "label")));
	};
	const auto unit = [root](const char* setting)
	{ return std::string(lv_label_get_text(lv_obj_find_by_name(lv_obj_find_by_name(root, setting), "hint"))); };

	// A printer in millimetres: mm/s, made per minute for the G-code
	EXPECT_EQ(first("distances"), "0.1");
	EXPECT_EQ(unit("feedrate"), "mm/s");
	ASSERT_TRUE(UI::Xml::JogSource::feedrateGcode().has_value());
	EXPECT_FLOAT_EQ(*UI::Xml::JogSource::feedrateGcode(), 50.0f * 60.0f);

	// In inches the presets are inches' own, not millimetres' numbers
	StorageHelper::setData(ID_UNIT_SYSTEM, Units::UnitSystem::Imperial);
	UI::Xml::DataSources::updateAll();
	EXPECT_EQ(first("distances"), "0.001");
	EXPECT_EQ(unit("distance"), "in");
	EXPECT_EQ(unit("feedrate"), "ipm");

	// A CNC machine jogs in mm/min, with presets of its own
	StorageHelper::setData(ID_UNIT_SYSTEM, Units::UnitSystem::Metric);
	load_model_data(R"({"key":"state","flags":"vn","result":{"machineMode":"CNC"}})"
					"\n");
	deliverModelEvents();
	EXPECT_EQ(first("feedrates"), "100");
	EXPECT_EQ(unit("feedrate"), "mm/min");
	EXPECT_FLOAT_EQ(*UI::Xml::JogSource::feedrateGcode(), 1000.0f);

	load_model_data(R"({"key":"state","flags":"vn","result":{"machineMode":"FFF"}})"
					"\n");
	deliverModelEvents();
	EXPECT_EQ(first("feedrates"), "5");
}

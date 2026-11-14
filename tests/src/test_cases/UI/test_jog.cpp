/*
 * test_jog.cpp
 *
 * The jog distance and speed every view shares: the global source.jog.* (src/UI/Xml/Data/JogSource.h), chosen with
 * the selector actions (src/UI/Xml/View/SelectorActions.h).
 */

#include "UI/Core/Model.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
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

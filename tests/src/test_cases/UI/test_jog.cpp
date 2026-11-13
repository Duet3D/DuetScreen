/*
 * test_jog.cpp
 *
 * The jog distance and speed every view shares: the global jog.* (src/UI/Xml/Data/JogSource.h) and the actions that
 * choose them (src/UI/Presenters/Control/Move/JogActions.h).
 */

#include "UI/Core/Model.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

class TestJog : public UiTestSuite
{
  protected:
	/** Deliver queued model events, as the event loop would */
	static void pump()
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}

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
	pump();
	const auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
	ASSERT_GE(distances.size(), 2u);

	// Chosen in one view…
	UI::Xml::Actions::dispatch(distance(a, 1), "move.distance", "");
	pump();

	// …it is chosen in the other, and every jog goes that far
	EXPECT_TRUE(lv_obj_has_state(distance(b, 1), LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(distance(b, 0), LV_STATE_CHECKED));
	ASSERT_TRUE(UI::Xml::JogSource::distance().has_value());
	EXPECT_FLOAT_EQ(*UI::Xml::JogSource::distance(), distances[1]);
}

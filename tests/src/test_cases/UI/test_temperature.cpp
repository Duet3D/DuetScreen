/*
 * test_temperature.cpp
 *
 * The Temperature tab's tool rows (src/UI/Presenters/Control/Temperature/TemperaturePresenter.h).
 */

#include "UI/Core/Model.h"
#include "UI/Presenters/Control/Temperature/TemperaturePresenter.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestTemperature : public UiTestSuite
{
  protected:
	/** Tool 0, which takes filament, with none loaded */
	TestTemperature()
	{
		load_model_data(R"({"key":"tools","flags":"v","result":[{"number":0,"filamentExtruder":0,"heaters":[]}]})"
						"\n");
		load_model_data(R"({"key":"move.extruders","flags":"v","result":[{"filament":"","factor":1}]})"
						"\n");
	}

	/** Show tool 0 in `row`, as the presenter does */
	static void show(UI::TemperatureToolRow& row)
	{
		MODEL_LOCK();
		row.update(0);
	}
};

TEST_F(TestTemperature, AFilamentIsPickedThenLoaded)
{
	UI::TemperatureToolRow row;
	show(row);
	EXPECT_EQ(row.filamentLoaded.get(), 0);
	EXPECT_EQ(row.changeShown.get(), 0) << "nothing picked yet";

	row.pick("PLA");
	EXPECT_EQ(row.filamentText.get(), "PLA");
	EXPECT_EQ(row.changeShown.get(), 1) << "Load is offered";

	show(row);
	EXPECT_EQ(row.filamentText.get(), "PLA") << "the pick lasts until a filament is loaded";

	row.pick("");
	EXPECT_EQ(row.changeShown.get(), 0) << "picking what is loaded offers nothing";
}

/*
 * test_temperature.cpp
 *
 * The Temperature tab's tool rows (src/UI/Screens/Temperature/TemperaturePresenter.h).
 */

#include "ObjectModel/Tool.h"
#include "UI/Screens/Temperature/TemperaturePresenter.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

class TestTemperature : public UiTestSuite
{
  protected:
	TestTemperature() { tool.Reset(); }

	OM::Tool tool; // An empty tool, with no filament loaded
};

TEST_F(TestTemperature, AFilamentIsPickedThenLoaded)
{
	UI::TemperatureToolRow row;
	row.update(tool);
	EXPECT_EQ(row.filamentLoaded.get(), 0);
	EXPECT_EQ(row.changeShown.get(), 0) << "nothing picked yet";

	row.pick("PLA");
	EXPECT_EQ(row.filamentText.get(), "PLA");
	EXPECT_EQ(row.changeShown.get(), 1) << "Load is offered";

	row.update(tool);
	EXPECT_EQ(row.filamentText.get(), "PLA") << "the pick lasts until a filament is loaded";

	row.pick("");
	EXPECT_EQ(row.changeShown.get(), 0) << "picking what is loaded offers nothing";
}

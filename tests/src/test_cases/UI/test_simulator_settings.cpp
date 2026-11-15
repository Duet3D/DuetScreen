/*
 * test_simulator_settings.cpp
 *
 * The simulated Duet's page in the settings (assets/ui/components/settings/settings_simulator*.xml,
 * src/UI/Xml/Data/SimulatorSource.h, src/UI/Presenters/Settings/Simulator*.h): what it shows, and each way its parts
 * are edited, reaching the simulated Duet and, through it, the screen.
 */

#if DUET_SIMULATOR

#  include "ObjectModel/Tree/Tree.h"
#  include "Simulator/Simulator.h"
#  include "UI/Xml/Data/DataSources.h"
#  include "UI/Xml/Data/SettingsSource.h"
#  include "UI/Xml/View/XmlView.h"
#  include "test_utils/Dialogs.h"
#  include "test_utils/UiTestSuite.h"
#  include <gtest/gtest.h>
#  include <string>

class TestSimulatorSettings : public UiTestSuite
{
  protected:
	/** The page, filling the screen */
	TestSimulatorSettings()
	{
		page.setSize(HOR_RES, VER_RES);
		page.show();
		runTimers();
	}

	/** What the simulated Duet is now */
	static Sim::Config config() { return Sim::Simulator::get().config(); }

	/** Change setting `name` to `value` as the UI does */
	static void setSetting(std::string_view name, int32_t value)
	{
		auto* settings = UI::Xml::DataSources::find<UI::Xml::SettingsSource>();
		ASSERT_NE(settings, nullptr);
		ASSERT_TRUE(settings->set(name, static_cast<float>(value))) << name;
	}

	/** The group called `name` on the page */
	lv_obj_t* group(const char* name) { return lv_obj_find_by_name(page.getRootPtr(), name); }

	/** Row `index` of the list in group `name` */
	lv_obj_t* row(const char* name, uint32_t index)
	{
		lv_obj_t* list = lv_obj_find_by_name(group(name), "list");
		return list != nullptr && index < lv_obj_get_child_count(list)
				   ? lv_obj_get_child(list, static_cast<int32_t>(index))
				   : nullptr;
	}

	/** How many rows the list in group `name` has */
	uint32_t rows(const char* name) { return lv_obj_get_child_count(lv_obj_find_by_name(group(name), "list")); }

	/** The widget called `widget` in row `index` of group `name` */
	lv_obj_t* in(const char* name, uint32_t index, const char* widget)
	{
		lv_obj_t* r = row(name, index);
		return r != nullptr ? lv_obj_find_by_name(r, widget) : nullptr;
	}

	/** Tap `obj` */
	static void tap(lv_obj_t* obj)
	{
		ASSERT_NE(obj, nullptr);
		lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
		runTimers();
	}

	/** Type `text` into the text box `obj`, then its keyboard's OK key */
	static void type(lv_obj_t* obj, const char* text)
	{
		ASSERT_NE(obj, nullptr);
		lv_textarea_set_text(obj, text);
		lv_obj_send_event(obj, LV_EVENT_READY, nullptr);
		runTimers();
	}

	/** The text of the label called `name` in `obj` */
	static std::string label(lv_obj_t* obj, const char* name)
	{
		lv_obj_t* l = obj != nullptr ? lv_obj_find_by_name(obj, name) : nullptr;
		return l != nullptr ? lv_label_get_text(l) : "";
	}

	/** Scroll the page to show the group called `name` at its top */
	void scrollTo(const char* name)
	{
		lv_obj_update_layout(page.getRootPtr());
		lv_obj_scroll_to_y(page.getRootPtr(),
						   lv_obj_get_y(group(name)) - lv_obj_get_style_pad_top(page.getRootPtr(), LV_PART_MAIN),
						   LV_ANIM_OFF);
	}

	UI::Xml::XmlView page{"settings_simulator", screen, "settings_simulator"};
};

TEST_F(TestSimulatorSettings, ShowsEveryPart)
{
	setSetting("simulator_preset", 2); // the tool changer: something of everything but a spindle
	runTimers();
	EXPECT_EQUAL_SCREENSHOT("simulator/machine.png")
	scrollTo("connection");
	EXPECT_EQUAL_SCREENSHOT("simulator/connection_messages.png")
	scrollTo("axes");
	EXPECT_EQUAL_SCREENSHOT("simulator/axes_heaters.png")
	scrollTo("extruders");
	EXPECT_EQUAL_SCREENSHOT("simulator/extruders_tools.png")
	scrollTo("tools");
	EXPECT_EQUAL_SCREENSHOT("simulator/tools.png")
	scrollTo("fans");
	EXPECT_EQUAL_SCREENSHOT("simulator/fans_sensors.png")

	setSetting("simulator_preset", 3); // the CNC mill: a spindle, its tool's
	runTimers();
	scrollTo("tools");
	EXPECT_EQUAL_SCREENSHOT("simulator/cnc_tools.png")
	lv_obj_scroll_to_y(page.getRootPtr(), LV_COORD_MAX, LV_ANIM_OFF);
	EXPECT_EQUAL_SCREENSHOT("simulator/cnc_end.png")
}

TEST_F(TestSimulatorSettings, APresetReplacesEveryPart)
{
	EXPECT_EQ(rows("tools"), 1u);
	setSetting("simulator_preset", 2);
	runTimers();
	EXPECT_EQ(config().preset, "tool_changer");
	EXPECT_EQ(rows("tools"), 4u);
	EXPECT_EQ(rows("heaters"), 6u);
	EXPECT_EQ(label(row("heaters", 5), "title"), "Heater 5");
}

TEST_F(TestSimulatorSettings, PartsAreAddedAndRemoved)
{
	tap(lv_obj_find_by_name(lv_obj_find_by_name(group("axes"), "add"), "button"));
	ASSERT_EQ(config().axes.size(), 4u);
	EXPECT_EQ(config().axes[3].letter, "U");
	ASSERT_EQ(rows("axes"), 4u);

	tap(in("axes", 3, "remove"));
	EXPECT_EQ(config().axes.size(), 3u);
	EXPECT_EQ(rows("axes"), 3u);
}

TEST_F(TestSimulatorSettings, RemovingAHeaterRenumbersTheToolsHeaters)
{
	ASSERT_EQ(config().tools[0].heaters, std::vector<int>{1});
	tap(in("heaters", 0, "remove")); // the bed
	EXPECT_EQ(config().heaters.size(), 1u);
	EXPECT_EQ(config().tools[0].heaters, std::vector<int>{0});
}

TEST_F(TestSimulatorSettings, AToolsHeatersAndFansAreChosenWithChips)
{
	lv_obj_t* heaters = in("tools", 0, "heaters");
	ASSERT_EQ(lv_obj_get_child_count(heaters), 2u);
	EXPECT_FALSE(lv_obj_has_state(lv_obj_get_child(heaters, 0), LV_STATE_CHECKED));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_get_child(heaters, 1), LV_STATE_CHECKED));

	tap(lv_obj_get_child(in("tools", 0, "heaters"), 0));
	EXPECT_EQ(config().tools[0].heaters, (std::vector<int>{0, 1}));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_get_child(in("tools", 0, "heaters"), 0), LV_STATE_CHECKED));

	tap(lv_obj_get_child(in("tools", 0, "fans"), 0));
	EXPECT_TRUE(config().tools[0].fans.empty());
}

TEST_F(TestSimulatorSettings, ExtrudersAreAddedAndChosenByTools)
{
	tap(lv_obj_find_by_name(lv_obj_find_by_name(group("extruders"), "add"), "button"));
	ASSERT_EQ(config().extruders.size(), 2u);
	type(in("extruders", 1, "filament"), "PETG");
	EXPECT_EQ(config().extruders[1].filament, "PETG");

	tap(lv_obj_get_child(in("tools", 0, "extruders"), 1));
	EXPECT_EQ(config().tools[0].extruders, (std::vector<int>{0, 1}));

	tap(in("extruders", 0, "remove"));
	EXPECT_EQ(config().tools[0].extruders, std::vector<int>{0}); // the second, renumbered
}

TEST_F(TestSimulatorSettings, HeatersRatesAndJobsThumbnails)
{
	tap(in("heaters", 1, "cooling_rate"));
	TestDialogs::enterOnPad("6");
	runTimers();
	EXPECT_EQ(config().heaters[1].coolingRate, 6.0f);

	EXPECT_TRUE(lv_obj_has_state(in("files", 0, "thumbnail"), LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(in("files", 2, "thumbnail"), LV_STATE_CHECKED));
	tap(in("files", 0, "thumbnail"));
	EXPECT_FALSE(config().files[0].thumbnail);
}

TEST_F(TestSimulatorSettings, NamesAreTyped)
{
	type(in("heaters", 1, "name"), "Nozzle");
	EXPECT_EQ(config().heaters[1].name, "Nozzle");

	type(in("files", 0, "name"), ""); // a job needs a name: it is given one
	EXPECT_EQ(config().files[0].name, "job 1.gcode");
	EXPECT_STREQ(lv_textarea_get_text(in("files", 0, "name")), "job 1.gcode");
}

TEST_F(TestSimulatorSettings, AnAxisLetterTakenSwaps)
{
	tap(in("axes", 2, "letter"));
	TestDialogs::choose(0); // X, which axis 0 has
	runTimers();
	EXPECT_EQ(config().axes[2].letter, "X");
	EXPECT_EQ(config().axes[0].letter, "Z");

	tap(in("axes", 2, "letter"));
	TestDialogs::choose(5); // W, which none has
	runTimers();
	EXPECT_EQ(config().axes[2].letter, "W");
}

TEST_F(TestSimulatorSettings, NumbersAreAskedForOnTheNumberPad)
{
	tap(in("heaters", 1, "max"));
	EXPECT_EQ(TestDialogs::title(), "Heater 1 · Max");
	TestDialogs::enterOnPad("250");
	runTimers();
	EXPECT_EQ(config().heaters[1].max, 250.0f);
	EXPECT_EQ(label(in("heaters", 1, "max"), "value"), "250 °C");

	tap(in("files", 0, "print_time"));
	TestDialogs::enterOnPad("2");
	runTimers();
	EXPECT_EQ(config().files[0].printTime, 120);
}

TEST_F(TestSimulatorSettings, ChoicesAndSwitches)
{
	tap(in("heaters", 1, "role"));
	TestDialogs::choose(3); // other
	runTimers();
	EXPECT_EQ(config().heaters[1].role, Sim::HeaterRole::other);

	tap(in("heaters", 1, "fault"));
	EXPECT_TRUE(config().heaters[1].fault);
	EXPECT_TRUE(lv_obj_has_state(in("heaters", 1, "fault"), LV_STATE_CHECKED));
	tap(in("heaters", 1, "fault"));
	EXPECT_FALSE(config().heaters[1].fault);
}

TEST_F(TestSimulatorSettings, MachineWideSettings)
{
	setSetting("simulator_mode", 1);
	EXPECT_EQ(config().mode, Sim::MachineMode::cnc);

	setSetting("simulator_status", 12); // halted
	EXPECT_EQ(Sim::Simulator::get().forcedStatus(), "halted");
	setSetting("simulator_status", 0);
	EXPECT_FALSE(Sim::Simulator::get().forcedStatus().has_value());

	auto* settings = UI::Xml::DataSources::find<UI::Xml::SettingsSource>();
	ASSERT_TRUE(settings->set("simulator_noise", 1.5f)); // edited in °C, as the slider does
	EXPECT_FLOAT_EQ(Sim::Simulator::get().behaviour().noise, 1.5f);
	setSetting("simulator_responding", 0);
	EXPECT_FALSE(Sim::Simulator::get().behaviour().responding);
}

TEST_F(TestSimulatorSettings, ChangesReachTheScreen)
{
	connectSimulator(config());
	pollSimulator();
	EXPECT_EQ(OM::Tree::get().root().at("heat.heaters").size(), 2u);

	tap(lv_obj_find_by_name(lv_obj_find_by_name(group("heaters"), "add"), "button"));
	pollSimulator();
	EXPECT_EQ(OM::Tree::get().root().at("heat.heaters").size(), 3u);

	tap(lv_obj_find_by_name(group("messages"), "mode_4"));
	pollSimulator();
	EXPECT_EQ(OM::Tree::get().root().at("state.messageBox.mode").integer(), 4);
}

#endif

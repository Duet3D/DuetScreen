/*
 * test_gauges.cpp
 *
 * The gauge widgets (assets/ui/components/gauge/, widgets/gauges/): the faces each show what the names they are
 * given hold, the widgets of one kind of thing give them the names of that thing (a bed, a tool's heater, a fan),
 * lists show a gauge per row, and the actions given to a gauge run when it is tapped or held.
 */

#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	/* Gauges of each kind, as placed widgets give them their props. The screen isn't a placed widget, so its size
	 * classes are a page's (expanded): the bars don't grow, as they wouldn't in a placed widget their size */
	constexpr std::string_view GAUGES_SCREEN = R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%" flex_flow="row_wrap">
				<gauge_beds name="bed" key="row.index" match="0" look="bar" grow="0" width="272" height="89"/>
				<gauge_beds name="no_bed" key="row.index" match="1" look="bar" grow="0" width="272" height="89"/>
				<gauge_tool name="tool" tool="0" key="row.index" match="0" look="arc" width="180" height="184"/>
				<gauge_fans name="fan" key="row.index" match="1" look="value" width="180" height="89"/>
				<gauge_bar name="speed" frame="card" grow="0" title="tr.widget.status_speed" value="om.move.speedFactor|percent"
						   text="om.move.speedFactor|percent" unit="'%'" max="200" width="272" height="89"/>
				<gauge_heaters name="heaters" look="value" grow="0" width="272" height="277"/>
				<gauge_list name="custom" list="om.fans" shown="row.exists" title="row.name|default:tr.fine_tune.fan"
							look="bar" flow="pages" tap_action="actions.gcode.send" tap_arg="M106 P{row.index} S255"
							hold_action="actions.gcode.send" hold_arg="M106 P{row.index} S0" width="272" height="150"/>
			</view>
		</component>)";
} // namespace

class TestGauges : public UiTestSuite
{
  protected:
	TestGauges()
	{
		m_userDir = fs::temp_directory_path() / fmt::format("duetscreen-gauges-{:d}", getpid());
		fs::remove_all(m_userDir);
		fs::create_directories(m_userDir / "test" / "components");
		std::ofstream(m_userDir / "test" / "manifest.json")
			<< R"({"id": "test", "ui_api": "1.0", "components": ["components/*.xml"]})";
		std::ofstream(m_userDir / "test" / "components" / "gauges_screen.xml") << GAUGES_SCREEN;
		UI::Xml::deinit();
		UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { m_sent.emplace_back(gcode); });

		load_model_data_from_file("tests/object_model/test_bench/model_heat_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_tools_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_fans_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_sensors_v.json");
		m_view = std::make_unique<UI::Xml::XmlView>("gauges", screen, "gauges_screen");
		deliverModelEvents();
	}

	~TestGauges() override
	{
		m_view.reset();
		UI::Xml::Services::setGcodeSender(nullptr);
		fs::remove_all(m_userDir);
	}

	/** The widget called `name` */
	lv_obj_t* widget(const char* name) const { return lv_obj_find_by_name(m_view->getRootPtr(), name); }

	/** The gauges of the list widget called `name`, in row order (a paged list's dots aren't one) */
	std::vector<lv_obj_t*> gauges(const char* name) const
	{
		lv_obj_t* list = lv_obj_find_by_name(widget(name), "list");
		std::vector<lv_obj_t*> result;
		for (uint32_t i = 0; list != nullptr && i < lv_obj_get_child_count(list); i++)
		{
			lv_obj_t* child = lv_obj_get_child(list, static_cast<int32_t>(i));
			if (lv_obj_find_by_name(child, "title") != nullptr)
			{
				result.push_back(child);
			}
		}
		return result;
	}

	/** The one gauge of the single gauge widget called `name` */
	lv_obj_t* gauge(const char* name) const
	{
		const auto all = gauges(name);
		EXPECT_EQ(all.size(), 1u) << name;
		return all.empty() ? nullptr : all.front();
	}

	/** The value `gauge` shows, as text (the face's bar or arc is called `value` too) */
	static std::string reading(lv_obj_t* gauge)
	{
		return text(gauge != nullptr ? lv_obj_find_by_name(gauge, "reading") : nullptr, "value");
	}

	/** The text of the label called `label` in `gauge` */
	static std::string text(lv_obj_t* gauge, const char* label)
	{
		lv_obj_t* obj = gauge != nullptr ? lv_obj_find_by_name(gauge, label) : nullptr;
		EXPECT_NE(obj, nullptr) << label;
		return obj != nullptr ? lv_label_get_text(obj) : "";
	}

	fs::path m_userDir;						  // The user bundle with the test screen
	std::unique_ptr<UI::Xml::XmlView> m_view; // The gauges
	std::vector<std::string> m_sent;		  // G-code sent
};

TEST_F(TestGauges, EachKindShowsWhatItIsAbout)
{
	// Bed 0 is heater 0, named for its sensor, off
	lv_obj_t* bed = gauge("bed");
	EXPECT_EQ(text(bed, "title"), "Bed");
	EXPECT_EQ(text(bed, "unit"), "°C");
	EXPECT_EQ(text(bed, "detail"), _("temperature.status.off"));
	EXPECT_FALSE(lv_obj_has_flag(bed, LV_OBJ_FLAG_HIDDEN));

	// Tool 0's heater is heater 2, named for the tool, which has no name
	lv_obj_t* tool = gauge("tool");
	EXPECT_EQ(text(tool, "title"), _("temperature.default_tool_name", 0));
	EXPECT_EQ(reading(tool), "23.6");
	EXPECT_EQ(text(tool, "detail"), _("temperature.status.active")) << "no target above 0";

	// Fan 1, unnamed and without a tachometer
	lv_obj_t* fan = gauge("fan");
	EXPECT_EQ(text(fan, "title"), _("fine_tune.fan", 1));
	EXPECT_EQ(reading(fan), "0");
	EXPECT_EQ(text(fan, "unit"), "%");
	EXPECT_EQ(text(fan, "detail"), "");

	// Not of a list: constants and object model names given directly
	lv_obj_t* speed = lv_obj_find_by_name(widget("speed"), "title");
	ASSERT_NE(speed, nullptr);
	EXPECT_EQ(std::string(lv_label_get_text(speed)), _("widget.status_speed"));
}

TEST_F(TestGauges, ASlotWithoutAHeaterShowsNothing)
{
	// Bed 1 has no heater (-1)
	lv_obj_t* none = gauge("no_bed");
	ASSERT_NE(none, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(none, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(TestGauges, AListHasAGaugePerRowAndHidesTheGaps)
{
	// Five heaters, heater 1 a gap
	const auto heaters = gauges("heaters");
	ASSERT_EQ(heaters.size(), 5u);
	EXPECT_TRUE(lv_obj_has_flag(heaters[1], LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(heaters[4], LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQ(text(heaters[2], "title"), "Revo with long heater name");
	EXPECT_EQ(text(heaters[3], "title"), _("temperature.heater", 3)) << "its sensor has no name";
}

TEST_F(TestGauges, TheActionsGivenRunWhenAGaugeIsTappedOrHeld)
{
	// Fans 1 and 3 are there; the placeholders are each gauge's row
	const auto fans = gauges("custom");
	ASSERT_EQ(fans.size(), 4u);
	lv_obj_send_event(fans[3], LV_EVENT_SINGLE_CLICKED, nullptr);
	lv_obj_send_event(fans[1], LV_EVENT_LONG_PRESSED, nullptr);
	const std::vector<std::string> expected = {"M106 P3 S255\n", "M106 P1 S0\n"};
	EXPECT_EQ(m_sent, expected);

	// A gauge with no actions does nothing
	lv_obj_send_event(gauge("bed"), LV_EVENT_LONG_PRESSED, nullptr);
	EXPECT_EQ(m_sent, expected);
}

TEST_F(TestGauges, EachFaceAndArrangementLooksRight)
{
	// Bars, an arc, a number, a column of numbers and the fans a page at a time, a dot for each fan there is
	lv_obj_update_layout(m_view->getRootPtr());
	EXPECT_EQUAL_SCREENSHOT("gauges/gauges.png")
}

TEST_F(TestGauges, APagedListHasADotForEachRowThatIsThere)
{
	// Fans 0 and 2 are gaps: two pages
	lv_obj_t* list = lv_obj_find_by_name(widget("custom"), "list");
	ASSERT_NE(list, nullptr);
	lv_obj_t* dots = lv_obj_get_child(list, -1);
	EXPECT_EQ(lv_obj_get_child_count(dots), 2u);
}

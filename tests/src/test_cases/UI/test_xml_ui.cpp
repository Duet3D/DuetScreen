/*
 * test_xml_ui.cpp
 *
 * The runtime-loaded XML UI (src/UI/Xml): bundles, per-item list subjects, presenters and actions, overrides and
 * their fallbacks, reload, theme styles and translations. The Fan screen is the vertical slice; its screenshots are
 * covered by test_home_view.cpp (control_view/fan.png), these tests check behaviour and structure.
 */

#include "ObjectModel/Fan.h"
#include "ObjectModel/Tool.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/Binding/Globals.h"
#include "UI/Xml/Binding/History.h"
#include "UI/Xml/Data/FanSource.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "UI/Xml/Widgets/DsGraph.h"
#include "UI/Xml/Widgets/DsModal.h"
#include "UI/Xml/Widgets/DsTabs.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include <algorithm>
#include <filesystem>
#include <fmt/ranges.h>
#include <fstream>
#include <gtest/gtest.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace
{
	// Fan 0 is thermostatic; fans 1 and 2 can be set by hand.
	// One line each: the decoder treats a newline as the end of a message.
	constexpr std::string_view THREE_FANS =
		R"({"key":"fans","flags":"v","result":[{"actualValue":0.3,"requestedValue":0.3,"rpm":1200,"thermostatic":{"lowTemperature":45,"sensors":[1]}},{"actualValue":0.5,"requestedValue":0.5,"rpm":-1,"thermostatic":{"sensors":[]}},{"actualValue":0,"requestedValue":0,"rpm":-1,"thermostatic":{"sensors":[]}}]})";

	constexpr std::string_view ONE_FAN =
		R"({"key":"fans","flags":"v","result":[{"actualValue":0.3,"requestedValue":0.3,"rpm":1200,"thermostatic":{"lowTemperature":45,"sensors":[1]}}]})";

	constexpr std::string_view FAN_1_AT_80 =
		R"({"key":"fans","flags":"v","result":[{"actualValue":0.3,"requestedValue":0.3,"rpm":1200,"thermostatic":{"lowTemperature":45,"sensors":[1]}},{"actualValue":0.8,"requestedValue":0.8,"rpm":-1,"thermostatic":{"sensors":[]}},{"actualValue":0,"requestedValue":0,"rpm":-1,"thermostatic":{"sensors":[]}}]})";

	void writeFile(const fs::path& path, std::string_view text)
	{
		fs::create_directories(path.parent_path());
		std::ofstream(path) << text;
	}

	std::string manifest(std::string_view api = "1.0")
	{
		return fmt::format(R"({{"id": "test", "ui_api": "{:s}", "components": ["components/*.xml"]}})", api);
	}

	/* A screen of tools, each with its own list of heaters: nested lists */
	constexpr std::string_view TOOLS_SCREEN = R"(
		<component presenter="test_tools">
			<view extends="lv_obj" width="100%" height="100%" flex_flow="column">
				<ds_list name="tools" list="tools" item="dashboard_tool" width="100%" height="content" flex_flow="column"/>
			</view>
		</component>)";

	constexpr std::string_view TOOL_CARD = R"(
		<component>
			<view extends="lv_obj" width="100%" height="content" flex_flow="column">
				<lv_label name="tool_name" bind_text="name"/>
				<ds_list name="heaters" list="heaters" item="tool_heater_row" width="100%" height="content"/>
			</view>
		</component>)";

	/* `name` and `index` are the heater's; `number` is only on the tool row, `title` only on the presenter */
	constexpr std::string_view TOOL_HEATER_ROW = R"(
		<component>
			<view extends="lv_obj" width="100%" height="content" flex_flow="row">
				<lv_label name="heater_name" bind_text="name"/>
				<lv_label name="title" bind_text="title"/>
				<ds_button name="edit" text="Edit">
					<action trigger="clicked" name="test.heater"/>
				</ds_button>
				<ds_button name="send" text="Send">
					<action trigger="clicked" name="gcode" arg="T{number} H{index} {title}"/>
				</ds_button>
			</view>
		</component>)";

	/** One tool with one heater, replacing the test bench's four tools */
	constexpr std::string_view ONE_TOOL =
		R"({"key":"tools","flags":"v","result":[{"active":[200],"heaters":[2],"name":"Solo","number":0,"standby":[150],"state":"active"}]})";

	/** Declares `title` and records which rows `test.heater` came from */
	class RecordingPresenter : public UI::Xml::XmlPresenter
	{
	  public:
		struct Call
		{
			int32_t tool;	// ToolRow::number, or -1 if no tool row was found
			int32_t heater; // Row index of the ToolHeaterRow, or -1 if none was found
			bool innermost; // Whether row() (any type) found the heater row
			bool noFanRow;	// Whether row<FanRow>() found nothing
			bool operator==(const Call&) const = default;
		};

		RecordingPresenter()
			: XmlPresenter("test_tools")
		{
			action("test.heater",
				   "Record the heater and tool the action came from",
				   [this](const UI::Xml::ActionArgs& args)
				   {
					   auto* tool = args.row<UI::Xml::ToolRow>();
					   auto* heater = args.row<UI::Xml::ToolHeaterRow>();
					   calls.push_back({tool ? tool->number.get() : -1,
										heater ? heater->index.get() : -1,
										heater != nullptr && args.row() == heater,
										args.row<UI::Xml::FanRow>() == nullptr});
				   });
		}

		void fields(UI::Xml::FieldVisitor& f) override { f.field("title", m_title, "A presenter subject"); }

		std::vector<Call> calls;

	  private:
		UI::Xml::StringSubject<16> m_title{"hot"};
	};

	/* A graph of the presenter's `samples` history: 2 seconds at 10 samples a second is 20 points */
	constexpr std::string_view HISTORY_SCREEN = R"(
		<component presenter="test_history">
			<view extends="lv_obj" width="100%" height="100%">
				<ds_graph name="graph" history="samples" seconds="2" y_min="0" y_max="100" bind_y_max="limit"
						  width="100%" height="100%"/>
			</view>
		</component>)";

	/** Owns a history, recorded into by the tests, and a limit for the Y axis */
	class HistoryPresenter : public UI::Xml::XmlPresenter
	{
	  public:
		HistoryPresenter()
			: XmlPresenter("test_history")
		{
		}

		void fields(UI::Xml::FieldVisitor& f) override
		{
			f.list("samples", history, "Test history");
			f.field("limit", limit, "Test limit");
		}

		/** Make the history `names.size()` series long, named `names` */
		void setSeries(const std::vector<std::string>& names)
		{
			history.sync(names.size(), [&](UI::Xml::SeriesRow& row, size_t i) { row.name.set(names[i]); });
		}

		UI::Xml::History history{10, 50};
		UI::Xml::IntSubject limit;
	};

	/** A row holding one number */
	class NumberRow : public UI::Xml::Row
	{
	  public:
		void fields(UI::Xml::FieldVisitor& f) override { f.field("value", value, "The number"); }

		UI::Xml::IntSubject value;
	};

	/** Owns a dialog subject and a long list, and records the actions it handles */
	class DialogPresenter : public UI::Xml::XmlPresenter
	{
	  public:
		DialogPresenter()
			: XmlPresenter("test_dialog")
		{
			action("test.ok", "Record ok", [this](const UI::Xml::ActionArgs&) { calls.emplace_back("ok"); });
			action("test.dismissed",
				   "Record dismissed",
				   [this](const UI::Xml::ActionArgs&)
				   {
					   calls.emplace_back("dismissed");
					   dialog.set("");
				   });
		}

		void fields(UI::Xml::FieldVisitor& f) override
		{
			f.field("dialog", dialog, "Which dialog is open");
			f.field("busy", busy, "Disables a tab");
			f.field("job", job, "Selects a tab when set");
			f.list("numbers", numbers, "A long list");
		}

		/** Make `numbers` `count` long, each row holding its index */
		void setCount(size_t count)
		{
			numbers.sync(count, [](NumberRow& row, size_t i) { row.value.set(static_cast<int32_t>(i)); });
		}

		std::vector<std::string> calls;
		UI::Xml::StringSubject<16> dialog;
		UI::Xml::IntSubject busy;
		UI::Xml::StringSubject<16> job;
		UI::Xml::List<NumberRow> numbers;
	};

	/* A dialog, tabs and two windowed lists (a column of rows, and a grid of 100x50 tiles, 3 to a line) */
	constexpr std::string_view DIALOG_SCREEN = R"(
		<component presenter="test_dialog">
			<view extends="lv_obj" width="100%" height="100%" style_pad_all="0">
				<ds_modal name="dialog" bind_open="dialog" open_value="ask" width="50%" height="content">
					<action trigger="dismissed" name="test.dismissed"/>
					<ds_button name="ok" text="OK">
						<action trigger="clicked" name="test.ok"/>
					</ds_button>
				</ds_modal>
				<ds_tabs name="tabs" width="300" height="100">
					<ds_tabs-tab name="first" title="First"/>
					<ds_tabs-tab name="second" title="Second" bind_disabled="busy"/>
					<ds_tabs-tab name="third" title="Third" select_on="job"/>
				</ds_tabs>
				<ds_list name="rows" list="numbers" item="number_item" windowed="true" item_height="50"
						 width="300" height="200" y="100" style_pad_all="0" style_pad_row="0"/>
				<ds_list name="tiles" list="numbers" item="number_item" windowed="true" item_width="100"
						 item_height="50" flow="row_wrap" width="300" height="100" y="300" style_pad_all="0"
						 style_pad_row="0" style_pad_column="0"/>
			</view>
		</component>)";

	constexpr std::string_view NUMBER_ITEM = R"(
		<component>
			<view extends="lv_obj">
				<lv_label name="value" bind_text="value"/>
			</view>
		</component>)";

	/* Hosts a view that is given the test_history presenter by name */
	constexpr std::string_view HOST_SCREEN = R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_view name="hosted" view="hosted_limit" presenter="test_history" shown="true"/>
			</view>
		</component>)";

	constexpr std::string_view HOSTED_LIMIT = R"(
		<component>
			<view extends="lv_obj">
				<lv_label name="limit" bind_text="limit"/>
			</view>
		</component>)";
} // namespace

class TestXmlUi : public UiTestSuite
{
  protected:
	TestXmlUi()
	{
		UI::Xml::Presenters::add<RecordingPresenter>("test_tools");
		UI::Xml::Presenters::add<HistoryPresenter>("test_history");
		UI::Xml::Presenters::add<DialogPresenter>("test_dialog");
		m_userDir = fs::temp_directory_path() / fmt::format("duetscreen-xml-ui-{:d}", getpid());
		fs::remove_all(m_userDir);
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { m_sent.emplace_back(gcode); });
	}

	~TestXmlUi() override
	{
		m_views.clear();
		fs::remove_all(m_userDir);
	}

	/** Restart the XML UI with a user bundle folder */
	void restartWithUserBundles()
	{
		m_views.clear();
		UI::Xml::deinit();
		UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { m_sent.emplace_back(gcode); });
	}

	fs::path bundleFile(std::string_view relative) const { return m_userDir / "test" / relative; }

	UI::Xml::XmlView& makeView(const std::string& component = "fans")
	{
		m_views.push_back(
			std::make_unique<UI::Xml::XmlView>(fmt::format("view{:d}", m_views.size()), screen, component));
		return *m_views.back();
	}

	/** Deliver queued model events, as the event loop would */
	static void pump()
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}

	static lv_obj_t* find(UI::LvObj& root, const char* name) { return lv_obj_find_by_name(root.getRootPtr(), name); }
	static lv_obj_t* find(lv_obj_t* root, const char* name) { return lv_obj_find_by_name(root, name); }

	static lv_obj_t* fanList(UI::LvObj& view) { return find(view, "list"); }
	static lv_obj_t* fanCard(UI::LvObj& view, int32_t index) { return lv_obj_get_child(fanList(view), index); }

	/** Install the tools screen as a user bundle and fill the model from the test bench */
	UI::Xml::XmlView& makeToolsView()
	{
		writeFile(bundleFile("manifest.json"), manifest());
		writeFile(bundleFile("components/tools_screen.xml"), TOOLS_SCREEN);
		writeFile(bundleFile("components/dashboard_tool.xml"), TOOL_CARD);
		writeFile(bundleFile("components/tool_heater_row.xml"), TOOL_HEATER_ROW);
		restartWithUserBundles();
		EXPECT_TRUE(UI::Xml::getIssues().empty());

		load_model_data_from_file("tests/object_model/test_bench/model_sensors_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_heat_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_tools_v.json");
		auto& view = makeView("tools_screen");
		pump();
		return view;
	}

	static lv_obj_t* toolCard(UI::LvObj& view, int32_t index) { return lv_obj_get_child(find(view, "tools"), index); }

	static lv_obj_t* heaterRow(UI::LvObj& view, int32_t tool, int32_t heater)
	{
		return lv_obj_get_child(find(toolCard(view, tool), "heaters"), heater);
	}

	static std::string_view text(lv_obj_t* parent, const char* name) { return lv_label_get_text(find(parent, name)); }

	static UI::Slider& slider(lv_obj_t* card)
	{
		return static_cast<UI::Slider&>(*UI::LvObj::fromPtr(find(card, "slider")));
	}

	/** Install the history screen as a user bundle */
	void installHistoryScreen()
	{
		writeFile(bundleFile("manifest.json"), manifest());
		writeFile(bundleFile("components/history_screen.xml"), HISTORY_SCREEN);
		restartWithUserBundles();
		EXPECT_TRUE(UI::Xml::getIssues().empty());
	}

	static UI::Xml::DsGraph& graph(UI::LvObj& view)
	{
		return static_cast<UI::Xml::DsGraph&>(*UI::LvObj::fromPtr(find(view, "graph")));
	}

	/** The newest `count` values a graph shows for a series, oldest first */
	static std::vector<int32_t> shown(UI::Graph& graph, size_t series, uint32_t count)
	{
		lv_obj_t* chart = find(graph, "chart");
		lv_chart_series_t* ser = graph.getSeries(series)->series;
		const uint32_t points = lv_chart_get_point_count(chart);
		const uint32_t start = lv_chart_get_x_start_point(chart, ser); // The oldest point
		const int32_t* y = lv_chart_get_series_y_array(chart, ser);
		std::vector<int32_t> values;
		for (uint32_t i = points - count; i < points; i++)
		{
			values.push_back(y[(start + i) % points]);
		}
		return values;
	}

	/** Install the dialog and host screens as a user bundle */
	void installDialogScreens()
	{
		writeFile(bundleFile("manifest.json"), manifest());
		writeFile(bundleFile("components/dialog_screen.xml"), DIALOG_SCREEN);
		writeFile(bundleFile("components/number_item.xml"), NUMBER_ITEM);
		writeFile(bundleFile("components/host_screen.xml"), HOST_SCREEN);
		writeFile(bundleFile("components/hosted_limit.xml"), HOSTED_LIMIT);
		restartWithUserBundles();
		EXPECT_TRUE(UI::Xml::getIssues().empty());
	}

	/** The values shown by a windowed list's items, in row order */
	static std::vector<int32_t> builtRows(lv_obj_t* list)
	{
		std::vector<int32_t> rows;
		for (uint32_t i = 0; i < lv_obj_get_child_count(list); i++)
		{
			lv_obj_t* label = find(lv_obj_get_child(list, static_cast<int32_t>(i)), "value");
			if (label != nullptr)
			{
				rows.push_back(std::stoi(lv_label_get_text(label)));
			}
		}
		std::sort(rows.begin(), rows.end());
		return rows;
	}

	fs::path m_userDir;
	std::vector<std::string> m_sent;
	std::vector<std::unique_ptr<UI::Xml::XmlView>> m_views;
};

TEST_F(TestXmlUi, BuiltinBundleLoadsCleanly)
{
	EXPECT_TRUE(UI::Xml::getIssues().empty());

	const auto* screen = UI::Xml::findComponent("fans");
	ASSERT_NE(screen, nullptr);
	EXPECT_EQ(screen->presenter, "fans");
	EXPECT_EQ(screen->bundle, "duetscreen");

	const auto* card = UI::Xml::findComponent("fan_card");
	ASSERT_NE(card, nullptr);
	EXPECT_TRUE(card->presenter.empty());
}

TEST_F(TestXmlUi, ViewHasItsPresenterAndTranslatedText)
{
	auto& view = makeView();
	ASSERT_NE(view.getPresenter(), nullptr);
	EXPECT_EQ(view.getPresenter()->getName(), "fans");

	lv_obj_t* title = find(view, "title");
	ASSERT_NE(title, nullptr);
	EXPECT_STREQ(lv_label_get_text(title), _("fan.title").c_str());
}

TEST_F(TestXmlUi, EachCardBindsToItsOwnFan)
{
	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();

	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 3u);

	// Attributes the engine doesn't know are only logged, and LVGL logging is off in this build, so check that the
	// layout attributes in the bundle actually reached the widgets
	EXPECT_EQ(lv_obj_get_style_flex_flow(fanCard(view, 1), LV_PART_MAIN), LV_FLEX_FLOW_ROW);
	EXPECT_EQ(lv_obj_get_style_flex_cross_place(fanCard(view, 1), LV_PART_MAIN), LV_FLEX_ALIGN_CENTER);
	EXPECT_EQ(lv_obj_get_style_flex_track_place(view.getRootPtr(), LV_PART_MAIN), LV_FLEX_ALIGN_CENTER);
	EXPECT_EQ(lv_obj_get_style_flex_cross_place(find(view, "header"), LV_PART_MAIN), LV_FLEX_ALIGN_CENTER);

	// Thermostatic fans are hidden by a binding in fan_card.xml, not filtered in C++
	EXPECT_TRUE(lv_obj_has_flag(fanCard(view, 0), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(fanCard(view, 1), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(fanCard(view, 2), LV_OBJ_FLAG_HIDDEN));

	EXPECT_EQ(slider(fanCard(view, 1)).getLabelText(), _("fine_tune.fan", 1));
	EXPECT_EQ(slider(fanCard(view, 2)).getLabelText(), _("fine_tune.fan", 2));
	EXPECT_FLOAT_EQ(slider(fanCard(view, 1)).getValue(), 50.0f);
	EXPECT_FLOAT_EQ(slider(fanCard(view, 2)).getValue(), 0.0f);

	// Updating the model updates only that card
	load_model_data(FAN_1_AT_80);
	pump();
	EXPECT_FLOAT_EQ(slider(fanCard(view, 1)).getValue(), 80.0f);
	EXPECT_FLOAT_EQ(slider(fanCard(view, 2)).getValue(), 0.0f);
}

TEST_F(TestXmlUi, TwoViewsShareOneModel)
{
	auto& a = makeView();
	auto& b = makeView();
	load_model_data(THREE_FANS);
	pump();

	ASSERT_EQ(lv_obj_get_child_count(fanList(a)), 3u);
	ASSERT_EQ(lv_obj_get_child_count(fanList(b)), 3u);
	EXPECT_EQ(slider(fanCard(a, 2)).getLabelText(), _("fine_tune.fan", 2));
	EXPECT_EQ(slider(fanCard(b, 2)).getLabelText(), _("fine_tune.fan", 2));
}

TEST_F(TestXmlUi, ListGrowsAndShrinksAtTheEnd)
{
	auto& view = makeView();
	load_model_data(ONE_FAN);
	pump();
	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 1u);
	lv_obj_t* first = fanCard(view, 0);

	load_model_data(THREE_FANS);
	pump();
	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 3u);
	EXPECT_EQ(fanCard(view, 0), first) << "existing cards are kept when the list grows";

	load_model_data(ONE_FAN);
	pump();
	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 1u);
	EXPECT_EQ(fanCard(view, 0), first);

	// Disconnecting empties the list
	Model::get().post<EventType::Disconnected>();
	pump();
	EXPECT_EQ(lv_obj_get_child_count(fanList(view)), 0u);
}

TEST_F(TestXmlUi, ButtonsReachThePresenterWithTheirFan)
{
	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();

	lv_obj_send_event(find(fanCard(view, 1), "max"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(find(fanCard(view, 2), "max"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(find(fanCard(view, 1), "off"), LV_EVENT_CLICKED, nullptr);
	// Already off: nothing to send
	lv_obj_send_event(find(fanCard(view, 2), "off"), LV_EVENT_CLICKED, nullptr);

	EXPECT_EQ(m_sent, (std::vector<std::string>{"M106 P1 S255\n", "M106 P2 S255\n", "M106 P1 S0\n"}));
}

TEST_F(TestXmlUi, SliderEditsSendButBindingUpdatesDont)
{
	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();
	EXPECT_TRUE(m_sent.empty()) << "showing the model's values is not an edit";

	slider(fanCard(view, 2)).setValue(40.0f);
	EXPECT_EQ(m_sent, (std::vector<std::string>{"M106 P2 S102\n"}));

	m_sent.clear();
	load_model_data(FAN_1_AT_80);
	pump();
	EXPECT_FLOAT_EQ(slider(fanCard(view, 1)).getValue(), 80.0f);
	EXPECT_TRUE(m_sent.empty());
}

TEST_F(TestXmlUi, UserBundleOverridesAComponent)
{
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/fan_card.xml"), R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="custom" bind_text="name"/>
				<ds_button name="home" text="Home">
					<action trigger="clicked" name="gcode" arg="G28 ; from fan {number} at {index}"/>
				</ds_button>
			</view>
		</component>)");
	restartWithUserBundles();

	EXPECT_TRUE(UI::Xml::getIssues().empty());
	EXPECT_EQ(UI::Xml::findComponent("fan_card")->bundle, "test");

	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();

	// The built-in screen now uses the user's card, and thermostatic fans are no longer hidden
	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 3u);
	lv_obj_t* label = find(fanCard(view, 0), "custom");
	ASSERT_NE(label, nullptr);
	EXPECT_STREQ(lv_label_get_text(label), _("fine_tune.fan", 0).c_str());
	EXPECT_EQ(find(fanCard(view, 0), "slider"), nullptr);

	// A global action with placeholders filled from the row
	lv_obj_send_event(find(fanCard(view, 2), "home"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(m_sent, (std::vector<std::string>{"G28 ; from fan 2 at 2\n"}));
}

TEST_F(TestXmlUi, BrokenOverrideFallsBackToBuiltin)
{
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/fan_card.xml"), "<component>\n<view>\n<lv_label>\n</view>\n</component>\n");
	restartWithUserBundles();

	ASSERT_EQ(UI::Xml::getIssues().size(), 1u);
	EXPECT_EQ(UI::Xml::getIssues()[0].file.filename(), "fan_card.xml");
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("line 4"), std::string::npos) << UI::Xml::getIssues()[0].message;
	EXPECT_EQ(UI::Xml::findComponent("fan_card")->bundle, "duetscreen");

	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();
	EXPECT_NE(find(fanCard(view, 1), "slider"), nullptr);
}

TEST_F(TestXmlUi, OverrideNamingAnUnknownPresenterIsRejected)
{
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/fans.xml"), R"(<component presenter="nope"><view extends="lv_obj"/></component>)");
	restartWithUserBundles();

	ASSERT_EQ(UI::Xml::getIssues().size(), 1u);
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("unknown presenter 'nope'"), std::string::npos);
	EXPECT_EQ(UI::Xml::findComponent("fans")->presenter, "fans");
}

TEST_F(TestXmlUi, BundleForAnotherUiApiIsNotLoaded)
{
	writeFile(bundleFile("manifest.json"), manifest("2.0"));
	writeFile(bundleFile("components/fan_card.xml"), R"(<component><view extends="lv_obj"/></component>)");
	restartWithUserBundles();

	ASSERT_EQ(UI::Xml::getIssues().size(), 1u);
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("needs UI API 2.0"), std::string::npos);
	EXPECT_EQ(UI::Xml::findComponent("fan_card")->bundle, "duetscreen");
}

TEST_F(TestXmlUi, DoubleStarFolderFindsComponentsInEveryFolderBelow)
{
	writeFile(bundleFile("manifest.json"), R"({"id": "test", "ui_api": "1.0", "components": ["components/**/*.xml"]})");
	writeFile(bundleFile("components/fan_card.xml"), R"(<component><view extends="lv_obj"/></component>)");
	writeFile(bundleFile("components/control/fans/fans.xml"),
			  R"(<component presenter="fans"><view extends="lv_obj"/></component>)");
	writeFile(bundleFile("components/control/fans/notes.txt"), "not a component");
	restartWithUserBundles();

	EXPECT_TRUE(UI::Xml::getIssues().empty());
	EXPECT_EQ(UI::Xml::findComponent("fan_card")->bundle, "test");
	EXPECT_EQ(UI::Xml::findComponent("fans")->bundle, "test");
	EXPECT_EQ(UI::Xml::findComponent("notes"), nullptr);
}

TEST_F(TestXmlUi, TwoComponentsOfOneNameInABundleAreReported)
{
	writeFile(bundleFile("manifest.json"), R"({"id": "test", "ui_api": "1.0", "components": ["components/**/*.xml"]})");
	writeFile(bundleFile("components/a/fan_card.xml"),
			  R"(<component><view extends="lv_obj"><lv_label name="first"/></view></component>)");
	writeFile(bundleFile("components/b/fan_card.xml"),
			  R"(<component><view extends="lv_obj"><lv_label name="second"/></view></component>)");
	restartWithUserBundles();

	// The first (in path order) is used; the second is reported, naming the first
	ASSERT_EQ(UI::Xml::getIssues().size(), 1u);
	EXPECT_EQ(UI::Xml::getIssues()[0].file, bundleFile("components/b/fan_card.xml"));
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("same name as"), std::string::npos);
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("components/a/fan_card.xml"), std::string::npos);
	EXPECT_EQ(UI::Xml::findComponent("fan_card")->source, bundleFile("components/a/fan_card.xml"));
}

TEST_F(TestXmlUi, ReloadRebuildsLiveViewsAndKeepsThePresenter)
{
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/fan_card.xml"),
			  R"(<component><view extends="lv_obj" height="content"><lv_label name="v1"/></view></component>)");
	restartWithUserBundles();

	auto& view = makeView();
	load_model_data(THREE_FANS);
	pump();
	ASSERT_NE(find(fanCard(view, 0), "v1"), nullptr);
	auto* presenter = view.getPresenter();
	const int32_t index = lv_obj_get_index(view.getRootPtr());

	writeFile(bundleFile("components/fan_card.xml"),
			  R"(<component><view extends="lv_obj" height="content"><lv_label name="v2"/></view></component>)");
	UI::Xml::reload();

	EXPECT_EQ(view.getPresenter(), presenter);
	EXPECT_EQ(lv_obj_get_index(view.getRootPtr()), index);
	ASSERT_EQ(lv_obj_get_child_count(fanList(view)), 3u) << "the rebuilt list starts populated from the model";
	EXPECT_EQ(find(fanCard(view, 0), "v1"), nullptr);
	EXPECT_NE(find(fanCard(view, 0), "v2"), nullptr);
}

TEST_F(TestXmlUi, ThemeStylesAreSharedNotCopied)
{
	auto& view = makeView();
	auto& card = const_cast<UI::Themes::Style&>(UI::Themes::getLvglStyles().card);

	lv_style_value_t original{};
	const bool hadColor = card.getProp(LV_STYLE_BG_COLOR, &original) == LV_STYLE_RES_FOUND;

	// What a theme switch does: rewrite the shared style in place and report it
	card.setProp(LV_STYLE_BG_COLOR, lv_style_value_t{.color = lv_color_hex(0x123456)});
	lv_obj_report_style_change(card);
	EXPECT_EQ(lv_color_to_u32(lv_obj_get_style_bg_color(view.getRootPtr(), LV_PART_MAIN)),
			  lv_color_to_u32(lv_color_hex(0x123456)));

	if (hadColor)
	{
		card.setProp(LV_STYLE_BG_COLOR, original);
	}
	else
	{
		lv_style_remove_prop(card, LV_STYLE_BG_COLOR);
	}
	lv_obj_report_style_change(card);
}

TEST_F(TestXmlUi, MissingComponentShowsAPlaceholder)
{
	auto& view = makeView("no_such_component");
	EXPECT_EQ(view.getPresenter(), nullptr);
	ASSERT_EQ(lv_obj_get_child_count(view.getRootPtr()), 1u);
	EXPECT_NE(std::string_view(lv_label_get_text(lv_obj_get_child(view.getRootPtr(), 0))).find("no_such_component"),
			  std::string_view::npos);
}

TEST_F(TestXmlUi, NestedListsShowEachRowsOwnChildren)
{
	auto& view = makeToolsView();

	// Test bench: T0 (heater 2), T2 (heaters 0 and 3), T3 (none), T4 (heater 2)
	const std::vector<size_t> heaterCounts = {1, 2, 0, 1};
	ASSERT_EQ(lv_obj_get_child_count(find(view, "tools")), heaterCounts.size());
	for (size_t slot = 0; slot < heaterCounts.size(); slot++)
	{
		auto tool = OM::GetToolBySlot(slot);
		ASSERT_NE(tool, nullptr);
		lv_obj_t* card = toolCard(view, static_cast<int32_t>(slot));
		EXPECT_EQ(text(card, "tool_name"), tool->GetName());
		ASSERT_EQ(lv_obj_get_child_count(find(card, "heaters")), heaterCounts[slot]) << "tool slot " << slot;

		for (size_t h = 0; h < heaterCounts[slot]; h++)
		{
			lv_obj_t* row = heaterRow(view, static_cast<int32_t>(slot), static_cast<int32_t>(h));
			const auto& heater = *tool->GetHeater(h)->heater;
			const std::string expected =
				heater.GetName().empty() ? _("temperature.heater", heater.index) : std::string(heater.GetName());
			EXPECT_EQ(text(row, "heater_name"), expected) << "the heater's own name, not the tool's";
			EXPECT_EQ(text(row, "title"), "hot") << "names the rows don't have come from the presenter";
		}
	}
}

TEST_F(TestXmlUi, ActionsFindEveryEnclosingRow)
{
	auto& view = makeToolsView();
	auto* presenter = dynamic_cast<RecordingPresenter*>(view.getPresenter());
	ASSERT_NE(presenter, nullptr);

	lv_obj_send_event(find(heaterRow(view, 1, 1), "edit"), LV_EVENT_CLICKED, nullptr);
	lv_obj_send_event(find(heaterRow(view, 3, 0), "edit"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(presenter->calls,
			  (std::vector<RecordingPresenter::Call>{{.tool = 2, .heater = 1, .innermost = true, .noFanRow = true},
													 {.tool = 4, .heater = 0, .innermost = true, .noFanRow = true}}));

	// Placeholders resolve like bindings: the heater's index, the tool's number, the presenter's title
	lv_obj_send_event(find(heaterRow(view, 1, 1), "send"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(m_sent, (std::vector<std::string>{"T2 H1 hot\n"}));
}

TEST_F(TestXmlUi, NestedListFollowsItsRowsLength)
{
	auto& view = makeToolsView();
	ASSERT_EQ(lv_obj_get_child_count(find(toolCard(view, 1), "heaters")), 2u);
	lv_obj_t* firstCard = toolCard(view, 0);

	load_model_data(ONE_TOOL);
	pump();

	ASSERT_EQ(lv_obj_get_child_count(find(view, "tools")), 1u);
	EXPECT_EQ(toolCard(view, 0), firstCard) << "the remaining card is kept";
	EXPECT_EQ(text(firstCard, "tool_name"), "Solo");
	ASSERT_EQ(lv_obj_get_child_count(find(firstCard, "heaters")), 1u);
}

namespace
{
	/** A style property's value, or -1 if the style doesn't set it */
	int32_t styleNum(const UI::Themes::Style& style, lv_style_prop_t prop)
	{
		lv_style_value_t v{};
		return style.getProp(prop, &v) == LV_STYLE_RES_FOUND ? v.num : -1;
	}

	/** A style colour property as 0xRRGGBB, or -1 if the style doesn't set it */
	int64_t styleColor(const UI::Themes::Style& style, lv_style_prop_t prop)
	{
		lv_style_value_t v{};
		return style.getProp(prop, &v) == LV_STYLE_RES_FOUND ? (lv_color_to_u32(v.color) & 0xFFFFFF) : -1;
	}

	uint32_t rgb(lv_color_t color)
	{
		return lv_color_to_u32(color) & 0xFFFFFF;
	}

	constexpr std::string_view THEME_MANIFEST =
		R"({"id": "test", "ui_api": "1.0", "components": ["components/*.xml"], "themes": ["themes/*.xml"]})";
} // namespace

TEST_F(TestXmlUi, UserThemeExtendsABuiltinTheme)
{
	writeFile(bundleFile("manifest.json"), THEME_MANIFEST);
	writeFile(bundleFile("themes/midnight.xml"), R"(
		<theme name="midnight" extends="duetscreen">
			<palette primary_hue="300"/>
			<consts><const name="radius" value="9"/></consts>
			<styles><style name="card" border_width="0"/></styles>
		</theme>)");
	restartWithUserBundles();
	EXPECT_TRUE(UI::Xml::getIssues().empty());

	UI::Themes::Theme* theme = UI::Themes::getThemeByName("midnight");
	ASSERT_NE(theme, nullptr);
	EXPECT_EQ(UI::Themes::getThemeByName("base"), nullptr) << "abstract themes can't be chosen";
	theme->resetPalette();
	theme->setThemeActive();
	const auto colors = theme->getColors();
	const auto& lvgl = UI::Themes::getLvglStyles();
	const auto& components = UI::Themes::getComponentStyles();

	// The parent's styles, built with the child's palette and consts
	EXPECT_EQ(styleColor(lvgl.btn, LV_STYLE_BG_COLOR), rgb(colors.primary));
	EXPECT_EQ(styleNum(lvgl.card, LV_STYLE_RADIUS), 9);
	// ...then the child's changes
	EXPECT_EQ(styleNum(lvgl.card, LV_STYLE_BORDER_WIDTH), 0);
	// A copy made in the parent sees the card as the parent left it: the child's change came later
	EXPECT_EQ(styleNum(components.tool_list_tool, LV_STYLE_BORDER_WIDTH), 2);
	EXPECT_EQ(styleNum(components.tool_list_tool, LV_STYLE_RADIUS), 9);
}

TEST_F(TestXmlUi, ThemeProblemsAreReported)
{
	writeFile(bundleFile("manifest.json"), THEME_MANIFEST);
	writeFile(bundleFile("themes/broken.xml"), R"(<theme name="broken" extends="duetscreen">
		<styles>
			<style name="no_such_style" radius="1"/>
			<style name="card" bg_color="#no_such_colour"/>
		</styles>
	</theme>)");
	writeFile(bundleFile("themes/orphan.xml"), R"(<theme name="orphan" extends="nope"/>)");
	writeFile(bundleFile("themes/malformed.xml"), "<theme name=\"m\">\n<styles>\n</theme>");
	restartWithUserBundles();

	std::vector<std::string> messages;
	for (const auto& issue : UI::Xml::getIssues())
	{
		messages.push_back(issue.file.filename().string() + ": " + issue.message);
	}
	auto has = [&](std::string_view text)
	{
		return std::any_of(
			messages.begin(), messages.end(), [&](const std::string& m) { return m.find(text) != std::string::npos; });
	};
	EXPECT_TRUE(has("broken.xml: line 3: unknown style 'no_such_style'"))
		<< fmt::format("{}", fmt::join(messages, "\n"));
	EXPECT_TRUE(has("broken.xml: line 4: bg_color: unknown colour or const '#no_such_colour'"));
	EXPECT_TRUE(has("orphan.xml: theme 'nope' not found"));
	EXPECT_TRUE(has("malformed.xml: line 3"));

	// The built-in themes still load and the active one is unaffected
	EXPECT_NE(UI::Themes::getCurrentTheme(), nullptr);
	EXPECT_EQ(UI::Themes::getCurrentTheme()->getName(), "duetscreen");
}

TEST_F(TestXmlUi, ComponentsCanUseThemeColours)
{
	writeFile(bundleFile("manifest.json"), THEME_MANIFEST);
	writeFile(bundleFile("components/swatch.xml"),
			  R"(<component><view extends="lv_obj" style_bg_color="#primary" style_radius="#radius"/></component>)");
	restartWithUserBundles();
	ASSERT_TRUE(UI::Xml::getIssues().empty());

	auto& view = makeView("swatch");
	EXPECT_EQ(rgb(lv_obj_get_style_bg_color(view.getRootPtr(), LV_PART_MAIN)),
			  rgb(UI::Themes::getCurrentTheme()->getColors().primary));
	EXPECT_EQ(lv_obj_get_style_radius(view.getRootPtr(), LV_PART_MAIN), 5);

	// Switching theme recolours the view
	UI::Themes::Theme* grey = UI::Themes::getThemeByName("grey");
	ASSERT_NE(grey, nullptr);
	grey->setThemeActive();
	EXPECT_EQ(rgb(lv_obj_get_style_bg_color(view.getRootPtr(), LV_PART_MAIN)), rgb(grey->getColors().primary));
}

TEST_F(TestXmlUi, GraphStartsWithTheHistoryRecordedBeforeIt)
{
	installHistoryScreen();
	auto presenter = std::make_shared<HistoryPresenter>();
	presenter->setSeries({"A"});
	for (int32_t value : {1, 2, 3})
	{
		presenter->history.record([&](UI::Xml::SeriesRow&, size_t) { return value; });
	}

	auto& view =
		*m_views.emplace_back(std::make_unique<UI::Xml::XmlView>("history", screen, "history_screen", presenter));
	auto& g = graph(view);
	ASSERT_EQ(g.getSeriesCount(), 1u);
	EXPECT_EQ(g.getSeries(0)->legendObj->getText(), "A");
	EXPECT_EQ(lv_chart_get_point_count(find(g, "chart")), 20u) << "2 seconds at 10 samples a second";
	EXPECT_EQ(g.getXRange().min, -2);
	EXPECT_EQ(shown(g, 0, 3), (std::vector<int32_t>{1, 2, 3}));
}

TEST_F(TestXmlUi, GraphFollowsSeriesAndSamples)
{
	installHistoryScreen();
	auto presenter = std::make_shared<HistoryPresenter>();
	auto& view =
		*m_views.emplace_back(std::make_unique<UI::Xml::XmlView>("history", screen, "history_screen", presenter));
	auto& g = graph(view);
	EXPECT_EQ(g.getSeriesCount(), 0u);

	// Added series show up; a value is added to every series
	presenter->setSeries({"A", "B"});
	ASSERT_EQ(g.getSeriesCount(), 2u);
	presenter->history.record([](UI::Xml::SeriesRow&, size_t i) { return static_cast<int32_t>(10 + i); });
	EXPECT_EQ(shown(g, 0, 1), (std::vector<int32_t>{10}));
	EXPECT_EQ(shown(g, 1, 1), (std::vector<int32_t>{11}));

	// Renamed series update their legend
	presenter->setSeries({"A", "Bed"});
	EXPECT_EQ(g.getSeries(1)->legendObj->getText(), "Bed");

	// Removed series go
	presenter->setSeries({"A"});
	EXPECT_EQ(g.getSeriesCount(), 1u);

	// The Y axis reaches the bound limit, but never drops below y_max
	presenter->limit.set(250);
	EXPECT_EQ(g.getYRange().max, 250);
	presenter->limit.set(50);
	EXPECT_EQ(g.getYRange().max, 100);
}

TEST_F(TestXmlUi, GraphOutlivesItsHistory)
{
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/global_history.xml"), R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_graph name="graph" history="test_samples"/>
			</view>
		</component>)");
	restartWithUserBundles();
	auto history = std::make_unique<UI::Xml::History>(10, 50);
	history->sync(1, [](UI::Xml::SeriesRow& row, size_t) { row.name.set("A"); });
	UI::Xml::Globals::add("test_samples", *history);

	auto& view = makeView("global_history");
	ASSERT_EQ(graph(view).getSeriesCount(), 1u);

	UI::Xml::Globals::remove("test_samples");
	history.reset();
	EXPECT_EQ(graph(view).getSeriesCount(), 0u) << "the graph lets go of a destroyed history";
	m_views.clear(); // Deleting the graph afterwards must not touch the history
}

TEST_F(TestXmlUi, ViewCanBeGivenAPresenterByName)
{
	installDialogScreens();
	auto& host = makeView("host_screen");
	auto* hosted = UI::findView<UI::Xml::XmlView>("hosted");
	ASSERT_NE(hosted, nullptr);
	ASSERT_NE(hosted->getPresenter(), nullptr);
	EXPECT_EQ(hosted->getPresenter()->getName(), "test_history");

	auto& presenter = static_cast<HistoryPresenter&>(*hosted->getPresenter());
	presenter.limit.set(42);
	EXPECT_EQ(text(host.getRootPtr(), "limit"), "42") << "the component shows the given presenter's names";
}

TEST_F(TestXmlUi, ReloadRebuildsHostedViewsWithTheirHost)
{
	installDialogScreens();
	auto& host = makeView("host_screen");
	ASSERT_NE(UI::findView("hosted"), nullptr);

	UI::Xml::reload(); // Used to free the hosted view while it rebuilt itself

	auto* hosted = UI::findView<UI::Xml::XmlView>("hosted");
	ASSERT_NE(hosted, nullptr);
	EXPECT_EQ(lv_obj_get_parent(hosted->getRootPtr()), host.getRootPtr()) << "built afresh inside the new host";
}

TEST_F(TestXmlUi, TabDisabledWhileItsSubjectIsSet)
{
	installDialogScreens();
	auto& view = makeView("dialog_screen");
	auto& tabs = static_cast<UI::Xml::DsTabs&>(*UI::LvObj::fromPtr(find(view, "tabs")));
	auto& presenter = static_cast<DialogPresenter&>(*view.getPresenter());
	ASSERT_EQ(tabs.getTabCount(), 3u);
	EXPECT_EQ(tabs.getTabButton(1)->getText(), "Second");
	EXPECT_FALSE(tabs.getTabButton(1)->hasState(LV_STATE_DISABLED));

	presenter.busy.set(1);
	EXPECT_TRUE(tabs.getTabButton(1)->hasState(LV_STATE_DISABLED));
	EXPECT_FALSE(tabs.getTabButton(0)->hasState(LV_STATE_DISABLED));
	presenter.busy.set(0);
	EXPECT_FALSE(tabs.getTabButton(1)->hasState(LV_STATE_DISABLED));
}

TEST_F(TestXmlUi, ModalFollowsItsSubjectFromTheModalHost)
{
	installDialogScreens();
	UI::LvContainer mainWindow("main_window", screen);
	UI::registerView("main_window", &mainWindow);

	auto& view = makeView("dialog_screen");
	auto& presenter = static_cast<DialogPresenter&>(*view.getPresenter());
	lv_obj_t* dialog = find(mainWindow, "dialog");
	ASSERT_NE(dialog, nullptr) << "the dialog is in the modal host";
	EXPECT_EQ(find(view, "dialog"), nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));

	// Its subject opens and closes it, raising nothing
	presenter.dialog.set("ask");
	EXPECT_FALSE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	presenter.dialog.set("other");
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN)) << "open_value must match";
	EXPECT_TRUE(presenter.calls.empty());

	// Actions in it reach the view it was written in
	presenter.dialog.set("ask");
	lv_obj_send_event(find(dialog, "ok"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(presenter.calls, (std::vector<std::string>{"ok"}));

	// Closed by navigation, it says so, and the presenter's answer doesn't reopen it
	UI::closeAllModals();
	EXPECT_EQ(presenter.calls, (std::vector<std::string>{"ok", "dismissed"}));
	EXPECT_EQ(presenter.dialog.get(), "");
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));

	// It goes with the view it was written in
	m_views.clear();
	EXPECT_EQ(find(mainWindow, "dialog"), nullptr);
	UI::forgetView(&mainWindow);
}

TEST_F(TestXmlUi, ModalGoesWithItsHost)
{
	installDialogScreens();
	auto mainWindow = std::make_unique<UI::LvContainer>("main_window", screen);
	UI::registerView("main_window", mainWindow.get());
	auto& view = makeView("dialog_screen");
	ASSERT_NE(find(*mainWindow, "dialog"), nullptr);

	UI::forgetView(mainWindow.get());
	mainWindow.reset(); // Deletes the dialog
	m_views.clear();	// Mustn't touch it again
	(void)view;
}

TEST_F(TestXmlUi, WindowedListBuildsOnlyRowsOnScreen)
{
	installDialogScreens();
	auto& view = makeView("dialog_screen");
	auto& presenter = static_cast<DialogPresenter&>(*view.getPresenter());
	lv_obj_t* rows = find(view, "rows");
	lv_obj_t* tiles = find(view, "tiles");

	presenter.setCount(100);
	EXPECT_EQ(builtRows(rows), (std::vector<int32_t>{0, 1, 2, 3, 4})) << "200px high, 50px rows";
	EXPECT_EQ(builtRows(tiles), (std::vector<int32_t>{0, 1, 2, 3, 4, 5, 6, 7, 8})) << "3 tiles a line, 2 lines";
	lv_obj_t* row2 = find(rows, "2");
	ASSERT_NE(row2, nullptr);
	lv_obj_update_layout(row2);
	EXPECT_EQ(lv_obj_get_y(row2), 100);
	EXPECT_EQ(lv_obj_get_height(row2), 50);
	EXPECT_EQ(lv_obj_get_width(row2), 300) << "no item_width: the list's width";
	EXPECT_EQ(lv_obj_get_x(find(tiles, "5")), 200);

	// Scrolling adds the rows coming on screen; the ones that left go when it stops
	lv_obj_scroll_to_y(rows, 1000, LV_ANIM_OFF);
	EXPECT_NE(find(rows, "20"), nullptr);
	lv_obj_send_event(rows, LV_EVENT_SCROLL_END, nullptr);
	EXPECT_EQ(builtRows(rows), (std::vector<int32_t>{20, 21, 22, 23, 24}));

	// Rows removed from the list go at once
	presenter.setCount(3);
	EXPECT_EQ(builtRows(rows), (std::vector<int32_t>{0, 1, 2}));
	EXPECT_EQ(builtRows(tiles), (std::vector<int32_t>{0, 1, 2}));
}

TEST_F(TestXmlUi, TabSelectedWhenItsSubjectIsSet)
{
	installDialogScreens();
	auto& view = makeView("dialog_screen");
	auto& tabs = static_cast<UI::Xml::DsTabs&>(*UI::LvObj::fromPtr(find(view, "tabs")));
	auto& presenter = static_cast<DialogPresenter&>(*view.getPresenter());
	EXPECT_EQ(tabs.getActiveTabIndex(), 0u);

	presenter.job.set("a.gcode");
	EXPECT_EQ(tabs.getActiveTabIndex(), 2u);

	tabs.setActiveTab(0);
	presenter.job.set("");
	EXPECT_EQ(tabs.getActiveTabIndex(), 0u) << "clearing it selects nothing";
	presenter.job.set("b.gcode");
	EXPECT_EQ(tabs.getActiveTabIndex(), 2u) << "each time it is set";
}

TEST_F(TestXmlUi, ReloadDropsWidgetsBeforeTheirComponentStyles)
{
	// A component's own styles are freed when it is loaded again, so its widgets must already be gone
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/styled_screen.xml"), R"(
		<component>
			<styles>
				<style name="marked" bg_color="0xff0000" bg_opa="100%" border_width="3"/>
			</styles>
			<view extends="lv_obj" width="100%" height="100%">
				<lv_obj name="box" width="100" height="100">
					<style name="marked"/>
				</lv_obj>
			</view>
		</component>)");
	restartWithUserBundles();
	auto& view = makeView("styled_screen");
	for (int i = 0; i < 3; i++)
	{
		UI::Xml::reload();
		lv_refr_now(nullptr);
	}
	lv_obj_t* box = find(view, "box");
	ASSERT_NE(box, nullptr);
	EXPECT_EQ(lv_obj_get_style_border_width(box, LV_PART_MAIN), 3);
}

TEST_F(TestXmlUi, FlashingButtonCanBeDeleted)
{
	// The flash animation must end with the button, not carry on with a deleted one
	writeFile(bundleFile("manifest.json"), manifest());
	writeFile(bundleFile("components/flash_screen.xml"), R"(
		<component presenter="test_dialog">
			<view extends="lv_obj" width="100%" height="100%">
				<ds_button name="flasher" text="Flash" flash_on="busy"/>
			</view>
		</component>)");
	restartWithUserBundles();
	auto& view = makeView("flash_screen");
	static_cast<DialogPresenter&>(*view.getPresenter()).busy.set(1);
	lv_tick_inc(100);
	lv_timer_handler();

	m_views.clear();
	for (int i = 0; i < 10; i++)
	{
		lv_tick_inc(100);
		lv_timer_handler();
	}
}

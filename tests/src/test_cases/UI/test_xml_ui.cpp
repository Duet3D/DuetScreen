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
#include "UI/Styles/Styles.h"
#include "UI/Xml/Data/FanSource.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include <filesystem>
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
				<ds_list name="tools" list="tools" item="tool_card" width="100%" height="content" flex_flow="column"/>
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
} // namespace

class TestXmlUi : public UiTestSuite
{
  protected:
	TestXmlUi()
	{
		UI::Xml::Presenters::add<RecordingPresenter>("test_tools");
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

	UI::Xml::XmlView& makeView(const std::string& component = "fan_screen")
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
		writeFile(bundleFile("components/tool_card.xml"), TOOL_CARD);
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

	fs::path m_userDir;
	std::vector<std::string> m_sent;
	std::vector<std::unique_ptr<UI::Xml::XmlView>> m_views;
};

TEST_F(TestXmlUi, BuiltinBundleLoadsCleanly)
{
	EXPECT_TRUE(UI::Xml::getIssues().empty());

	const auto* screen = UI::Xml::findComponent("fan_screen");
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
	writeFile(bundleFile("components/fan_screen.xml"),
			  R"(<component presenter="nope"><view extends="lv_obj"/></component>)");
	restartWithUserBundles();

	ASSERT_EQ(UI::Xml::getIssues().size(), 1u);
	EXPECT_NE(UI::Xml::getIssues()[0].message.find("unknown presenter 'nope'"), std::string::npos);
	EXPECT_EQ(UI::Xml::findComponent("fan_screen")->presenter, "fans");
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

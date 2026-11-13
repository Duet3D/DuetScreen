/*
 * test_layouts.cpp
 *
 * The widget pool and layouts of the customisable UI (src/UI/Xml/Layout): reading widget and layout files, repairing
 * layouts, and saving them safely. These need no screen; ds_layout and the editor are tested in test_customise.cpp.
 */

#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;
using UI::Xml::CellSize;
using UI::Xml::Layout;
using UI::Xml::LayoutItem;
using UI::Xml::LayoutKind;
using UI::Xml::SettingKind;
using UI::Xml::WidgetDef;
namespace WidgetCatalogue = UI::Xml::WidgetCatalogue;
namespace Layouts = UI::Xml::Layouts;

namespace
{
	/** A widget file as JSON */
	nlohmann::json json(std::string_view text)
	{
		return nlohmann::json::parse(text, nullptr, true, true);
	}

	/** Parse a widget file that must be valid */
	WidgetDef widget(std::string id, std::string_view text)
	{
		std::string error;
		auto parsed = WidgetCatalogue::parse(json(text), std::move(id), error);
		EXPECT_TRUE(parsed.has_value()) << error;
		return parsed.value_or(WidgetDef{});
	}

	/** The error a widget file that must be invalid gives */
	std::string widgetError(std::string_view text)
	{
		std::string error;
		EXPECT_FALSE(WidgetCatalogue::parse(json(text), "bad", error).has_value());
		return error;
	}

	/** Read a file's contents */
	std::string readFile(const fs::path& path)
	{
		std::ifstream file(path);
		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	/** A pool of three widgets: a fan (2x1 to 4x2) with a fan setting, a big graph, and a sidebar button */
	class TestLayouts : public ::testing::Test
	{
	  protected:
		TestLayouts()
		{
			WidgetCatalogue::set({
				widget("fan", R"({"slots": ["dashboard"], "size": {"default": [2, 1], "min": [2, 1], "max": [4, 2]},
								 "settings": [{"name": "fan", "kind": "int", "min": 0, "max": 9, "default": 0}]})"),
				widget("graph", R"({"slots": ["dashboard"], "size": {"default": [4, 3], "min": [3, 2]}})"),
				widget("button", R"({"slots": ["sidebar"]})"),
			});
		}

		~TestLayouts() override { WidgetCatalogue::set({}); }

		/** A 10x6 dashboard holding `items` */
		static Layout dashboard(std::vector<LayoutItem> items)
		{
			return Layout{
				.slot = "dashboard", .kind = LayoutKind::Grid, .columns = 10, .rows = 6, .items = std::move(items)};
		}
	};

	/** Layout store tests, with a local bundle folder of their own */
	class TestLayoutStore : public TestLayouts
	{
	  protected:
		TestLayoutStore()
		{
			m_dir = fs::temp_directory_path() / fmt::format("duetscreen-layouts-{:d}", getpid());
			fs::remove_all(m_dir);
		}

		~TestLayoutStore() override
		{
			Layouts::clear();
			fs::remove_all(m_dir);
		}

		/** Use `files` as the bundles' layouts */
		std::vector<UI::Xml::LoadIssue> use(std::vector<UI::Xml::LayoutFile> files)
		{
			std::vector<UI::Xml::LoadIssue> issues;
			Layouts::set(std::move(files), m_dir, issues);
			return issues;
		}

		fs::path m_dir;
	};

	/** Records the slots it is told about */
	class RecordingListener : public UI::Xml::LayoutListener
	{
	  public:
		void onLayoutChanged(std::string_view slot) override { slots.emplace_back(slot); }

		std::vector<std::string> slots;
	};
} // namespace

TEST_F(TestLayouts, WidgetFileGivesTheWidgetAndItsSettings)
{
	const WidgetDef def = widget("fan", R"({
		"title_tag": "widget.fan.title", "icon": "fan.png", "slots": ["dashboard", "sidebar"],
		"size": {"default": [3, 2], "min": [2, 1], "max": [5, 3]},
		"props": {"style": "card", "big": true},
		"settings": [
			{"name": "fan", "kind": "row", "list": "fans", "key": "number", "label": "name", "default": 0},
			{"name": "seconds", "kind": "choice", "default": 60,
			 "options": [{"value": 30, "title": "30 s"}, {"value": 60, "title_tag": "time.minute"}]}
		]})");

	EXPECT_EQ(def.component, "widget_fan");
	EXPECT_EQ(def.titleTag, "widget.fan.title");
	EXPECT_TRUE(def.fits("sidebar"));
	EXPECT_FALSE(def.fits("status_bar"));
	EXPECT_EQ(def.defaultSize, (CellSize{3, 2}));
	EXPECT_EQ(def.minSize, (CellSize{2, 1}));
	EXPECT_EQ(def.maxSize, (CellSize{5, 3}));
	ASSERT_EQ(def.settings.size(), 2u);
	EXPECT_EQ(def.settings[0].kind, SettingKind::Row);
	EXPECT_EQ(def.settings[0].list, "fans");
	EXPECT_EQ(def.settings[1].options[1].value, "60");
	EXPECT_EQ(def.settings[1].fallback, "60");

	// Props, then each setting's value or its default
	const auto props = def.propsFor(json(R"({"fan": 2, "unknown": 5})"));
	EXPECT_EQ(
		props,
		(std::map<std::string, std::string>{{"style", "card"}, {"big", "true"}, {"fan", "2"}, {"seconds", "60"}}));
}

TEST_F(TestLayouts, InvalidWidgetFilesSayWhatIsWrong)
{
	EXPECT_EQ(widgetError(R"([])"), "isn't a JSON object");
	EXPECT_EQ(widgetError(R"({})"), "\"slots\" must list the layout slots it can be placed in");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "size": {"min": [0, 1]}})"),
			  "size \"min\" must be [columns, rows], each at least 1");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "size": {"default": [1, 1], "min": [2, 2]}})"),
			  "the default size must be between the min and max sizes");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "settings": [{"name": "x", "kind": "colour", "default": 0}]})"),
			  "setting \"x\": unknown kind \"colour\"");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "settings": [{"name": "x", "kind": "int"}]})"),
			  "setting \"x\" has no \"default\"");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "settings": [{"name": "x", "kind": "int", "max": 5, "default": 9}]})"),
			  "setting \"x\": the default isn't one of its values");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "settings": [{"name": "x", "kind": "row", "default": 0}]})"),
			  "setting \"x\": a row setting needs \"list\" and \"key\"");
	EXPECT_EQ(
		widgetError(
			R"({"slots": ["d"], "props": {"x": 1}, "settings": [{"name": "x", "kind": "bool", "default": false}]})"),
		"setting \"x\" is given twice");
}

TEST_F(TestLayouts, SettingsAcceptOnlyTheirValues)
{
	const WidgetDef def = widget("w", R"({"slots": ["d"], "settings": [
		{"name": "n", "kind": "int", "min": 1, "max": 10, "default": 1},
		{"name": "b", "kind": "bool", "default": false},
		{"name": "t", "kind": "text", "max_length": 4, "default": ""},
		{"name": "c", "kind": "choice", "default": "a", "options": [{"value": "a"}, {"value": "b"}]}]})");

	EXPECT_EQ(def.setting("n")->accept(5), json("5"));
	EXPECT_EQ(def.setting("n")->accept("7"), json("7"));
	EXPECT_FALSE(def.setting("n")->accept(11));
	EXPECT_FALSE(def.setting("n")->accept("7x"));
	EXPECT_EQ(def.setting("b")->accept("true"), json("true"));
	EXPECT_FALSE(def.setting("b")->accept(1));
	EXPECT_EQ(def.setting("t")->accept("abcd"), json(R"("abcd")"));
	EXPECT_FALSE(def.setting("t")->accept("abcde"));
	EXPECT_EQ(def.setting("c")->accept("b"), json(R"("b")"));
	EXPECT_FALSE(def.setting("c")->accept("z"));
}

TEST_F(TestLayouts, LayoutFileRoundTrips)
{
	std::string error;
	const auto layout = Layout::fromJson(json(R"({
		"format": 1, "slot": "dashboard", "kind": "grid", "columns": 12, "rows": 8,
		"items": [{"id": "a", "widget": "fan", "x": 1, "y": 2, "w": 3, "h": 1, "settings": {"fan": 2}},
				  {"id": "b", "widget": "graph", "locked": true}]})"),
										 error);
	ASSERT_TRUE(layout.has_value()) << error;
	EXPECT_EQ(layout->columns, 12);
	EXPECT_EQ(layout->items[0].settings["fan"], 2);
	EXPECT_TRUE(layout->items[1].locked);

	const auto again = Layout::fromJson(layout->toJson(), error);
	ASSERT_TRUE(again.has_value()) << error;
	EXPECT_EQ(*again, *layout);
}

TEST_F(TestLayouts, LayoutFilesOfAnUnknownFormatAreRejected)
{
	std::string error;
	EXPECT_FALSE(Layout::fromJson(json(R"({"format": 2, "slot": "s", "kind": "strip", "items": []})"), error));
	EXPECT_EQ(error, "format 2 isn't one this firmware reads (1 to 1)");
	EXPECT_FALSE(Layout::fromJson(json(R"({"format": 1, "slot": "s", "kind": "pile", "items": []})"), error));
	EXPECT_EQ(error, "\"kind\" must be \"grid\" or \"strip\"");
	EXPECT_FALSE(Layout::fromJson(json(R"({"format": 1, "kind": "strip", "items": []})"), error));
	EXPECT_EQ(error, "\"slot\" must name the slot it is for");
}

TEST_F(TestLayouts, RepairMovesOverlappingWidgetsAndKeepsThemInTheGrid)
{
	Layout layout = dashboard({
		{.id = "a", .widget = "graph", .x = 0, .y = 0, .w = 4, .h = 3},
		{.id = "b", .widget = "fan", .x = 2, .y = 1, .w = 2, .h = 1}, // Over a
		{.id = "c", .widget = "fan", .x = 9, .y = 5, .w = 9, .h = 9}, // Too big for the widget and the grid
	});
	const auto changes = Layouts::repair(layout);

	EXPECT_EQ(changes, (std::vector<std::string>{"\"b\" overlapped another widget; it is moved"}));
	EXPECT_EQ(layout.items[1].x, 4); // The first free place, row by row
	EXPECT_EQ(layout.items[1].y, 0);
	EXPECT_EQ(layout.items[2].w, 4); // The fan's largest
	EXPECT_EQ(layout.items[2].h, 2);
	EXPECT_EQ(layout.items[2].x, 6); // Back inside the grid
	EXPECT_EQ(layout.items[2].y, 4);
}

TEST_F(TestLayouts, RepairRemovesWhatCantBeShownAndKeepsMissingWidgets)
{
	Layout layout = dashboard({
		{.id = "a", .widget = "button"},		// A sidebar widget
		{.id = "b", .widget = "not_installed"}, // Kept, for when it is installed again
		{.id = "b", .widget = "fan", .x = 3, .settings = json(R"({"fan": 99, "later": 1})")},
	});
	const auto changes = Layouts::repair(layout);

	EXPECT_EQ(changes,
			  (std::vector<std::string>{"two items are called \"b\"; one is now \"fan\"",
										"\"a\": widget \"button\" can't be placed in \"dashboard\"; it is removed",
										"\"b\": widget \"not_installed\" isn't installed",
										"\"fan\": 99 isn't a value of setting \"fan\"; the default is used"}));
	ASSERT_EQ(layout.items.size(), 2u);
	EXPECT_EQ(layout.items[0].widget, "not_installed");
	EXPECT_EQ(layout.items[1].settings, json(R"({"later": 1})")); // Unknown settings are kept
}

TEST_F(TestLayouts, RepairPutsLockedItemsBack)
{
	Layout locks{
		.slot = "sidebar",
		.kind = LayoutKind::Strip,
		.items = {{.id = "home", .widget = "button"},
				  {.id = "settings", .widget = "button", .settings = json(R"({"page": "settings"})"), .locked = true},
				  {.id = "estop", .widget = "button", .locked = true}}};
	Layout layout{.slot = "sidebar",
				  .kind = LayoutKind::Strip,
				  .items = {{.id = "estop", .widget = "button", .settings = json(R"({"page": "x"})")},
							{.id = "mine", .widget = "button", .locked = true}}};
	const auto changes = Layouts::repair(layout, &locks);

	EXPECT_EQ(changes,
			  (std::vector<std::string>{"\"settings\" can't be removed; it is put back",
										"\"estop\" can't be changed; it is put back"}));
	ASSERT_EQ(layout.items.size(), 3u);
	EXPECT_EQ(layout.items[0].id, "estop"); // Moved, which is allowed
	EXPECT_TRUE(layout.items[0].locked);
	EXPECT_EQ(layout.items[0].settings, json("{}"));
	EXPECT_EQ(layout.items[1].id, "settings"); // Back where it was
	EXPECT_TRUE(layout.items[1].locked);
	EXPECT_FALSE(layout.items[2].locked); // Only the locking layout locks
}

TEST_F(TestLayouts, ResizingTheGridScalesItsItems)
{
	Layout layout = dashboard({
		{.id = "a", .widget = "fan", .x = 0, .y = 0, .w = 4, .h = 2},
		{.id = "b", .widget = "fan", .x = 4, .y = 2, .w = 4, .h = 2},
		{.id = "c", .widget = "graph", .x = 0, .y = 4, .w = 4, .h = 2},
	});
	Layouts::resizeGrid(layout, 5, 3);

	EXPECT_EQ(layout.columns, 5);
	EXPECT_EQ(layout.rows, 3);
	ASSERT_EQ(layout.items.size(), 2u); // The graph's smallest no longer fits beside the others
	EXPECT_EQ((std::array{layout.items[0].x, layout.items[0].y, layout.items[0].w, layout.items[0].h}),
			  (std::array{0, 0, 2, 1})); // Halved
	EXPECT_EQ((std::array{layout.items[1].x, layout.items[1].y, layout.items[1].w, layout.items[1].h}),
			  (std::array{2, 1, 2, 1}));
}

TEST_F(TestLayoutStore, SlotsShowTheLastBundlesLayoutWithTheBuiltinLocks)
{
	const Layout builtin{
		.slot = "sidebar", .kind = LayoutKind::Strip, .items = {{.id = "estop", .widget = "button", .locked = true}}};
	const Layout shared{.slot = "sidebar", .kind = LayoutKind::Strip, .items = {{.id = "a", .widget = "button"}}};
	const Layout own{.slot = "sidebar", .kind = LayoutKind::Strip, .items = {{.id = "b", .widget = "button"}}};
	const auto issues = use({{builtin, "builtin.json"}, {shared, "shared.json"}, {own, "own.json", true}});

	ASSERT_EQ(issues.size(), 2u);
	EXPECT_EQ(issues[0].file, "shared.json");
	EXPECT_EQ(issues[0].message, "\"estop\" can't be removed; it is put back");

	ASSERT_NE(Layouts::defaultFor("sidebar"), nullptr);
	EXPECT_EQ(Layouts::defaultFor("sidebar")->items.size(), 2u); // estop, a
	ASSERT_NE(Layouts::find("sidebar"), nullptr);
	EXPECT_EQ(Layouts::find("sidebar")->items[1].id, "b");
	EXPECT_EQ(Layouts::find("dashboard"), nullptr);
	EXPECT_EQ(Layouts::slots(), (std::vector<std::string>{"sidebar"}));
}

TEST_F(TestLayoutStore, SavingWritesTheLocalBundleAndKeepsTheOldCopy)
{
	const Layout builtin = dashboard({{.id = "a", .widget = "graph", .w = 4, .h = 3}});
	use({{builtin, "builtin.json"}});
	RecordingListener listener;
	Layouts::addListener(&listener);

	Layout mine = builtin;
	mine.items[0].x = 2;
	std::string error;
	ASSERT_TRUE(Layouts::save(mine, error)) << error;
	EXPECT_EQ(listener.slots, (std::vector<std::string>{"dashboard"}));
	EXPECT_EQ(Layouts::find("dashboard")->items[0].x, 2);

	const fs::path file = m_dir / "layouts" / "dashboard.json";
	EXPECT_EQ(json(readFile(file)), mine.toJson());
	EXPECT_TRUE(fs::exists(m_dir / "manifest.json"));
	EXPECT_FALSE(fs::exists(fs::path(file) += ".bak")); // Nothing before it

	mine.items[0].x = 3;
	ASSERT_TRUE(Layouts::save(mine, error)) << error;
	EXPECT_EQ(json(readFile(fs::path(file) += ".bak"))["items"][0]["x"], 2);
	EXPECT_EQ(json(readFile(file))["items"][0]["x"], 3);
	EXPECT_FALSE(fs::exists(fs::path(file) += ".tmp"));

	// Back to the default: the user's own files go, so later changes to the default show
	ASSERT_TRUE(Layouts::save(builtin, error)) << error;
	EXPECT_FALSE(fs::exists(file));
	EXPECT_FALSE(fs::exists(fs::path(file) += ".bak"));
	EXPECT_EQ(*Layouts::find("dashboard"), builtin);
	Layouts::removeListener(&listener);
}

TEST_F(TestLayoutStore, SafeWritesCreateTheFoldersOnTheWay)
{
	const fs::path file = m_dir / "a" / "b" / "file.json";
	std::string error;
	ASSERT_TRUE(Layouts::writeSafely(file, "{}\n", error)) << error;
	EXPECT_EQ(readFile(file), "{}\n");

	// A folder can't be made where a file is
	EXPECT_FALSE(Layouts::writeSafely(file / "inside.json", "{}\n", error));
	EXPECT_FALSE(error.empty());
}

TEST_F(TestLayoutStore, SavedLayoutsAreRepaired)
{
	use({{dashboard({}), "builtin.json"}});
	std::string error;
	ASSERT_TRUE(Layouts::save(dashboard({{.id = "a", .widget = "fan", .x = 9, .w = 1}}), error)) << error;
	EXPECT_EQ(Layouts::find("dashboard")->items[0].x, 8); // 2 wide at least, so it starts a column earlier
	EXPECT_EQ(Layouts::find("dashboard")->items[0].w, 2);
}

namespace
{
	/** Editing tests: a 10x6 dashboard with a graph at 0,0 (4x3) and a fan at 4,0 (2x1), and a sidebar strip of a
	 * button, then a locked button */
	class TestLayoutEditor : public TestLayoutStore
	{
	  protected:
		TestLayoutEditor()
		{
			use({{dashboard({{.id = "graph", .widget = "graph", .w = 4, .h = 3},
							 {.id = "fan", .widget = "fan", .x = 4, .w = 2, .h = 1}}),
				  "dashboard.json"},
				 {Layout{.slot = "sidebar",
						 .kind = LayoutKind::Strip,
						 .items = {{.id = "a", .widget = "button"},
								   {.id = "settings", .widget = "button", .locked = true}}},
				  "sidebar.json"}});
			editor().begin();
		}

		~TestLayoutEditor() override { editor().cancel(); }

		static UI::Xml::LayoutEditor& editor() { return UI::Xml::LayoutEditor::get(); }

		/** The item `id` of `slot` as shown */
		static const LayoutItem& shown(std::string_view slot, std::string_view id)
		{
			static const LayoutItem none;
			const LayoutItem* item = Layouts::find(slot) ? Layouts::find(slot)->find(id) : nullptr;
			return item ? *item : none;
		}

		/** The ids of `slot`'s items as shown, in order */
		static std::vector<std::string> ids(std::string_view slot)
		{
			std::vector<std::string> result;
			for (const auto& item : Layouts::find(slot)->items)
			{
				result.push_back(item.id);
			}
			return result;
		}
	};
} // namespace

TEST_F(TestLayoutEditor, ChangesShowAtOnceAndCancelPutsThemBack)
{
	EXPECT_TRUE(editor().move("dashboard", "fan", 6, 2));
	EXPECT_EQ(shown("dashboard", "fan").x, 6);

	editor().cancel();
	EXPECT_EQ(shown("dashboard", "fan").x, 4);
	EXPECT_FALSE(fs::exists(m_dir / "layouts"));
}

TEST_F(TestLayoutEditor, DoneSavesTheSlotsThatChanged)
{
	EXPECT_TRUE(editor().move("dashboard", "fan", 6, 2));
	std::string error;
	ASSERT_TRUE(editor().done(error)) << error;
	EXPECT_FALSE(editor().isEditing());
	EXPECT_TRUE(fs::exists(m_dir / "layouts" / "dashboard.json"));
	EXPECT_FALSE(fs::exists(m_dir / "layouts" / "sidebar.json"));
	EXPECT_EQ(Layouts::saved("dashboard")->find("fan")->x, 6);
}

TEST_F(TestLayoutEditor, WidgetsOnlyMoveAndGrowWhereTheyFit)
{
	EXPECT_FALSE(editor().resize("dashboard", "graph", 5, 3)); // Into the fan
	EXPECT_FALSE(editor().move("dashboard", "fan", 3, 0));	   // Over the graph
	EXPECT_FALSE(editor().move("dashboard", "fan", 9, 0));	   // Off the grid
	EXPECT_TRUE(editor().move("dashboard", "fan", 8, 5));

	EXPECT_FALSE(editor().resize("dashboard", "fan", 5, 1));  // Wider than the fan can be
	EXPECT_FALSE(editor().resize("dashboard", "fan", 1, 1));  // Narrower than it can be
	EXPECT_TRUE(editor().resize("dashboard", "graph", 5, 4)); // Now there is room
	EXPECT_EQ(shown("dashboard", "graph").w, 5);
	EXPECT_EQ(shown("dashboard", "graph").h, 4);
}

TEST_F(TestLayoutEditor, AddedWidgetsGoInTheSelectedPlace)
{
	editor().selectPlace("dashboard", std::pair{6, 3});
	ASSERT_EQ(editor().add("fan"), "fan_2");
	EXPECT_EQ(shown("dashboard", "fan_2").x, 6);
	EXPECT_EQ(shown("dashboard", "fan_2").y, 3);
	EXPECT_EQ(editor().selection().item, "fan_2");

	// Where it doesn't fit, the first place it does
	editor().selectPlace("dashboard", std::pair{1, 1});
	ASSERT_EQ(editor().add("fan"), "fan_3");
	EXPECT_EQ(shown("dashboard", "fan_3").x, 6);
	EXPECT_EQ(shown("dashboard", "fan_3").y, 0);

	// A strip adds after the selected item; a widget for another slot can't be added
	editor().select("sidebar", "a");
	ASSERT_EQ(editor().add("button"), "button");
	EXPECT_EQ(ids("sidebar"), (std::vector<std::string>{"a", "button", "settings"}));
	EXPECT_EQ(editor().add("graph"), std::nullopt);
}

TEST_F(TestLayoutEditor, LockedItemsMoveButStay)
{
	editor().select("sidebar", "settings");
	EXPECT_FALSE(editor().canRemove());
	EXPECT_FALSE(editor().removeSelected());
	EXPECT_TRUE(editor().moveTo("sidebar", "settings", 0));
	EXPECT_EQ(ids("sidebar"), (std::vector<std::string>{"settings", "a"}));

	editor().select("sidebar", "a");
	EXPECT_TRUE(editor().removeSelected());
	EXPECT_EQ(ids("sidebar"), (std::vector<std::string>{"settings"}));
}

TEST_F(TestLayoutEditor, SettingsTakeOnlyTheirValues)
{
	editor().select("dashboard", "fan");
	EXPECT_TRUE(editor().canConfigure());
	EXPECT_FALSE(editor().setSetting("fan", 12)); // 0 to 9
	EXPECT_FALSE(editor().setSetting("speed", 1));
	EXPECT_TRUE(editor().setSetting("fan", 3));
	EXPECT_EQ(shown("dashboard", "fan").settings["fan"], 3);

	editor().select("dashboard", "graph");
	EXPECT_FALSE(editor().canConfigure()); // It has none
}

TEST_F(TestLayoutEditor, UndoGoesBackOneChangeAndADragIsOne)
{
	editor().beginGesture();
	EXPECT_TRUE(editor().move("dashboard", "fan", 5, 0));
	EXPECT_TRUE(editor().move("dashboard", "fan", 6, 0));
	EXPECT_TRUE(editor().move("dashboard", "fan", 6, 1));
	editor().endGesture();
	EXPECT_TRUE(editor().setGrid("dashboard", 12, 8));
	EXPECT_EQ(Layouts::find("dashboard")->columns, 12);

	editor().undo();
	EXPECT_EQ(Layouts::find("dashboard")->columns, 10);
	EXPECT_EQ(shown("dashboard", "fan").x, 6);
	editor().undo();
	EXPECT_EQ(shown("dashboard", "fan").x, 4);
	EXPECT_FALSE(editor().canUndo());
}

TEST_F(TestLayoutEditor, ResetGoesBackToTheDefault)
{
	EXPECT_TRUE(editor().move("dashboard", "fan", 6, 2));
	std::string error;
	ASSERT_TRUE(editor().done(error)) << error;

	editor().begin();
	editor().reset("dashboard");
	EXPECT_EQ(shown("dashboard", "fan").x, 4);
	ASSERT_TRUE(editor().done(error)) << error;
	EXPECT_FALSE(fs::exists(m_dir / "layouts" / "dashboard.json")); // The default needs no file
}

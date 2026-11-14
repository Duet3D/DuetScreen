/*
 * test_layouts.cpp
 *
 * The widget pool and layouts of the customisable UI (src/UI/Xml/Layout): reading widget and layout files, repairing
 * layouts, and saving them safely. These need no screen; ds_layout and the editor are tested in test_customise.cpp.
 */

#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutLibrary.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/Snap.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;
using UI::Xml::Layout;
using UI::Xml::LayoutItem;
using UI::Xml::LayoutKind;
using UI::Xml::Rect;
using UI::Xml::SettingKind;
using UI::Xml::Size;
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

	/**
	 * A pool of three widgets: a fan (200x100 to 400x200 pixels) with a fan setting, a big graph, and a sidebar
	 * button. Sizes are in hundreds of pixels, so the tests read easily.
	 */
	class TestLayouts : public ::testing::Test
	{
	  protected:
		TestLayouts()
		{
			WidgetCatalogue::set({
				widget("fan", R"({"slots": ["dashboard"], "size": {"default": [200, 100], "min": [200, 100],
								 "max": [400, 200]},
								 "settings": [{"name": "fan", "kind": "int", "min": 0, "max": 9, "default": 0}]})"),
				widget("graph", R"({"slots": ["dashboard"], "size": {"default": [400, 300], "min": [300, 200]}})"),
				widget("button", R"({"slots": ["sidebar"]})"),
			});
		}

		~TestLayouts() override { WidgetCatalogue::set({}); }

		/** A 1000x600 pixel dashboard holding `items` */
		static Layout dashboard(std::vector<LayoutItem> items)
		{
			return Layout{
				.slot = "dashboard", .kind = LayoutKind::Free, .width = 1000, .height = 600, .items = std::move(items)};
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
		"title_tag": "tr.widget.fan.title", "icon": "fan.png", "slots": ["dashboard", "sidebar"],
		"size": {"default": [300, 200], "min": [200, 100], "max": [500, 300]},
		"props": {"style": "card", "big": true},
		"settings": [
			{"name": "fan", "kind": "row", "list": "om.fans", "key": "row.index", "label": "row.name", "default": 0},
			{"name": "seconds", "kind": "choice", "default": 60,
			 "options": [{"value": 30, "title": "30 s"}, {"value": 60, "title_tag": "tr.time.minute"}]}
		]})");

	EXPECT_EQ(def.component, "widget_fan");
	EXPECT_EQ(def.titleTag, "widget.fan.title");
	EXPECT_TRUE(def.fits("sidebar"));
	EXPECT_FALSE(def.fits("status_bar"));
	EXPECT_EQ(def.defaultSize, (Size{300, 200}));
	EXPECT_EQ(def.minSize, (Size{200, 100}));
	EXPECT_EQ(def.maxSize, (Size{500, 300}));
	ASSERT_EQ(def.settings.size(), 2u);
	EXPECT_EQ(def.settings[0].kind, SettingKind::Row);
	EXPECT_EQ(def.settings[0].list, "om.fans");
	EXPECT_EQ(def.settings[0].key, "index") << "kept without its row.";
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
			  "size \"min\" must be [width, height] in pixels, each at least 1");
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
		widgetError(R"({"slots": ["d"], "settings": [{"name": "x", "kind": "row", "list": "om.fans", "key": "index",
		"default": 0}]})"),
		"\"key\" is a field of the rows, written row.<field>");
	EXPECT_EQ(widgetError(R"({"slots": ["d"], "title_tag": "widget.fan"})"),
			  "\"title_tag\" is a translation key, written tr.<key>");
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
		"format": 1, "slot": "dashboard", "kind": "free", "width": 1200, "height": 800, "snap": false,
		"items": [{"id": "a", "widget": "fan", "x": 100, "y": 200, "w": 300, "h": 100, "settings": {"fan": 2}},
				  {"id": "b", "widget": "graph", "locked": true}]})"),
										 error);
	ASSERT_TRUE(layout.has_value()) << error;
	EXPECT_EQ(layout->width, 1200);
	EXPECT_FALSE(layout->snap);
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
	EXPECT_EQ(error, "\"kind\" must be \"free\" or \"strip\"");
	EXPECT_FALSE(Layout::fromJson(json(R"({"format": 1, "slot": "s", "kind": "free", "items": []})"), error));
	EXPECT_EQ(error, "a free layout's \"width\" and \"height\" must give its area, in pixels");
	EXPECT_FALSE(Layout::fromJson(json(R"({"format": 1, "kind": "strip", "items": []})"), error));
	EXPECT_EQ(error, "\"slot\" must name the slot it is for");
}

TEST_F(TestLayouts, RepairMovesOverlappingWidgetsAndKeepsThemInTheArea)
{
	Layout layout = dashboard({
		{.id = "a", .widget = "graph", .x = 0, .y = 0, .w = 400, .h = 300},
		{.id = "b", .widget = "fan", .x = 300, .y = 250, .w = 200, .h = 100}, // Over a's corner
		{.id = "c", .widget = "fan", .x = 900, .y = 500, .w = 900, .h = 900}, // Too big for the widget and the area
	});
	const auto changes = Layouts::repair(layout);

	EXPECT_EQ(changes, (std::vector<std::string>{"\"b\" overlapped another widget; it is moved"}));
	EXPECT_EQ(layout.items[1].rect(), (Rect{.x = 300, .y = 300, .w = 200, .h = 100})); // The nearest free place
	EXPECT_EQ(layout.items[2].rect(), (Rect{.x = 600, .y = 400, .w = 400, .h = 200})); // The fan's largest, inside
}

TEST_F(TestLayouts, RepairRemovesWhatCantBeShownAndKeepsMissingWidgets)
{
	Layout layout = dashboard({
		{.id = "a", .widget = "button"},		// A sidebar widget
		{.id = "b", .widget = "not_installed"}, // Kept, for when it is installed again
		{.id = "b", .widget = "fan", .x = 300, .settings = json(R"({"fan": 99, "later": 1})")},
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

TEST(TestGeometry, FreePlacesAreFoundNearestWhereTheyAreWanted)
{
	namespace Geometry = UI::Xml::Geometry;
	const Size area{1000, 600};
	const std::vector<Rect> taken{{.x = 0, .y = 0, .w = 400, .h = 300}, {.x = 600, .y = 0, .w = 400, .h = 600}};

	// Where it is wanted, if that is free
	EXPECT_EQ(Geometry::findFree(area, taken, {100, 100}, {450, 200}, 5),
			  (Rect{.x = 450, .y = 200, .w = 100, .h = 100}));
	// Else the nearest place, the gap away from what is in the way
	EXPECT_EQ(Geometry::findFree(area, taken, {150, 100}, {300, 100}, 5),
			  (Rect{.x = 405, .y = 100, .w = 150, .h = 100}));
	// The gap is kept wherever there is room for it, even if that is further away
	EXPECT_EQ(Geometry::findFree(area, taken, {200, 100}, {400, 100}, 5),
			  (Rect{.x = 395, .y = 305, .w = 200, .h = 100}));
	// Touching, where only that fits: the 200 pixels between the two, all the way down
	EXPECT_EQ(Geometry::findFree(area, taken, {200, 600}, {0, 0}, 5), (Rect{.x = 400, .y = 0, .w = 200, .h = 600}));
	// Nowhere, if nothing fits
	EXPECT_EQ(Geometry::findFree(area, taken, {300, 400}, {0, 0}, 5), std::nullopt);

	EXPECT_TRUE(Geometry::isFree({.x = 400, .y = 0, .w = 200, .h = 600}, area, taken)); // Touching both
	EXPECT_FALSE(Geometry::isFree({.x = 399, .y = 0, .w = 200, .h = 100}, area, taken));
	EXPECT_FALSE(Geometry::isFree({.x = 900, .y = 500, .w = 200, .h = 200}, area, {})); // Off the area
}

TEST(TestSnap, EdgesSnapToTheAreaAndToOtherWidgets)
{
	namespace Snap = UI::Xml::Snap;
	const Size area{1000, 600};
	const std::vector<Rect> others{{.x = 100, .y = 100, .w = 300, .h = 200}};
	const Size minimum{50, 50};

	// Moved whole near the area's corner, it goes into it
	auto result = Snap::snap({.x = 8, .y = 590 - 100, .w = 100, .h = 100}, {}, area, others, 5, minimum);
	EXPECT_EQ(result.rect, (Rect{.x = 0, .y = 500, .w = 100, .h = 100}));
	EXPECT_EQ(result.guides.size(), 2u);

	// Beside another widget, the gap from it, and lined up with its top
	result = Snap::snap({.x = 410, .y = 96, .w = 100, .h = 100}, {}, area, others, 5, minimum);
	EXPECT_EQ(result.rect, (Rect{.x = 405, .y = 100, .w = 100, .h = 100}));

	// Far from everything, it stays where it is, with no guides
	result = Snap::snap({.x = 600, .y = 400, .w = 100, .h = 100}, {}, area, others, 5, minimum);
	EXPECT_EQ(result.rect, (Rect{.x = 600, .y = 400, .w = 100, .h = 100}));
	EXPECT_TRUE(result.guides.empty());

	// Resized, only the edges dragged snap: the right edge to the other's left, the gap away
	result = Snap::snap({.x = 0, .y = 400, .w = 88, .h = 100}, {.right = true}, area, others, 5, minimum);
	EXPECT_EQ(result.rect, (Rect{.x = 0, .y = 400, .w = 95, .h = 100}));
	ASSERT_EQ(result.guides.size(), 1u);
	EXPECT_EQ(result.guides[0], (Snap::Guide{.vertical = true, .at = 95}));

	// A snap that would make it smaller than it can be isn't made
	result = Snap::snap({.x = 60, .y = 400, .w = 52, .h = 100}, {.left = true}, area, others, 5, minimum);
	EXPECT_EQ(result.rect.x, 60);
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
	const Layout builtin = dashboard({{.id = "a", .widget = "graph", .w = 400, .h = 300}});
	use({{builtin, "builtin.json"}});
	RecordingListener listener;
	Layouts::addListener(&listener);

	Layout mine = builtin;
	mine.items[0].x = 200;
	std::string error;
	ASSERT_TRUE(Layouts::save(mine, error)) << error;
	EXPECT_EQ(listener.slots, (std::vector<std::string>{"dashboard"}));
	EXPECT_EQ(Layouts::find("dashboard")->items[0].x, 200);

	const fs::path file = m_dir / "layouts" / "dashboard.json";
	EXPECT_EQ(json(readFile(file)), mine.toJson());
	EXPECT_TRUE(fs::exists(m_dir / "manifest.json"));
	EXPECT_FALSE(fs::exists(fs::path(file) += ".bak")); // Nothing before it

	mine.items[0].x = 300;
	ASSERT_TRUE(Layouts::save(mine, error)) << error;
	EXPECT_EQ(json(readFile(fs::path(file) += ".bak"))["items"][0]["x"], 200);
	EXPECT_EQ(json(readFile(file))["items"][0]["x"], 300);
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
	ASSERT_TRUE(Layouts::save(dashboard({{.id = "a", .widget = "fan", .x = 900, .w = 100, .h = 100}}), error)) << error;
	EXPECT_EQ(Layouts::find("dashboard")->items[0].x, 800); // 200 wide at least, so it starts further left
	EXPECT_EQ(Layouts::find("dashboard")->items[0].w, 200);
}

namespace
{
	/** Editing tests: a 1000x600 dashboard with a graph at 0,0 (400x300) and a fan at 400,0 (200x100), and a sidebar
	 * strip of a button, then a locked button */
	class TestLayoutEditor : public TestLayoutStore
	{
	  protected:
		TestLayoutEditor()
		{
			use({{dashboard({{.id = "graph", .widget = "graph", .w = 400, .h = 300},
							 {.id = "fan", .widget = "fan", .x = 400, .w = 200, .h = 100}}),
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

		/** Move free item `id` of the dashboard so its corner is at (x, y). @return whether it fits there */
		static bool move(std::string_view id, int32_t x, int32_t y)
		{
			Rect rect = shown("dashboard", id).rect();
			rect.x = x;
			rect.y = y;
			return editor().place("dashboard", std::string(id), rect);
		}

		/** Resize free item `id` of the dashboard to w by h. @return whether that fits */
		static bool resize(std::string_view id, int32_t w, int32_t h)
		{
			Rect rect = shown("dashboard", id).rect();
			rect.w = w;
			rect.h = h;
			return editor().place("dashboard", std::string(id), rect);
		}

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
	EXPECT_TRUE(move("fan", 600, 200));
	EXPECT_EQ(shown("dashboard", "fan").x, 600);

	editor().cancel();
	EXPECT_EQ(shown("dashboard", "fan").x, 400);
	EXPECT_FALSE(fs::exists(m_dir / "layouts"));
}

TEST_F(TestLayoutEditor, DoneSavesTheSlotsThatChanged)
{
	EXPECT_TRUE(move("fan", 600, 200));
	std::string error;
	ASSERT_TRUE(editor().done(error)) << error;
	EXPECT_FALSE(editor().isEditing());
	EXPECT_TRUE(fs::exists(m_dir / "layouts" / "dashboard.json"));
	EXPECT_FALSE(fs::exists(m_dir / "layouts" / "sidebar.json"));
	EXPECT_EQ(Layouts::saved("dashboard")->find("fan")->x, 600);
}

TEST_F(TestLayoutEditor, WidgetsOnlyMoveAndGrowWhereTheyFit)
{
	EXPECT_FALSE(resize("graph", 450, 300)); // Into the fan
	EXPECT_FALSE(move("fan", 350, 0));		 // Over the graph
	EXPECT_FALSE(move("fan", 850, 0));		 // Off the area
	EXPECT_TRUE(move("fan", 800, 500));

	EXPECT_FALSE(resize("fan", 450, 100));	// Wider than the fan can be
	EXPECT_FALSE(resize("fan", 150, 100));	// Narrower than it can be
	EXPECT_TRUE(resize("graph", 517, 413)); // Now there is room, to the pixel
	EXPECT_EQ(shown("dashboard", "graph").w, 517);
	EXPECT_EQ(shown("dashboard", "graph").h, 413);
}

TEST_F(TestLayoutEditor, AddedWidgetsGoInTheSelectedPlace)
{
	editor().selectPlace("dashboard", UI::Xml::Point{600, 300});
	ASSERT_EQ(editor().add("fan"), "fan_2");
	EXPECT_EQ(shown("dashboard", "fan_2").rect(), (Rect{.x = 600, .y = 300, .w = 200, .h = 100}));
	EXPECT_EQ(editor().selection().item, "fan_2");

	// Where it doesn't fit, as near as it does: beside the graph, below the fan
	editor().selectPlace("dashboard", UI::Xml::Point{350, 50});
	ASSERT_EQ(editor().add("fan"), "fan_3");
	EXPECT_FALSE(shown("dashboard", "fan_3").rect().overlaps(shown("dashboard", "graph").rect()));
	EXPECT_FALSE(shown("dashboard", "fan_3").rect().overlaps(shown("dashboard", "fan").rect()));
	EXPECT_LE(std::abs(shown("dashboard", "fan_3").x - 350) + std::abs(shown("dashboard", "fan_3").y - 50), 120);

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
	EXPECT_TRUE(move("fan", 500, 0));
	EXPECT_TRUE(move("fan", 600, 0));
	EXPECT_TRUE(move("fan", 600, 100));
	editor().endGesture();
	EXPECT_TRUE(editor().setSnap("dashboard", false));
	EXPECT_FALSE(Layouts::find("dashboard")->snap);

	editor().undo();
	EXPECT_TRUE(Layouts::find("dashboard")->snap);
	EXPECT_EQ(shown("dashboard", "fan").x, 600);
	editor().undo();
	EXPECT_EQ(shown("dashboard", "fan").x, 400);
	EXPECT_FALSE(editor().canUndo());
}

TEST_F(TestLayoutEditor, ResetGoesBackToTheDefault)
{
	EXPECT_TRUE(move("fan", 600, 200));
	std::string error;
	ASSERT_TRUE(editor().done(error)) << error;

	editor().begin();
	editor().reset("dashboard");
	EXPECT_EQ(shown("dashboard", "fan").x, 400);
	ASSERT_TRUE(editor().done(error)) << error;
	EXPECT_FALSE(fs::exists(m_dir / "layouts" / "dashboard.json")); // The default needs no file
}

TEST_F(TestLayoutStore, SavedLayoutsAreListedLoadedAndDeletedByName)
{
	namespace Library = UI::Xml::LayoutLibrary;
	const Layout layout = dashboard({{.id = "a", .widget = "graph", .w = 400, .h = 300}});
	std::string error;
	Library::setFolder({});
	EXPECT_FALSE(Library::canSave());
	EXPECT_FALSE(Library::save("Mine", {layout}, error));
	EXPECT_TRUE(Library::names().empty());

	const fs::path folder = m_dir / "saved";
	Library::setFolder(folder);
	ASSERT_TRUE(Library::canSave());
	ASSERT_TRUE(Library::save("Mine", {layout}, error)) << error;
	ASSERT_TRUE(Library::save("Another one", {}, error)) << error;
	EXPECT_EQ(Library::names(), (std::vector<std::string>{"Another one", "Mine"}));
	EXPECT_EQ(json(readFile(folder / "Mine.json"))["name"], "Mine");

	auto loaded = Library::load("Mine", error);
	ASSERT_TRUE(loaded) << error;
	EXPECT_EQ(*loaded, std::vector<Layout>{layout});
	EXPECT_FALSE(Library::load("Missing", error));
	EXPECT_FALSE(error.empty());

	// A file that isn't a saved layout says so
	std::ofstream(folder / "Broken.json") << "{\"format\": 99}";
	EXPECT_FALSE(Library::load("Broken", error));
	EXPECT_NE(error.find("Broken"), std::string::npos);

	EXPECT_TRUE(Library::remove("Mine"));
	EXPECT_FALSE(Library::remove("Mine"));
	EXPECT_EQ(Library::names(), (std::vector<std::string>{"Another one", "Broken"}));
	Library::setFolder({});
}

TEST(TestLayoutLibrary, NamesAreWhatAFileCanBeCalledEverywhere)
{
	namespace Library = UI::Xml::LayoutLibrary;
	for (const char* name : {"Printing", "my layout 2", "a-b_c", "x"})
	{
		EXPECT_TRUE(Library::isValidName(name)) << name;
	}
	for (const char* name : {"", "   ", "a/b", "..", "a.b", "a\\b", "caf\xc3\xa9"})
	{
		EXPECT_FALSE(Library::isValidName(name)) << name;
	}
	EXPECT_TRUE(Library::isValidName(std::string(Library::MAX_NAME, 'a')));
	EXPECT_FALSE(Library::isValidName(std::string(Library::MAX_NAME + 1, 'a')));
}

TEST_F(TestLayoutEditor, ApplyingSavedLayoutsIsOneChange)
{
	Layout saved = *editor().draft("dashboard");
	saved.items.erase(saved.items.begin()); // No graph
	const Layout otherKind{.slot = "sidebar", .kind = LayoutKind::Free, .width = 1000, .height = 600};
	const Layout notShown{.slot = "elsewhere", .kind = LayoutKind::Free, .width = 1000, .height = 600};

	EXPECT_EQ(editor().apply({saved, otherKind, notShown}), 1u);
	EXPECT_EQ(ids("dashboard"), (std::vector<std::string>{"fan"}));
	EXPECT_EQ(ids("sidebar"), (std::vector<std::string>{"a", "settings"}));
	const auto drafts = editor().drafts();
	ASSERT_EQ(drafts.size(), 2u);
	EXPECT_EQ(drafts[0].slot, "dashboard");

	editor().undo();
	EXPECT_EQ(ids("dashboard"), (std::vector<std::string>{"graph", "fan"}));
}

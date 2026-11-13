/*
 * test_customise.cpp
 *
 * The customisable screen: <ds_layout> showing a slot's layout and following its changes. The widget pool and layout
 * files themselves are tested in test_layouts.cpp.
 */

#include "UI/Core/Navigation.h"
#include "UI/Widgets/Layout/DsLayout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace Layouts = UI::Xml::Layouts;
using UI::Xml::Layout;

namespace
{
	void writeFile(const fs::path& path, std::string_view text)
	{
		fs::create_directories(path.parent_path());
		std::ofstream(path) << text;
	}

	/* A bundle with a `test` slot: a label widget whose text is a setting, and a 4x2 grid layout of two labels */
	constexpr std::string_view MANIFEST = R"({"id": "test", "ui_api": "1.0", "components": ["*.xml"],
		"widgets": ["*.widget.json"], "layouts": ["*.layout.json"]})";

	constexpr std::string_view LABEL = R"(
		<component>
			<api><prop name="text" type="string"/></api>
			<view extends="lv_label" text="$text"/>
		</component>)";

	constexpr std::string_view LABEL_WIDGET =
		R"({"component": "test_label", "slots": ["test"], "settings": [{"name": "text", "kind": "text", "default": "Hi"}]})";

	constexpr std::string_view LAYOUT = R"({"format": 1, "slot": "test", "kind": "grid", "columns": 4, "rows": 2,
		"items": [{"id": "a", "widget": "label", "x": 0, "y": 0, "w": 2, "h": 1},
				  {"id": "b", "widget": "label", "x": 2, "y": 1, "w": 2, "h": 1, "settings": {"text": "Bye"}}]})";

	constexpr std::string_view SCREEN = R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%" style_pad_all="0">
				<ds_layout name="layout" slot="test" width="400" height="200" style_pad_all="0" style_pad_gap="0"/>
			</view>
		</component>)";
} // namespace

class TestCustomise : public UiTestSuite
{
  protected:
	TestCustomise()
	{
		m_userDir = fs::temp_directory_path() / fmt::format("duetscreen-customise-{:d}", getpid());
		fs::remove_all(m_userDir);
		writeFile(m_userDir / "test/manifest.json", MANIFEST);
		writeFile(m_userDir / "test/test_label.xml", LABEL);
		writeFile(m_userDir / "test/test_screen.xml", SCREEN);
		writeFile(m_userDir / "test/label.widget.json", LABEL_WIDGET);
		writeFile(m_userDir / "test/test.layout.json", LAYOUT);
		m_view.reset();
		UI::Xml::deinit();
		UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
		EXPECT_TRUE(UI::Xml::getIssues().empty());
		m_view = std::make_unique<UI::Xml::XmlView>("screen", screen, "test_screen");
		m_view->show();
		lv_obj_update_layout(m_view->getRootPtr());
	}

	~TestCustomise() override
	{
		m_view.reset();
		fs::remove_all(m_userDir);
	}

	/** The ds_layout widget */
	lv_obj_t* layout() const { return lv_obj_find_by_name(m_view->getRootPtr(), "layout"); }

	/** The widget placed as item `id` */
	lv_obj_t* item(const char* id) const { return lv_obj_find_by_name(layout(), id); }

	/** The text of the label placed as item `id` */
	std::string text(const char* id) const { return lv_label_get_text(item(id)); }

	/** Save `layout` as the user's own */
	static void save(const Layout& layout)
	{
		std::string error;
		EXPECT_TRUE(Layouts::save(layout, error)) << error;
	}

	fs::path m_userDir;
	std::unique_ptr<UI::Xml::XmlView> m_view;
};

TEST_F(TestCustomise, LayoutShowsItsWidgetsWithTheirSettings)
{
	ASSERT_EQ(lv_obj_get_child_count(layout()), 2u);
	EXPECT_EQ(text("a"), "Hi"); // The setting's default
	EXPECT_EQ(text("b"), "Bye");

	// A 4x2 grid of 100x100 cells
	lv_area_t area;
	lv_obj_get_coords(item("b"), &area);
	lv_area_t parent;
	lv_obj_get_coords(layout(), &parent);
	EXPECT_EQ(area.x1 - parent.x1, 200);
	EXPECT_EQ(area.y1 - parent.y1, 100);
	EXPECT_EQ(lv_area_get_width(&area), 200);
	EXPECT_EQ(lv_area_get_height(&area), 100);
}

TEST_F(TestCustomise, LayoutRebuildsOnlyWhatChanged)
{
	lv_obj_t* a = item("a");
	lv_obj_t* b = item("b");

	Layout changed = *Layouts::find("test");
	changed.items[0].x = 2; // Above b
	changed.items[1].settings["text"] = "Again";
	save(changed);
	lv_obj_update_layout(layout());

	EXPECT_EQ(item("a"), a); // Moved, so kept
	EXPECT_NE(item("b"), b); // Its settings changed, so built again
	EXPECT_EQ(text("b"), "Again");
	lv_area_t area;
	lv_obj_get_coords(item("a"), &area);
	lv_area_t parent;
	lv_obj_get_coords(layout(), &parent);
	EXPECT_EQ(area.x1 - parent.x1, 200);

	changed.items.erase(changed.items.begin());
	save(changed);
	EXPECT_EQ(item("a"), nullptr);
	EXPECT_EQ(lv_obj_get_child_count(layout()), 1u);
}

TEST_F(TestCustomise, StripKeepsTheLayoutsOrder)
{
	Layout strip{.slot = "test", .kind = UI::Xml::LayoutKind::Strip};
	strip.items = {{.id = "x", .widget = "label"}, {.id = "y", .widget = "label"}};
	save(strip);
	lv_obj_t* x = item("x");
	EXPECT_EQ(lv_obj_get_index(x), 0);

	std::swap(strip.items[0], strip.items[1]);
	save(strip);
	EXPECT_EQ(item("x"), x);
	EXPECT_EQ(lv_obj_get_index(x), 1);
}

TEST_F(TestCustomise, MissingWidgetKeepsItsPlace)
{
	Layout changed = *Layouts::find("test");
	changed.items[0].widget = "gone";
	save(changed);

	lv_obj_t* placeholder = item("a");
	ASSERT_NE(placeholder, nullptr);
	ASSERT_EQ(lv_obj_get_child_count(placeholder), 2u);
	EXPECT_STREQ(lv_label_get_text(lv_obj_get_child(placeholder, 1)), "gone");
}

TEST_F(TestCustomise, SidebarIsALayoutWithItsSettingsAndEstopLocked)
{
	const Layout* sidebar = Layouts::find("sidebar");
	ASSERT_NE(sidebar, nullptr);
	std::vector<std::string> ids;
	for (const auto& item : sidebar->items)
	{
		ids.push_back(item.id);
	}
	EXPECT_EQ(ids, (std::vector<std::string>{"home", "control", "files", "console", "settings", "estop"}));
	EXPECT_TRUE(sidebar->find("settings")->locked);
	EXPECT_TRUE(sidebar->find("estop")->locked);
	EXPECT_FALSE(sidebar->find("home")->locked);
}

/**
 * Customising the home screen as the user does: from Settings, with touches on the widgets and the toolbar. The
 * changes are saved in a user bundle folder of the test's own.
 */
class TestCustomiseHome : public UiTestSuite
{
  protected:
	TestCustomiseHome()
	{
		m_userDir = fs::temp_directory_path() / fmt::format("duetscreen-customise-home-{:d}", getpid());
		fs::remove_all(m_userDir);
		UI::Xml::deinit();
		UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
		m_home = std::make_unique<UI::Xml::XmlView>("home", lv_screen_active(), "home");
		m_home->show();
		update();
	}

	~TestCustomiseHome() override
	{
		lv_test_mouse_release();
		read();
		UI::Xml::LayoutEditor::get().cancel();
		m_home.reset();
		fs::remove_all(m_userDir);
	}

	/** Lay the screen out */
	static void update() { lv_obj_update_layout(lv_screen_active()); }

	/** Let the pointer report where it is */
	static void read()
	{
		lv_indev_read(lv_test_indev_get_indev(LV_INDEV_TYPE_POINTER));
		update();
	}

	/** The middle of `obj` on the screen */
	static lv_point_t middleOf(lv_obj_t* obj)
	{
		lv_area_t area;
		lv_obj_get_coords(obj, &area);
		return {(area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2};
	}

	/** Touch `from`, drag to `to` in small steps, and let go */
	static void drag(lv_point_t from, lv_point_t to)
	{
		lv_test_mouse_move_to(from.x, from.y);
		lv_test_mouse_press();
		read();
		for (int step = 1; step <= 10; step++)
		{
			lv_test_mouse_move_to(from.x + (to.x - from.x) * step / 10, from.y + (to.y - from.y) * step / 10);
			read();
		}
		lv_test_mouse_release();
		read();
	}

	/** Tap `point` */
	static void tap(lv_point_t point) { drag(point, point); }

	/** Tap the toolbar's button called `name` */
	void tapToolbar(const char* name) const { tap(middleOf(lv_obj_find_by_name(toolbar(), name))); }

	/** The toolbar, which shows while customising */
	lv_obj_t* toolbar() const { return lv_obj_find_by_name(UI::findView("customise")->getRootPtr(), "toolbar"); }

	/** The placed widget `id` in `slot` */
	static lv_obj_t* placed(const char* slot, const char* id)
	{
		const auto* layout = UI::Xml::DsLayout::find(slot);
		return layout ? layout->findItem(id) : nullptr;
	}

	/** The item `id` of `slot` being edited */
	static const UI::Xml::LayoutItem* draftItem(const char* slot, const char* id)
	{
		const Layout* layout = UI::Xml::LayoutEditor::get().draft(slot);
		return layout ? layout->find(id) : nullptr;
	}

	/** Whether `obj` and every widget it is in are shown */
	static bool shownOnScreen(lv_obj_t* obj)
	{
		for (; obj != nullptr; obj = lv_obj_get_parent(obj))
		{
			if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
			{
				return false;
			}
		}
		return true;
	}

	/** Start customising, as the Settings button does */
	void start() const
	{
		UI::Xml::Actions::dispatch(m_home->getRootPtr(), "customise.start", "");
		update();
	}

	fs::path m_userDir;
	std::unique_ptr<UI::Xml::XmlView> m_home;
};

TEST_F(TestCustomiseHome, CustomisingCoversEveryWidgetAndShowsTheToolbar)
{
	EXPECT_FALSE(shownOnScreen(toolbar()));
	EXPECT_EQ(lv_obj_find_by_name(placed("dashboard", "tools"), "customise_overlay"), nullptr);

	start();
	EXPECT_TRUE(UI::Xml::LayoutEditor::get().isEditing());
	EXPECT_TRUE(shownOnScreen(toolbar()));
	for (const auto& [slot, id] : std::vector<std::pair<const char*, const char*>>{
			 {"dashboard", "tools"}, {"dashboard", "files"}, {"sidebar", "home"}, {"sidebar", "estop"}})
	{
		EXPECT_NE(lv_obj_find_by_name(placed(slot, id), "customise_overlay"), nullptr) << slot << "/" << id;
	}
	tap(middleOf(placed("dashboard", "temperature_graph")));
	EXPECT_EQ(UI::Xml::LayoutEditor::get().selection().item, "temperature_graph");
	EXPECT_EQUAL_SCREENSHOT("customise/editing.png")

	tapToolbar("cancel");
	EXPECT_FALSE(UI::Xml::LayoutEditor::get().isEditing());
	EXPECT_FALSE(shownOnScreen(toolbar()));
	EXPECT_EQ(lv_obj_find_by_name(placed("dashboard", "tools"), "customise_overlay"), nullptr);
}

TEST_F(TestCustomiseHome, WidgetsAreRemovedMovedAndResizedByTouch)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");
	EXPECT_EQ(draftItem("dashboard", "temperature_graph"), nullptr);
	EXPECT_EQ(placed("dashboard", "temperature_graph"), nullptr);

	// Down two rows: a row is a sixth of the dashboard
	lv_obj_t* dashboard = UI::Xml::DsLayout::find("dashboard")->getContainer();
	const int32_t row = lv_obj_get_content_height(dashboard) / 6;
	const lv_point_t middle = middleOf(placed("dashboard", "tools"));
	drag(middle, {middle.x, middle.y + 2 * row});
	ASSERT_NE(draftItem("dashboard", "tools"), nullptr);
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 2);

	// Its handle, at the bottom right, makes it a row taller
	const lv_point_t handle = middleOf(lv_obj_find_by_name(placed("dashboard", "tools"), "handle"));
	drag(handle, {handle.x, handle.y + row});
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 4);

	tapToolbar("undo");
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 3);
}

TEST_F(TestCustomiseHome, WidgetsAreAddedFromThePoolWhereTheUserTapped)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");

	// The empty bottom left cell
	lv_obj_t* dashboard = UI::Xml::DsLayout::find("dashboard")->getContainer();
	lv_area_t area;
	lv_obj_get_content_coords(dashboard, &area);
	tap({area.x1 + 20, area.y2 - 20});
	EXPECT_EQ(UI::Xml::LayoutEditor::get().selection().cell, (std::pair{0, 5}));
	ASSERT_NE(lv_obj_find_by_name(dashboard, "customise_place"), nullptr);

	tap(middleOf(lv_obj_find_by_name(dashboard, "customise_place")));
	lv_obj_t* pool = lv_obj_find_by_name(lv_screen_active(), "catalogue_dialog");
	ASSERT_TRUE(shownOnScreen(pool));
	EXPECT_EQUAL_SCREENSHOT("customise/widget_pool.png")

	// The pool lists the dashboard's widgets by name; the axis positions are 4x1
	lv_obj_t* list = lv_obj_find_by_name(pool, "list");
	lv_obj_t* axes = nullptr;
	for (uint32_t i = 0; i < lv_obj_get_child_count(list); i++)
	{
		lv_obj_t* item = lv_obj_get_child(list, static_cast<int32_t>(i));
		if (std::string_view(lv_label_get_text(lv_obj_find_by_name(item, "title"))) == "Axis positions")
		{
			axes = item;
		}
	}
	ASSERT_NE(axes, nullptr);
	tap(middleOf(axes));
	EXPECT_FALSE(shownOnScreen(pool));
	const auto* added = draftItem("dashboard", "axes");
	ASSERT_NE(added, nullptr);
	EXPECT_EQ((std::array{added->x, added->y, added->w, added->h}), (std::array{0, 5, 4, 1}));
	EXPECT_NE(placed("dashboard", "axes"), nullptr);
}

TEST_F(TestCustomiseHome, SettingsAreChangedInTheirDialogs)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("configure");
	lv_obj_t* dialog = lv_obj_find_by_name(lv_screen_active(), "settings_dialog");
	ASSERT_TRUE(shownOnScreen(dialog));
	EXPECT_EQUAL_SCREENSHOT("customise/settings.png")

	// The time shown: its values, then 5 minutes
	tap(middleOf(lv_obj_find_by_name(dialog, "value")));
	lv_obj_t* options = lv_obj_find_by_name(lv_screen_active(), "options_dialog");
	ASSERT_TRUE(shownOnScreen(options));
	lv_obj_t* list = lv_obj_find_by_name(options, "list");
	ASSERT_EQ(lv_obj_get_child_count(list), 5u);
	tap(middleOf(lv_obj_get_child(list, 3)));
	EXPECT_FALSE(shownOnScreen(options));
	EXPECT_EQ(draftItem("dashboard", "temperature_graph")->settings["seconds"], "300");
	EXPECT_STREQ(lv_label_get_text(lv_obj_get_child(lv_obj_find_by_name(dialog, "value"), -1)), "5 min");
}

TEST_F(TestCustomiseHome, TheSidebarIsReorderedButKeepsSettingsAndTheEstop)
{
	start();
	tap(middleOf(placed("sidebar", "settings")));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(toolbar(), "remove"), LV_STATE_DISABLED));

	// Home dragged past the middle of Files
	const lv_point_t files = middleOf(placed("sidebar", "files"));
	drag(middleOf(placed("sidebar", "home")), {files.x, files.y + 10});
	const Layout* sidebar = UI::Xml::LayoutEditor::get().draft("sidebar");
	ASSERT_NE(sidebar, nullptr);
	EXPECT_EQ(sidebar->items[0].id, "control");
	EXPECT_EQ(sidebar->items[2].id, "home");
}

TEST_F(TestCustomiseHome, DoneKeepsTheChangesAndCancelDoesNot)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");
	tapToolbar("cancel");
	EXPECT_NE(placed("dashboard", "temperature_graph"), nullptr);
	EXPECT_FALSE(fs::exists(m_userDir / "local"));

	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");
	tapToolbar("done");
	EXPECT_EQ(placed("dashboard", "temperature_graph"), nullptr);
	EXPECT_TRUE(fs::exists(m_userDir / "local" / "layouts" / "dashboard.json"));

	// It is there after a restart
	m_home.reset();
	UI::Xml::deinit();
	UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
	EXPECT_EQ(Layouts::find("dashboard")->find("temperature_graph"), nullptr);
}

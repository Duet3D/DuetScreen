/*
 * test_customise.cpp
 *
 * The customisable screen: <ds_layout> showing a slot's layout and following its changes. The widget pool and layout
 * files themselves are tested in test_layouts.cpp.
 */

#include "UI/Core/Navigation.h"
#include "UI/Styles/Theme.h"
#include "UI/Widgets/Layout/DsLayout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/WidgetCatalogue.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "test_utils/Dialogs.h"
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

	/* A bundle with a `test` slot: a label widget whose text is a setting, and a free layout of two labels */
	constexpr std::string_view MANIFEST = R"({"id": "test", "ui_api": "1.0", "components": ["*.xml"],
		"widgets": ["widgets/*.json"], "layouts": ["*.layout.json"], "themes": ["themes/*.xml"]})";

	constexpr std::string_view LABEL = R"(
		<component>
			<api><prop name="text" type="string"/></api>
			<view extends="lv_label" text="$text"/>
		</component>)";

	constexpr std::string_view LABEL_WIDGET =
		R"({"component": "test_label", "slots": ["test"], "settings": [{"name": "text", "kind": "text", "default": "Hi"}]})";

	/* A widget showing which size classes its place has: `wide` unless compact across, `tall` unless compact down,
	   `expanded` if expanded across, and `upright` if portrait */
	constexpr std::string_view SIZED = R"(
		<component>
			<view extends="lv_obj">
				<lv_obj name="wide"><bind_flag_if cond="widget.width_class eq 0" flag="hidden"/></lv_obj>
				<lv_obj name="tall"><bind_flag_if cond="widget.height_class eq 0" flag="hidden"/></lv_obj>
				<lv_obj name="expanded"><bind_flag_if cond="widget.width_class lt 2" flag="hidden"/></lv_obj>
				<lv_obj name="upright"><bind_flag_if cond="widget.portrait" flag="hidden" invert="true"/></lv_obj>
			</view>
		</component>)";

	/* A theme whose widgets need more room to be regular across: 250 pixels rather than the base theme's 180 */
	constexpr std::string_view ROOMY_THEME = R"(
		<theme name="roomy" extends="duetscreen">
			<consts><const name="widget_regular_min" value="250"/></consts>
		</theme>)";

	constexpr std::string_view SIZED_WIDGET = R"({"component": "test_sized", "slots": ["test"]})";

	constexpr std::string_view LAYOUT = R"({"format": 1, "slot": "test", "kind": "free", "width": 400, "height": 200,
		"items": [{"id": "a", "widget": "label", "x": 0, "y": 0, "w": 200, "h": 100},
				  {"id": "b", "widget": "label", "x": 200, "y": 100, "w": 200, "h": 100, "settings": {"text": "Bye"}}]})";

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
		writeFile(m_userDir / "test/widgets/label.json", LABEL_WIDGET);
		writeFile(m_userDir / "test/test_sized.xml", SIZED);
		writeFile(m_userDir / "test/widgets/sized.json", SIZED_WIDGET);
		writeFile(m_userDir / "test/themes/roomy.xml", ROOMY_THEME);
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

	/** Whether the child `name` of `widget` is shown */
	static bool shown(lv_obj_t* widget, const char* name)
	{
		lv_obj_t* child = lv_obj_find_by_name(widget, name);
		EXPECT_NE(child, nullptr) << name;
		return child != nullptr && !lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN);
	}

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

	// At its rectangle, in pixels
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
	changed.items[0].x = 200; // Above b
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

TEST_F(TestCustomise, WidgetsFollowTheSizeClassesOfTheirCells)
{
	Layout changed = *Layouts::find("test");
	changed.items[0].widget = "sized"; // 200x100: regular across and compact down
	save(changed);
	lv_obj_update_layout(layout());
	lv_obj_t* sized = item("a");
	EXPECT_TRUE(shown(sized, "wide"));
	EXPECT_FALSE(shown(sized, "tall"));
	EXPECT_FALSE(shown(sized, "expanded"));
	EXPECT_FALSE(shown(sized, "upright"));

	// Resized, it is kept, and follows its new size: 100x200
	changed.items[0].w = 100;
	changed.items[0].h = 200;
	save(changed);
	lv_obj_update_layout(layout());
	EXPECT_EQ(item("a"), sized);
	EXPECT_FALSE(shown(sized, "wide"));
	EXPECT_TRUE(shown(sized, "tall"));
	EXPECT_TRUE(shown(sized, "upright"));

	// 400x100: expanded across, from the base theme's widget_expanded_min
	changed.items[0].w = 400;
	changed.items[0].h = 100;
	changed.items[1].y = 100; // Out of the way, below
	changed.items[1].x = 0;
	save(changed);
	lv_obj_update_layout(layout());
	EXPECT_TRUE(shown(sized, "expanded"));
}

TEST_F(TestCustomise, SizeClassesFollowTheThemesBreakpoints)
{
	Layout changed = *Layouts::find("test");
	changed.items[0].widget = "sized"; // 200 wide: regular in the base theme
	save(changed);
	lv_obj_update_layout(layout());
	lv_obj_t* sized = item("a");
	ASSERT_TRUE(shown(sized, "wide"));

	UI::Themes::Theme* original = UI::Themes::getCurrentTheme();
	UI::Themes::Theme* roomy = UI::Themes::getThemeByName("roomy");
	ASSERT_NE(roomy, nullptr);
	roomy->setThemeActive();
	EXPECT_EQ(item("a"), sized); // Not built again
	EXPECT_FALSE(shown(sized, "wide")) << "200 is compact where regular starts at 250";

	ASSERT_NE(original, nullptr);
	original->setThemeActive();
	EXPECT_TRUE(shown(sized, "wide"));
}

TEST_F(TestCustomise, StripWidgetsAreRegular)
{
	// A strip sizes its widgets by their content, so it doesn't measure them: tiny as it is, this one is regular
	Layout strip{.slot = "test", .kind = UI::Xml::LayoutKind::Strip};
	strip.items = {{.id = "x", .widget = "sized"}};
	save(strip);
	lv_obj_update_layout(layout());
	EXPECT_TRUE(shown(item("x"), "wide"));
	EXPECT_TRUE(shown(item("x"), "tall"));
	EXPECT_FALSE(shown(item("x"), "upright"));
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

	/** The label in `root` showing `text`, or nullptr */
	static lv_obj_t* labelWithText(lv_obj_t* root, std::string_view text)
	{
		for (uint32_t i = 0; i < lv_obj_get_child_count(root); i++)
		{
			lv_obj_t* child = lv_obj_get_child(root, static_cast<int32_t>(i));
			if (lv_obj_check_type(child, &lv_label_class) && std::string_view(lv_label_get_text(child)) == text)
			{
				return child;
			}
			if (lv_obj_t* found = labelWithText(child, text))
			{
				return found;
			}
		}
		return nullptr;
	}

	/** The widget pool's item whose title is `title`, or nullptr */
	static lv_obj_t* itemTitled(lv_obj_t* pool, std::string_view title)
	{
		lv_obj_t* label = labelWithText(pool, title);
		return label ? lv_obj_get_parent(label) : nullptr;
	}

	/** The edit overlay over the placed widget `id` in `slot`: in its box in a free layout, else in the widget */
	static lv_obj_t* overlayOf(const char* slot, const char* id)
	{
		lv_obj_t* widget = placed(slot, id);
		if (widget == nullptr)
		{
			return nullptr;
		}
		const Layout* layout = Layouts::find(slot);
		lv_obj_t* holder = layout && layout->kind == UI::Xml::LayoutKind::Free ? lv_obj_get_parent(widget) : widget;
		return lv_obj_find_by_name(holder, "customise_overlay");
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
		UI::Xml::Actions::dispatch(m_home->getRootPtr(), "actions.customise.start", "");
		update();
	}

	fs::path m_userDir;
	std::unique_ptr<UI::Xml::XmlView> m_home;
};

TEST_F(TestCustomiseHome, CustomisingCoversEveryWidgetAndShowsTheToolbar)
{
	EXPECT_FALSE(shownOnScreen(toolbar()));
	EXPECT_EQ(overlayOf("dashboard", "tools"), nullptr);

	start();
	EXPECT_TRUE(UI::Xml::LayoutEditor::get().isEditing());
	EXPECT_TRUE(shownOnScreen(toolbar()));
	for (const auto& [slot, id] : std::vector<std::pair<const char*, const char*>>{
			 {"dashboard", "tools"}, {"dashboard", "job"}, {"sidebar", "home"}, {"sidebar", "estop"}})
	{
		EXPECT_NE(overlayOf(slot, id), nullptr) << slot << "/" << id;
	}
	tap(middleOf(placed("dashboard", "temperature_graph")));
	EXPECT_EQ(UI::Xml::LayoutEditor::get().selection().item, "temperature_graph");
	EXPECT_EQUAL_SCREENSHOT("customise/editing.png")

	tapToolbar("cancel");
	EXPECT_FALSE(UI::Xml::LayoutEditor::get().isEditing());
	EXPECT_FALSE(shownOnScreen(toolbar()));
	EXPECT_EQ(overlayOf("dashboard", "tools"), nullptr);
}

TEST_F(TestCustomiseHome, WidgetsAreRemovedMovedAndResizedByTouch)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");
	EXPECT_EQ(draftItem("dashboard", "temperature_graph"), nullptr);
	EXPECT_EQ(placed("dashboard", "temperature_graph"), nullptr);

	// Down 150 pixels, as the dashboard is shown at the size its layout was made for; nothing is near to snap to
	const lv_point_t middle = middleOf(placed("dashboard", "tools"));
	drag(middle, {middle.x, middle.y + 150});
	ASSERT_NE(draftItem("dashboard", "tools"), nullptr);
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 150);

	// The selected widget has a handle on each corner and edge; the bottom one makes it 100 pixels taller
	lv_obj_t* overlay = overlayOf("dashboard", "tools");
	for (const char* handle :
		 {"handle_nw", "handle_n", "handle_ne", "handle_e", "handle_se", "handle_s", "handle_sw", "handle_w"})
	{
		ASSERT_NE(lv_obj_find_by_name(overlay, handle), nullptr) << handle;
		EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(overlay, handle), LV_OBJ_FLAG_HIDDEN)) << handle;
	}
	const lv_point_t bottom = middleOf(lv_obj_find_by_name(overlay, "handle_s"));
	drag(bottom, {bottom.x, bottom.y + 100});
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 150);
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 377);

	tapToolbar("undo");
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 277);

	// The top one moves the top edge, so it starts higher and is taller
	const lv_point_t top = middleOf(lv_obj_find_by_name(overlayOf("dashboard", "tools"), "handle_n"));
	drag(top, {top.x, top.y - 100});
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 50);
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 377);
	EXPECT_EQUAL_SCREENSHOT("customise/handles.png")

	// Other widgets show no handles
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(overlayOf("dashboard", "job"), "handle_n"), LV_OBJ_FLAG_HIDDEN));
}

TEST_F(TestCustomiseHome, WidgetsAreAddedFromThePoolWhereTheUserTapped)
{
	start();
	tap(middleOf(placed("dashboard", "temperature_graph")));
	tapToolbar("remove");

	// Near the bottom left corner, where the graph was
	lv_obj_t* dashboard = UI::Xml::DsLayout::find("dashboard")->getContainer();
	lv_area_t area;
	lv_obj_get_content_coords(dashboard, &area);
	tap({area.x1 + 10, area.y1 + 400});
	EXPECT_EQ(UI::Xml::LayoutEditor::get().selection().point, (UI::Xml::Point{10, 400}));
	ASSERT_NE(lv_obj_find_by_name(dashboard, "customise_place"), nullptr);

	tap(middleOf(lv_obj_find_by_name(dashboard, "customise_place")));
	lv_obj_t* pool = lv_obj_find_by_name(lv_screen_active(), "catalogue_dialog");
	ASSERT_TRUE(shownOnScreen(pool));
	EXPECT_EQUAL_SCREENSHOT("customise/widget_pool.png")

	// The pool lists the dashboard's widgets by category, then name
	lv_obj_t* axes = itemTitled(pool, "Axis positions");
	EXPECT_NE(labelWithText(pool, "Motion"), nullptr);
	ASSERT_NE(axes, nullptr);
	lv_obj_scroll_to_view_recursive(axes, LV_ANIM_OFF); // As the user scrolls to it
	update();
	tap(middleOf(axes));
	EXPECT_FALSE(shownOnScreen(pool));
	const auto* added = draftItem("dashboard", "axes");
	ASSERT_NE(added, nullptr);
	EXPECT_EQ(added->rect(), (UI::Xml::Rect{.x = 10, .y = 400, .w = 365, .h = 89})); // At its default size
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

	// The time shown: its values, the settings' next step, then 5 minutes, which goes back to the settings
	tap(middleOf(lv_obj_find_by_name(dialog, "value")));
	lv_obj_t* options = TestDialogs::shownNamed("dialog_choice");
	ASSERT_NE(options, nullptr);
	EXPECT_EQ(TestDialogs::title(), _("widget.temperature_graph_seconds"));
	lv_obj_t* list = lv_obj_find_by_name(options, "list");
	ASSERT_EQ(lv_obj_get_child_count(list), 5u);
	EXPECT_EQUAL_SCREENSHOT("customise/setting_choices.png")
	tap(middleOf(lv_obj_get_child(list, 3)));
	EXPECT_EQ(TestDialogs::shown(), dialog);
	EXPECT_EQ(draftItem("dashboard", "temperature_graph")->settings["seconds"], "300");
	EXPECT_STREQ(lv_label_get_text(lv_obj_get_child(lv_obj_find_by_name(dialog, "value"), -1)), "5 min");
}

TEST_F(TestCustomiseHome, SelectingAnotherWidgetClosesTheSettingsAndTheirKeyboard)
{
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	editor.select("dashboard", "temperature_graph");
	editor.removeSelected();
	editor.selectPlace("dashboard", UI::Xml::Point{0, 470});
	ASSERT_EQ(editor.add("gcode_button"), "gcode_button");
	update();
	tapToolbar("configure");
	ASSERT_NE(TestDialogs::shownNamed("settings_dialog"), nullptr);

	// The label typed in: the keyboard is up
	lv_obj_t* label = lv_obj_find_by_name(lv_obj_get_child(TestDialogs::find("list"), 0), "text");
	ASSERT_NE(label, nullptr);
	lv_obj_send_event(label, LV_EVENT_FOCUSED, nullptr);
	lv_obj_t* keyboard = lv_obj_find_by_name(lv_screen_active(), "keyboard");
	ASSERT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));

	// A sidebar button tapped beside the dialog is selected: the settings, which were the G-code button's, close,
	// and the keyboard goes with the box it typed into
	tap(middleOf(placed("sidebar", "files")));
	EXPECT_EQ(editor.selection().item, "files");
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	EXPECT_TRUE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
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

TEST_F(TestCustomiseHome, EveryWidgetCanBeAddedToEverySlotItFits)
{
	// With printer data, so the widgets show something; the dashboard emptied, to make room
	load_model_data_from_file("tests/object_model/test_bench/model_sensors_v.json");
	load_model_data_from_file("tests/object_model/test_bench/model_heat_v.json");
	load_model_data_from_file("tests/object_model/test_bench/model_tools_v.json");
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	for (const char* id : {"tools", "temperature_graph", "job"})
	{
		editor.select("dashboard", id);
		EXPECT_TRUE(editor.removeSelected());
	}

	// Each is added, lays out and settles (a widget sized 100% once kept the layout from ever settling, see
	// DsLayout.h), and is removed again, so the next has the room
	for (const auto& [id, widget] : UI::Xml::WidgetCatalogue::all())
	{
		for (const auto& slot : widget.slots)
		{
			editor.selectPlace(slot);
			const auto added = editor.add(id);
			update();
			ASSERT_TRUE(added.has_value()) << id << " in " << slot;
			EXPECT_NE(placed(slot.c_str(), added->c_str()), nullptr) << id << " in " << slot;
			editor.select(slot, *added);
			EXPECT_TRUE(editor.removeSelected()) << id << " in " << slot;
			update();
		}
	}
}

TEST_F(TestCustomiseHome, TheToolbarIsDraggedByItsGrip)
{
	start();
	lv_obj_t* bar = lv_obj_find_by_name(toolbar(), "bar");
	lv_area_t before;
	lv_obj_get_coords(bar, &before);

	// Up and left, out of the way
	const lv_point_t grip = middleOf(lv_obj_find_by_name(bar, "grip"));
	drag(grip, {grip.x - 100, grip.y - 300});
	lv_area_t after;
	lv_obj_get_coords(bar, &after);
	EXPECT_EQ(after.x1, before.x1 - 100);
	EXPECT_EQ(after.y1, before.y1 - 300);

	// It stays on the screen however far it is dragged
	const lv_point_t moved = middleOf(lv_obj_find_by_name(bar, "grip"));
	drag(moved, {moved.x - 2000, moved.y - 2000});
	lv_obj_get_coords(bar, &after);
	lv_area_t screenArea;
	lv_obj_get_content_coords(toolbar(), &screenArea);
	EXPECT_EQ(after.x1, screenArea.x1);
	EXPECT_EQ(after.y1, screenArea.y1);
	EXPECT_EQUAL_SCREENSHOT("customise/toolbar_moved.png")
}

TEST_F(TestCustomiseHome, TextSettingsAreTypedInTheSettingsAboveTheKeyboard)
{
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	editor.select("dashboard", "temperature_graph");
	editor.removeSelected();
	editor.selectPlace("dashboard", UI::Xml::Point{0, 470});
	ASSERT_EQ(editor.add("gcode_button"), "gcode_button");
	update();
	tapToolbar("configure");
	lv_obj_t* dialog = lv_obj_find_by_name(lv_screen_active(), "settings_dialog");
	ASSERT_TRUE(shownOnScreen(dialog));

	// The label is typed in its row: the dialog moves above the keyboard while it is
	lv_obj_t* label = lv_obj_find_by_name(lv_obj_get_child(lv_obj_find_by_name(dialog, "list"), 0), "text");
	ASSERT_NE(label, nullptr);
	ASSERT_TRUE(shownOnScreen(label));
	lv_area_t before;
	lv_obj_get_coords(dialog, &before);
	lv_obj_send_event(label, LV_EVENT_FOCUSED, nullptr);
	update();
	lv_area_t above;
	lv_obj_get_coords(dialog, &above);
	EXPECT_LT(above.y1, before.y1);
	lv_textarea_set_text(label, "Level");
	EXPECT_EQUAL_SCREENSHOT("customise/text_setting.png")
	lv_obj_send_event(label, LV_EVENT_READY, nullptr);
	update();
	EXPECT_EQ(draftItem("dashboard", "gcode_button")->settings["label"], "Level");
	lv_area_t after;
	lv_obj_get_coords(dialog, &after);
	EXPECT_EQ(after.y1, before.y1);
}

TEST_F(TestCustomiseHome, EdgesAreGrabbedAnywhereNearThemEvenOnASmallWidget)
{
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	editor.select("dashboard", "temperature_graph");
	editor.removeSelected();
	editor.selectPlace("dashboard", UI::Xml::Point{185, 376});
	ASSERT_EQ(editor.add("gcode_button"), "gcode_button"); // 180x89, the smallest default
	update();
	const auto area = [&]
	{
		lv_area_t coords;
		lv_obj_get_coords(overlayOf("dashboard", "gcode_button"), &coords);
		return coords;
	};
	const auto rect = [] { return draftItem("dashboard", "gcode_button")->rect(); };
	ASSERT_EQ(rect(), (UI::Xml::Rect{.x = 185, .y = 376, .w = 180, .h = 89}));

	// Near the top right corner, but not on its bracket: both edges move
	lv_area_t a = area();
	drag({a.x2 - 20, a.y1 + 20}, {a.x2 - 20 + 50, a.y1 + 20 - 50});
	EXPECT_EQ(rect(), (UI::Xml::Rect{.x = 185, .y = 326, .w = 230, .h = 139}));

	// Near the bottom edge, well away from its bar: only the bottom moves
	a = area();
	drag({a.x1 + 30, a.y2 - 10}, {a.x1 + 30, a.y2 - 10 + 50});
	EXPECT_EQ(rect(), (UI::Xml::Rect{.x = 185, .y = 326, .w = 230, .h = 189}));

	// The middle moves the whole widget
	a = area();
	const lv_point_t middle = {(a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2};
	drag(middle, {middle.x + 50, middle.y});
	EXPECT_EQ(rect(), (UI::Xml::Rect{.x = 235, .y = 326, .w = 230, .h = 189}));

	// A first touch near an edge of a widget not yet selected selects it; a drag moves it
	const lv_point_t tools = middleOf(placed("dashboard", "tools"));
	lv_area_t toolsArea;
	lv_obj_get_coords(overlayOf("dashboard", "tools"), &toolsArea);
	drag({tools.x, toolsArea.y2 - 10}, {tools.x, toolsArea.y2 - 10 + 30});
	EXPECT_EQ(editor.selection().item, "tools");
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 277);
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 30);
}

TEST_F(TestCustomiseHome, DraggedWidgetsSnapToTheEdgesNearThem)
{
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	editor.select("dashboard", "temperature_graph");
	editor.removeSelected();
	editor.select("dashboard", "tools");
	update();

	// Its bottom dragged to a pixel short of the area's: it snaps there
	lv_obj_t* overlay = overlayOf("dashboard", "tools");
	const lv_point_t bottom = middleOf(lv_obj_find_by_name(overlay, "handle_s"));
	drag(bottom, {bottom.x, bottom.y + 280});
	EXPECT_EQ(draftItem("dashboard", "tools")->h, 561);

	// Moved near the files, it keeps the gap from them
	editor.select("dashboard", "tools");
	tapToolbar("undo");
	update();
	const lv_point_t middle = middleOf(placed("dashboard", "tools"));
	drag(middle, {middle.x - 8, middle.y + 30});
	EXPECT_EQ(draftItem("dashboard", "tools")->x, 0); // Back to the area's edge
	EXPECT_EQ(draftItem("dashboard", "tools")->y, 30);

	// With snapping off, it goes where it is dragged
	tapToolbar("snap");
	EXPECT_FALSE(editor.draft("dashboard")->snap);
	const lv_point_t again = middleOf(placed("dashboard", "tools"));
	drag(again, {again.x + 3, again.y});
	EXPECT_EQ(draftItem("dashboard", "tools")->x, 3);
}

TEST_F(TestCustomiseHome, LayoutsAreSavedUnderANameLoadedAndDeleted)
{
	auto& editor = UI::Xml::LayoutEditor::get();
	start();
	lv_obj_t* screen = lv_screen_active();
	lv_obj_t* loadDialog = lv_obj_find_by_name(screen, "load_dialog");
	tapToolbar("load");
	ASSERT_TRUE(shownOnScreen(loadDialog));
	EXPECT_TRUE(shownOnScreen(lv_obj_find_by_name(loadDialog, "none"))); // Nothing saved yet
	UI::Xml::Actions::dispatch(lv_obj_find_by_name(loadDialog, "none"), "actions.dialog.close", "");
	update();

	editor.select("dashboard", "temperature_graph");
	editor.removeSelected();
	update();

	// Saving asks for a name, and says what is wrong with one it can't use
	tapToolbar("save");
	lv_obj_t* saveDialog = lv_obj_find_by_name(screen, "save_dialog");
	ASSERT_TRUE(shownOnScreen(saveDialog));
	lv_obj_t* name = lv_obj_find_by_name(saveDialog, "name");
	EXPECT_STREQ(lv_textarea_get_text(name), "");
	lv_textarea_set_text(name, "no/graph");
	tap(middleOf(TestDialogs::find("ok")));
	EXPECT_TRUE(shownOnScreen(saveDialog));
	EXPECT_TRUE(shownOnScreen(lv_obj_find_by_name(saveDialog, "problem")));
	EXPECT_FALSE(fs::exists(m_userDir / "saved"));

	lv_textarea_set_text(name, "No graph");
	EXPECT_EQUAL_SCREENSHOT("customise/save.png")
	lv_obj_send_event(name, LV_EVENT_READY, nullptr);
	update();
	EXPECT_FALSE(shownOnScreen(saveDialog));
	EXPECT_TRUE(fs::exists(m_userDir / "saved" / "No graph.json"));

	// Loading puts back what was saved, as one change
	editor.undo();
	update();
	ASSERT_NE(draftItem("dashboard", "temperature_graph"), nullptr);
	tapToolbar("load");
	ASSERT_TRUE(shownOnScreen(loadDialog));
	EXPECT_FALSE(shownOnScreen(lv_obj_find_by_name(loadDialog, "none")));
	lv_obj_t* row = lv_obj_get_child(lv_obj_find_by_name(loadDialog, "list"), 0);
	ASSERT_NE(row, nullptr);
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(row, "name")), "No graph");
	EXPECT_EQUAL_SCREENSHOT("customise/load.png")
	tap(middleOf(lv_obj_find_by_name(row, "load")));
	EXPECT_FALSE(shownOnScreen(loadDialog));
	EXPECT_EQ(draftItem("dashboard", "temperature_graph"), nullptr);
	tapToolbar("undo");
	EXPECT_NE(draftItem("dashboard", "temperature_graph"), nullptr);

	// Deleting asks first
	tapToolbar("load");
	row = lv_obj_get_child(lv_obj_find_by_name(loadDialog, "list"), 0);
	tap(middleOf(lv_obj_find_by_name(row, "delete")));
	lv_obj_t* question = TestDialogs::shownNamed("dialog_question");
	ASSERT_NE(question, nullptr);
	tap(middleOf(TestDialogs::find("yes")));
	EXPECT_EQ(TestDialogs::shown(), loadDialog) << "back to the saved layouts";
	EXPECT_FALSE(fs::exists(m_userDir / "saved" / "No graph.json"));
	EXPECT_TRUE(shownOnScreen(lv_obj_find_by_name(loadDialog, "none")));
}

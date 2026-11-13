/*
 * test_customise.cpp
 *
 * The customisable screen: <ds_layout> showing a slot's layout and following its changes. The widget pool and layout
 * files themselves are tested in test_layouts.cpp.
 */

#include "UI/Widgets/Layout/DsLayout.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
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

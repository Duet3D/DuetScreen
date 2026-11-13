/*
 * test_xml_ui_api.cpp
 *
 * The UI API reference, docs/XML_UI_API.md, its schema, docs/XML_UI.xsd, and the built-in themes,
 * docs/XML_UI_THEMES.md, are generated from the code (src/UI/Xml/Api). These tests check that the committed copies
 * match the code; when one doesn't, the file is rewritten and the test fails, so the change shows in `git diff` to be
 * reviewed and committed. Their pictures are reference images (tests/ref_imgs/xml_ui_api/), compared like every other
 * screenshot: a changed one is written beside it as `_err`, for run_tests.py to review. The tests also lint the
 * built-in bundle.
 *
 * Presenters and global actions are registered for the life of the program, so the reference must be built in a
 * process that has registered only the firmware's own: ctest runs each test in a process of its own. Running the test
 * binary directly, filter to this suite (`--gtest_filter='XmlUiApi.*'`).
 */

#include "UI/Core/Model.h"
#include "UI/Xml/Api/ApiImages.h"
#include "UI/Xml/Api/ApiLint.h"
#include "UI/Xml/Api/ApiMarkdown.h"
#include "UI/Xml/Api/ApiReference.h"
#include "UI/Xml/Api/ApiXsd.h"
#include "test_utils/UiTestSuite.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <gtest/gtest.h>
#include <set>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace
{
	// Relative to the repository, where tests run
	constexpr const char* REFERENCE_PATH = "docs/XML_UI_API.md"; // The reference
	constexpr const char* SCHEMA_PATH = "docs/XML_UI.xsd";		 // Its schema
	constexpr const char* THEMES_PATH = "docs/XML_UI_THEMES.md"; // The built-in themes
	constexpr const char* IMAGE_ROOT = "../tests/ref_imgs/";	 // The reference images, from docs/

	/** Object model replies describing the printer the views are pictured with */
	constexpr const char* SAMPLE_PRINTER[] = {
		"tests/object_model/test_bench/model_boards_v.json",
		"tests/object_model/test_bench/model_directories_v.json",
		"tests/object_model/test_bench/model_fans_v.json",
		"tests/object_model/test_bench/model_heat_v.json",
		"tests/object_model/job/model_job_printing_layer_2.json",
		"tests/object_model/test_bench/model_move_vn.json",
		"tests/object_model/test_bench/model_network_v.json",
		"tests/object_model/test_bench/model_sensors_v.json",
		"tests/object_model/test_bench/model_spindles_v.json",
		"tests/object_model/job/model_state_printing.json",
		"tests/object_model/test_bench/model_tools_v.json",
		"tests/object_model/test_bench/model_volumes_v.json",
		"tests/object_model/test_bench/model_all_d99f.json",
		"tests/object_model/rr_filelist/example1.json",
	};

	/** The contents of `path`, or empty if it can't be read */
	std::string readFile(const char* path)
	{
		std::ifstream file(path, std::ios::binary);
		std::stringstream text;
		text << file.rdbuf();
		return text.str();
	}

	/** The field called `name` in `fields`, or nullptr */
	const UI::Xml::Api::Field* findField(const std::vector<UI::Xml::Api::Field>& fields, std::string_view name)
	{
		auto it = std::find_if(fields.begin(), fields.end(), [&](const auto& f) { return f.name == name; });
		return it == fields.end() ? nullptr : &*it;
	}
} // namespace

class XmlUiApi : public UiTestSuite
{
  protected:
	/** Fail, and rewrite the file at `path`, unless it holds `expected` */
	static void expectUpToDate(const char* path, const std::string& expected)
	{
		if (readFile(path) == expected)
		{
			return;
		}
		std::ofstream(path) << expected;
		FAIL() << path << " was out of date and has been rewritten: review the change and commit it";
	}
};

/* Every kind of name is gathered, with its description and type */
TEST_F(XmlUiApi, DescribesEveryKindOfName)
{
	const auto reference = UI::Xml::Api::build();

	// Global data, with list rows, nested lists and the `index` every row has
	auto tools = std::find_if(
		reference.sources.begin(), reference.sources.end(), [](const auto& s) { return findField(s.fields, "tools"); });
	ASSERT_NE(tools, reference.sources.end());
	const auto* toolList = findField(tools->fields, "tools");
	EXPECT_EQ(toolList->type, "list");
	EXPECT_NE(findField(toolList->rowFields, "index"), nullptr);
	const auto* heaters = findField(toolList->rowFields, "heaters");
	ASSERT_NE(heaters, nullptr);
	EXPECT_FALSE(heaters->rowFields.empty());

	// Every action says what it does
	EXPECT_FALSE(reference.globalActions.empty());
	for (const auto& action : reference.globalActions)
	{
		EXPECT_FALSE(action.description.empty()) << action.name;
	}
	for (const auto& presenter : reference.presenters)
	{
		for (const auto& action : presenter.actions)
		{
			EXPECT_FALSE(action.description.empty()) << presenter.name << ": " << action.name;
		}
	}

	// Presenters know the components that name them
	auto fans = std::find_if(
		reference.presenters.begin(), reference.presenters.end(), [](const auto& p) { return p.name == "fans"; });
	ASSERT_NE(fans, reference.presenters.end());
	EXPECT_FALSE(fans->components.empty());

	// Every widget says what it is, and what each attribute does
	EXPECT_FALSE(reference.widgets.empty());
	for (const auto& widget : reference.widgets)
	{
		EXPECT_FALSE(widget.description.empty()) << widget.name;
		for (const auto& attribute : widget.attributes)
		{
			EXPECT_FALSE(attribute.description.empty()) << widget.name << ": " << attribute.name;
		}
	}

	// The theme
	EXPECT_EQ(reference.rootTheme, "base");
	auto card =
		std::find_if(reference.styles.begin(), reference.styles.end(), [](const auto& s) { return s.name == "card"; });
	ASSERT_NE(card, reference.styles.end());
	EXPECT_FALSE(card->definition.empty());
	EXPECT_FALSE(reference.consts.empty());
	EXPECT_NE(std::find(reference.colours.begin(), reference.colours.end(), "primary"), reference.colours.end());
	EXPECT_NE(std::find(reference.fonts.begin(), reference.fonts.end(), "font_header"), reference.fonts.end());
}

/* docs/XML_UI_API.md and docs/XML_UI_THEMES.md match the code, and their pictures the reference images */
TEST_F(XmlUiApi, ReferenceIsUpToDate)
{
	// The printer the views are pictured with: the one the home view screenshots use
	for (const char* file : SAMPLE_PRINTER)
	{
		load_model_data_from_file(file);
	}
	const auto reference = UI::Xml::Api::build();

	// Deliver the events a view causes, then those of the threads it starts (the WiFi scanner posts its networks
	// when its scan, instant in simulation, finishes), so every picture is drawn with everything there
	const auto settle = []
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	};
	// The pictures show the release firmware, as the Test preset builds it. A developer build shows more (developer
	// settings, the console's shell), so its pictures differ and aren't compared; its Markdown is the same.
#if DEVELOPER_MODE || DEBUG_BORDERS || ENABLE_CONSOLE_SHELL
	constexpr bool comparePictures = false;
#else
	constexpr bool comparePictures = true;
#endif
	const auto images = UI::Xml::Api::render(reference,
											 settle,
											 [&](const std::string& file)
											 {
												 if (comparePictures)
												 {
													 EXPECT_EQUAL_PICTURE(file.c_str());
												 }
											 });
	EXPECT_FALSE(images.themes.empty());
	EXPECT_FALSE(images.views.empty());

	// Pictures of things that are gone (a changed picture's `_err` is run_tests.py's to clean up)
	const std::set<std::string> drawn(images.files.begin(), images.files.end());
	const std::filesystem::path root = REF_IMGS_PATH;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(root / UI::Xml::Api::IMAGES_DIR))
	{
		const std::string file = std::filesystem::relative(entry.path(), root).generic_string();
		if (comparePictures && entry.is_regular_file() && !file.ends_with("_err.png") && !drawn.contains(file))
		{
			std::filesystem::remove(entry.path());
			ADD_FAILURE() << entry.path().string() << " pictures nothing any more and has been removed: commit that";
		}
	}

	expectUpToDate(REFERENCE_PATH, UI::Xml::Api::toMarkdown(reference, images, IMAGE_ROOT) + "\n");
	expectUpToDate(THEMES_PATH, UI::Xml::Api::toThemesMarkdown(reference, images, IMAGE_ROOT) + "\n");
}

/* docs/XML_UI.xsd matches the code; if not, it is rewritten for review */
TEST_F(XmlUiApi, SchemaIsUpToDate)
{
	expectUpToDate(SCHEMA_PATH, UI::Xml::Api::toXsd(UI::Xml::Api::build()));
}

/* Every name the built-in bundle uses is one the UI provides where it is used */
TEST_F(XmlUiApi, BuiltInBundleUsesOnlyNamesTheUiProvides)
{
	const auto issues = UI::Xml::Api::lint(UI::Xml::Api::build());
	for (const auto& issue : issues)
	{
		ADD_FAILURE() << UI::Xml::Api::toString(issue);
	}
}

/* A user bundle's mistakes are found, each where it is made */
TEST_F(XmlUiApi, LintFindsNamesTheUiDoesNotProvide)
{
	namespace fs = std::filesystem;
	const fs::path userDir = fs::temp_directory_path() / fmt::format("duetscreen-xml-ui-api-{:d}", getpid());
	fs::remove_all(userDir);
	fs::create_directories(userDir / "test" / "components");
	std::ofstream(userDir / "test" / "manifest.json")
		<< R"({"id": "test", "ui_api": "1.0", "components": ["components/*.xml"]})";

	// A screen, nothing uses it, so it is a view of its own, with the fans presenter
	std::ofstream(userDir / "test" / "components" / "lint_screen.xml") << R"(<component presenter="fans">
	<view extends="lv_obj">
		<lv_label bind_text="no_such_subject"/>
		<style name="no_such_style"/>
		<ds_button translation_tag="no.such.key">
			<action name="no.such_action"/>
			<action name="fans.set_speed" trigger="clickd"/>
		</ds_button>
		<ds_list list="fans" item="lint_fan"/>
		<lint_dialog open="no_such_open"/>
		<no_such_widget/>
		<lv_obj style_radius="#no_such_const" style_pad_all="#pad"/>
		<bind_flag_if cond="machine.printing and no_such_operand" flag="hidden"/>
	</view>
</component>)";

	// A list item: its row's fields come first
	std::ofstream(userDir / "test" / "components" / "lint_fan.xml") << R"(<component>
	<view extends="lv_obj">
		<lv_label bind_text="rpm"/>
		<lv_label bind_text="rpmx"/>
		<ds_button><action name="gcode" arg="M106 P{number} S{value} X{nothing}"/></ds_button>
	</view>
</component>)";

	// A component with a prop: the value given where it is used is checked there
	std::ofstream(userDir / "test" / "components" / "lint_dialog.xml") << R"(<component>
	<api><prop name="open" type="string"/></api>
	<view extends="ds_modal" bind_open="$open">
		<lv_label bind_text="$missing_prop"/>
	</view>
</component>)";

	UI::Xml::deinit();
	UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = userDir});
	const auto issues = UI::Xml::Api::lint(UI::Xml::Api::build());
	fs::remove_all(userDir);

	std::vector<std::string> found;
	for (const auto& issue : issues)
	{
		if (issue.file.find("lint_") != std::string::npos)
		{
			found.push_back(fmt::format(
				"{:s}:{:d}: {:s}", std::filesystem::path(issue.file).filename().string(), issue.line, issue.message));
		}
	}
	const std::vector<std::string> expected = {
		"lint_dialog.xml:3: no subject `no_such_open` (in lint_screen › lint_dialog)",
		"lint_dialog.xml:4: no prop `missing_prop` (in lint_screen › lint_dialog)",
		"lint_fan.xml:4: no subject `rpmx` (in lint_screen › lint_fan)",
		"lint_fan.xml:5: no subject `nothing` (in lint_screen › lint_fan)",
		"lint_screen.xml:3: no subject `no_such_subject` (in lint_screen)",
		"lint_screen.xml:4: no style `no_such_style` (in lint_screen)",
		"lint_screen.xml:5: no translation `no.such.key` (in lint_screen)",
		"lint_screen.xml:6: no action `no.such_action` (in lint_screen)",
		"lint_screen.xml:7: no trigger `clickd` (in lint_screen)",
		"lint_screen.xml:11: <no_such_widget> is no widget, component or binding (in lint_screen)",
		"lint_screen.xml:12: no const or colour `#no_such_const` (in lint_screen)",
		"lint_screen.xml:13: no subject `no_such_operand` (in lint_screen)",
	};
	EXPECT_EQ(found, expected) << fmt::format("{}", fmt::join(found, "\n"));
}

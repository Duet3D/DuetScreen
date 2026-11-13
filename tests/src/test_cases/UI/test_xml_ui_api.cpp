/*
 * test_xml_ui_api.cpp
 *
 * The UI API reference, docs/XML_UI_API.md, is generated from the code (src/UI/Xml/Api). These tests check that the
 * committed copy matches the code; when it doesn't, the file is rewritten and the test fails, so the change shows in
 * `git diff` to be reviewed and committed.
 *
 * Presenters and global actions are registered for the life of the program, so the reference must be built in a
 * process that has registered only the firmware's own: ctest runs each test in a process of its own. Running the test
 * binary directly, filter to this suite (`--gtest_filter='XmlUiApi.*'`).
 *
 * The reference describes the release firmware, as built by the Test preset. Developer builds (DEVELOPER_MODE,
 * DEBUG_BORDERS, ENABLE_CONSOLE_SHELL) register extra settings, so they skip the comparison rather than rewrite it.
 */

#include "UI/Xml/Api/ApiMarkdown.h"
#include "UI/Xml/Api/ApiReference.h"
#include "test_utils/UiTestSuite.h"
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

namespace
{
	constexpr const char* REFERENCE_PATH = "docs/XML_UI_API.md"; // Relative to the repository, where tests run

	/** The contents of `path`, or empty if it can't be read */
	std::string readFile(const char* path)
	{
		std::ifstream file(path);
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

/* docs/XML_UI_API.md matches the code; if not, it is rewritten for review */
TEST_F(XmlUiApi, ReferenceIsUpToDate)
{
#if DEVELOPER_MODE || DEBUG_BORDERS || ENABLE_CONSOLE_SHELL
	GTEST_SKIP() << REFERENCE_PATH << " describes the release firmware; this is a developer build";
#endif
	const std::string expected = UI::Xml::Api::toMarkdown(UI::Xml::Api::build()) + "\n";
	if (readFile(REFERENCE_PATH) == expected)
	{
		return;
	}
	std::ofstream(REFERENCE_PATH) << expected;
	FAIL() << REFERENCE_PATH << " was out of date and has been rewritten: review the change and commit it";
}

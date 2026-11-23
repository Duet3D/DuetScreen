/*
 * test_pages.cpp
 *
 * <ds_pages> (src/UI/Widgets/Layout/DsPages.h): pages sharing one place, opened with actions.navigation.open_page and
 * left with the header's back button or the back key.
 */

#include "UI/Core/Navigation.h"
#include "UI/Widgets/Layout/DsPages.h"
#include "UI/Xml/View/XmlView.h"
#include "helix_xml.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

class TestPages : public UiTestSuite
{
  protected:
	TestPages()
	{
		lv_xml_register_component_from_data("test_pages", R"(
			<component>
				<view extends="lv_obj" width="400" height="240" style_pad_all="0" scrollable="false">
					<ds_pages name="pages" width="100%" height="100%">
						<ds_pages-page name="first" title="First">
							<ds_button name="to_second" text="Second">
								<action trigger="clicked" name="actions.navigation.open_page" arg="second"/>
							</ds_button>
						</ds_pages-page>
						<ds_pages-page name="second" title="Second" flex_flow="row">
							<ds_button name="to_third" text="Third">
								<action trigger="clicked" name="actions.navigation.open_page" arg="third"/>
							</ds_button>
							<ds_button name="to_nowhere" text="Nowhere">
								<action trigger="clicked" name="actions.navigation.open_page" arg="nowhere"/>
							</ds_button>
						</ds_pages-page>
						<ds_pages-page name="third" title="Third">
							<lv_label text="The third page"/>
						</ds_pages-page>
					</ds_pages>
				</view>
			</component>)");
		view = std::make_unique<UI::Xml::XmlView>("pages_view", screen, "test_pages");
		view->show();
		lv_obj_update_layout(screen.getRootPtr());
	}

	/** The widget called `name` in the view */
	lv_obj_t* part(const char* name) const { return lv_obj_find_by_name(view->getRootPtr(), name); }

	/** The stack */
	UI::Xml::DsPages& pages() const { return *static_cast<UI::Xml::DsPages*>(UI::LvObj::fromPtr(part("pages"))); }

	/** Whether the widget called `name` is hidden */
	bool hidden(const char* name) const { return lv_obj_has_flag(part(name), LV_OBJ_FLAG_HIDDEN); }

	/** Tap the widget called `name`, then let any slide finish */
	void tap(const char* name)
	{
		lv_obj_send_event(part(name), LV_EVENT_CLICKED, nullptr);
		runTimers();
	}

	/** The text of the header's label called `name` (the back button's, or the title) */
	std::string headerText(const char* name) const
	{
		lv_obj_t* header = part("header");
		lv_obj_t* obj = lv_obj_find_by_name(header, name);
		if (!lv_obj_check_type(obj, &lv_label_class))
		{
			obj = lv_obj_find_by_name(obj, "label");
		}
		return lv_label_get_text(obj);
	}

	std::unique_ptr<UI::Xml::XmlView> view; // The view holding the stack
};

TEST_F(TestPages, TheFirstPageIsShownWithoutAHeader)
{
	EXPECT_EQ(pages().shownPage(), "first");
	EXPECT_FALSE(hidden("first"));
	EXPECT_TRUE(hidden("second"));
	EXPECT_TRUE(hidden("third"));
	EXPECT_TRUE(hidden("header"));
}

TEST_F(TestPages, AnActionOpensAPageUnderAHeaderWithTheWayBack)
{
	tap("to_second");
	EXPECT_EQ(pages().shownPage(), "second");
	EXPECT_TRUE(hidden("first"));
	EXPECT_FALSE(hidden("second"));
	EXPECT_FALSE(hidden("header"));
	EXPECT_EQ(headerText("title"), "Second");
	EXPECT_EQ(headerText("back"), "First");
	EXPECT_EQUAL_SCREENSHOT("pages/second.png")

	tap("to_third");
	EXPECT_EQ(pages().shownPage(), "third");
	EXPECT_EQ(headerText("back"), "Second") << "back goes to the page it came from";

	tap("back");
	EXPECT_EQ(pages().shownPage(), "second");
	tap("back");
	EXPECT_EQ(pages().shownPage(), "first");
	EXPECT_FALSE(hidden("first"));
	EXPECT_TRUE(hidden("second"));
	EXPECT_TRUE(hidden("header"));
}

TEST_F(TestPages, TheBackKeyGoesBackThroughThePagesShown)
{
	tap("to_second");
	tap("to_third");
	UI::back();
	runTimers();
	EXPECT_EQ(pages().shownPage(), "second");
	UI::back();
	runTimers();
	EXPECT_EQ(pages().shownPage(), "first");
	UI::back(); // Nowhere left to go back to in the stack
	runTimers();
	EXPECT_EQ(pages().shownPage(), "first");
}

TEST_F(TestPages, TheBackKeyLeavesAStackThatIsNotOnScreen)
{
	tap("to_second");
	view->hide();
	UI::back();
	runTimers();
	EXPECT_EQ(pages().shownPage(), "second");
}

TEST_F(TestPages, OpeningAPageThereIsNotLeavesThePageShown)
{
	tap("to_second");
	tap("to_nowhere");
	EXPECT_EQ(pages().shownPage(), "second");
	EXPECT_FALSE(pages().open("nowhere"));
	EXPECT_TRUE(pages().open("second")) << "the page shown, opened again, stays";
	tap("back");
	EXPECT_EQ(pages().shownPage(), "first");
}

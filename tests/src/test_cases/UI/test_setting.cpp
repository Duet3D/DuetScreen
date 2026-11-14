/*
 * test_setting.cpp
 *
 * <ds_setting> (src/UI/Widgets/Layout/DsSetting.h): a setting's row, which a tap anywhere on works its control; and
 * <ds_setting_group> (DsSettingGroup.h), a group of them under a title.
 */

#include "UI/Widgets/Layout/DsSetting.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "helix_xml.h"
#include "test_utils/Dialogs.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

/** Holds what the settings show, and records what they are changed to */
class SettingPresenter : public UI::Xml::XmlPresenter
{
  public:
	SettingPresenter()
		: XmlPresenter("test_setting")
	{
		on.set(0);
		options.set("Red\nGreen\nBlue");
		chosen.set(0);
		title.set("Bound title");
		action("test.on",
			   "Record the switch",
			   [this](const UI::Xml::ActionArgs& args) { on.set(args.value().value_or(-1) != 0 ? 1 : 0); });
		action("test.choose",
			   "Record the choice",
			   [this](const UI::Xml::ActionArgs& args)
			   { chosen.set(static_cast<int32_t>(args.value().value_or(-1))); });
	}

	void fields(UI::Xml::FieldVisitor& f) override
	{
		f.field("on", on, "1 if the switch is on");
		f.field("options", options, "The choices");
		f.field("chosen", chosen, "The choice chosen");
		f.field("title", title, "A title from a subject");
	}

	UI::Xml::IntSubject on;				// The switch
	UI::Xml::StringSubject<64> options; // The choices
	UI::Xml::IntSubject chosen;			// The choice chosen
	UI::Xml::StringSubject<32> title;	// A title from a subject
};

class TestSetting : public UiTestSuite
{
  protected:
	TestSetting()
	{
		lv_xml_register_component_from_data("test_setting", R"(
			<component>
				<view extends="lv_obj" width="500" height="content" flex_flow="column">
					<ds_setting_group name="group" title="Colours, and the rest">
						<ds_setting name="switch_row" title="A switch" hint="With a hint">
							<ds_toggle name="switch">
								<bind_state_if_eq subject="presenter.on" state="checked" ref_value="1"/>
								<action trigger="value_changed" name="presenter.test.on"/>
							</ds_toggle>
						</ds_setting>
						<ds_setting name="choice_row" bind_title="presenter.title">
							<ds_choice name="choice" title="Colour" bind_options="presenter.options"
									   bind_selected="presenter.chosen">
								<action trigger="value_confirmed" name="presenter.test.choose"/>
							</ds_choice>
						</ds_setting>
						<ds_setting name="fact_row" title="A fact">
							<lv_label text="42"/>
						</ds_setting>
					</ds_setting_group>
				</view>
			</component>)");
		view = std::make_unique<UI::Xml::XmlView>("setting_view", screen, "test_setting", presenter);
		view->show();
		lv_obj_update_layout(screen.getRootPtr());
	}

	/** The widget called `name` in the view */
	lv_obj_t* part(const char* name) const { return lv_obj_find_by_name(view->getRootPtr(), name); }

	/** The row called `name` */
	UI::Xml::DsSetting& row(const char* name) const
	{
		return *static_cast<UI::Xml::DsSetting*>(UI::LvObj::fromPtr(part(name)));
	}

	/** The text of the title or hint (`label`) of the row called `name` */
	std::string text(const char* name, const char* label) const
	{
		return lv_label_get_text(lv_obj_find_by_name(part(name), label));
	}

	std::shared_ptr<SettingPresenter> presenter = std::make_shared<SettingPresenter>(); // What the settings show
	std::unique_ptr<UI::Xml::XmlView> view;												// The view holding the settings
};

TEST_F(TestSetting, ARowShowsItsTitleAndAnyHint)
{
	EXPECT_EQ(text("switch_row", "title"), "A switch");
	EXPECT_EQ(text("switch_row", "hint"), "With a hint");
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(part("fact_row"), "hint"), LV_OBJ_FLAG_HIDDEN)) << "no hint";
	EXPECT_EQ(text("choice_row", "title"), "Bound title");
	presenter->title.set("Another");
	EXPECT_EQ(text("choice_row", "title"), "Another");
	EXPECT_EQUAL_SCREENSHOT("setting/group.png")
}

TEST_F(TestSetting, TappingTheRowTurnsItsSwitchOverAsTappingTheSwitchWould)
{
	EXPECT_EQ(row("switch_row").control(), part("switch"));
	lv_obj_send_event(part("switch_row"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(presenter->on.get(), 1);
	EXPECT_TRUE(lv_obj_has_state(part("switch"), LV_STATE_CHECKED));
	lv_obj_send_event(part("switch_row"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(presenter->on.get(), 0);

	// Not while the switch is disabled
	lv_obj_add_state(part("switch"), LV_STATE_DISABLED);
	lv_obj_send_event(part("switch_row"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(presenter->on.get(), 0);
}

TEST_F(TestSetting, TappingTheRowOpensItsChoice)
{
	lv_obj_send_event(part("choice_row"), LV_EVENT_CLICKED, nullptr);
	ASSERT_EQ(TestDialogs::choiceCount(), 3u);
	EXPECT_EQ(TestDialogs::title(), "Colour");
	TestDialogs::choose(2);
	EXPECT_EQ(presenter->chosen.get(), 2);
	runTimers();
}

TEST_F(TestSetting, ARowWithNothingToWorkIsNotPressed)
{
	EXPECT_EQ(row("fact_row").control(), nullptr);
	lv_obj_send_event(part("fact_row"), LV_EVENT_PRESSED, nullptr);
	EXPECT_FALSE(lv_obj_has_state(part("fact_row"), LV_STATE_PRESSED));
	lv_obj_send_event(part("switch_row"), LV_EVENT_PRESSED, nullptr);
	EXPECT_TRUE(lv_obj_has_state(part("switch_row"), LV_STATE_PRESSED));
}

TEST_F(TestSetting, AGroupBoxesItsRowsUnderItsTitleInCapitals)
{
	lv_obj_t* group = part("group");
	lv_obj_t* rows = lv_obj_find_by_name(group, "rows");
	ASSERT_NE(rows, nullptr);
	EXPECT_EQ(lv_obj_get_parent(part("switch_row")), rows) << "the rows written in the group are in its box";
	EXPECT_EQ(lv_obj_get_parent(part("fact_row")), rows);
	EXPECT_EQ(lv_obj_get_child_count(group), 2u) << "the group holds only its title and box";
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(group, "title")), "COLOURS, AND THE REST");
	EXPECT_EQ(lv_obj_get_parent(part("choice")), part("choice_row")) << "what is in a row stays in the row";
}

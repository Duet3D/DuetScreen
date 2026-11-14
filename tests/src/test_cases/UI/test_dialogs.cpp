/*
 * test_dialogs.cpp
 *
 * The dialog surface (src/UI/Xml/Dialogs/DialogHost.h): one dialog at a time, anything opened from it a step of it;
 * the ways out (close, back, tapping outside), blocking steps, alerts set aside, dialogs going with their views;
 * prompts asked from code (Prompts.h), handing their answers back; and room for the on-screen keyboard.
 */

#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/Dialogs.h"
#include "test_utils/UiTestSuite.h"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	/** Opens the test dialogs, and records what they raise */
	class DialogsPresenter : public UI::Xml::XmlPresenter
	{
	  public:
		DialogsPresenter()
			: XmlPresenter("test_dialogs")
		{
			for (const char* name : {"a", "b", "alert"})
			{
				action(fmt::format("test.dismissed_{:s}", name),
					   "Record that the dialog was dismissed, and close it",
					   [this, name = std::string(name)](const UI::Xml::ActionArgs&)
					   {
						   calls.push_back("dismissed " + name);
						   subject(name).set(0);
					   });
			}
		}

		void fields(UI::Xml::FieldVisitor& f) override
		{
			f.field("a", a, "Opens dialog A");
			f.field("b", b, "Opens dialog B");
			f.field("alert", alert, "Opens the alert");
			f.field("b_blocking", bBlocking, "Opens the blocking dialog");
			f.field("untitled", untitled, "Opens the blocking dialog without a title");
			f.field("form", form, "Opens the form; open from the start");
		}

		/** The subject opening the dialog called `name` */
		UI::Xml::IntSubject& subject(const std::string& name) { return name == "a" ? a : name == "b" ? b : alert; }

		UI::Xml::IntSubject a;
		UI::Xml::IntSubject b;
		UI::Xml::IntSubject alert;
		UI::Xml::IntSubject bBlocking;
		UI::Xml::IntSubject untitled;
		UI::Xml::IntSubject form{1};
		std::vector<std::string> calls;
	};

	/* The surface, two dialogs, a blocking one and an alert */
	constexpr std::string_view DIALOGS_SCREEN = R"(
		<component presenter="test_dialogs">
			<view extends="lv_obj" width="100%" height="100%" style_pad_all="0">
				<ds_dialog_host name="dialogs" width="100%" height="100%"/>
				<ds_dialog name="a" bind_open="presenter.a" title="Dialog A">
					<action trigger="dismissed" name="presenter.test.dismissed_a"/>
					<lv_label name="text" text="A"/>
				</ds_dialog>
				<ds_dialog name="b" bind_open="presenter.b" title="Dialog B" width="50%">
					<action trigger="dismissed" name="presenter.test.dismissed_b"/>
					<lv_label name="text" text="B"/>
				</ds_dialog>
				<ds_dialog name="blocking" bind_open="presenter.b_blocking" title="Must answer" blocking="true">
					<lv_label name="text" text="Blocking"/>
				</ds_dialog>
				<ds_dialog name="untitled" bind_open="presenter.untitled" blocking="true">
					<lv_label name="text" text="Untitled"/>
				</ds_dialog>
				<ds_dialog name="alert" bind_open="presenter.alert" title="Alert" alert="true">
					<action trigger="dismissed" name="presenter.test.dismissed_alert"/>
					<lv_label name="text" text="Alert"/>
					<ds_dialog_footer name="footer">
						<ds_button name="done" text="Done"/>
					</ds_dialog_footer>
				</ds_dialog>
				<ds_keyboard name="keyboard" hidden="true" width="100%" height="50%" align="bottom_mid" floating="true"/>
			</view>
		</component>)";

	/* A dialog on its own, with no surface: it opens in the screen's */
	constexpr std::string_view LONE_SCREEN = R"(
		<component presenter="test_dialogs">
			<view extends="lv_obj" width="0" height="0">
				<ds_dialog name="lone" bind_open="presenter.a" title="Lone">
					<lv_label name="text" text="Lone"/>
				</ds_dialog>
			</view>
		</component>)";

	/* A dialog of many text boxes, open from the start, the last far below the room left by the keyboard */
	constexpr std::string_view FORM_SCREEN = R"(
		<component presenter="test_dialogs">
			<view extends="lv_obj" width="100%" height="100%" style_pad_all="0">
				<ds_dialog_host name="dialogs" width="100%" height="100%"/>
				<ds_dialog name="form" bind_open="presenter.form" title="Form" flex_flow="column">
					<ds_textarea name="box_0" one_line="true" width="100%"/>
					<ds_textarea name="box_1" one_line="true" width="100%"/>
					<ds_textarea name="box_2" one_line="true" width="100%"/>
					<ds_textarea name="box_3" one_line="true" width="100%"/>
					<ds_textarea name="box_4" one_line="true" width="100%"/>
					<ds_textarea name="box_5" one_line="true" width="100%"/>
					<ds_textarea name="box_6" one_line="true" width="100%"/>
					<ds_textarea name="box_7" one_line="true" width="100%"/>
					<ds_textarea name="box_8" one_line="true" width="100%"/>
					<ds_textarea name="box_9" one_line="true" width="100%"/>
					<ds_textarea name="box_10" one_line="true" width="100%"/>
					<ds_textarea name="box_11" one_line="true" width="100%"/>
					<ds_textarea name="box_12" one_line="true" width="100%"/>
					<ds_textarea name="box_13" one_line="true" width="100%"/>
					<ds_textarea name="box_14" one_line="true" width="100%"/>
					<ds_textarea name="box_15" one_line="true" width="100%"/>
					<ds_textarea name="box_16" one_line="true" width="100%"/>
					<ds_textarea name="box_17" one_line="true" width="100%"/>
					<ds_textarea name="box_18" one_line="true" width="100%"/>
					<ds_textarea name="box_19" one_line="true" width="100%"/>
					<ds_textarea name="box_20" one_line="true" width="100%"/>
					<ds_textarea name="box_21" one_line="true" width="100%"/>
					<ds_textarea name="box_22" one_line="true" width="100%"/>
					<ds_textarea name="box_23" one_line="true" width="100%"/>
					<ds_dialog_footer name="footer">
						<ds_button name="done" text="Done"/>
					</ds_dialog_footer>
				</ds_dialog>
				<ds_keyboard name="keyboard" hidden="true" width="100%" height="50%" align="bottom_mid" floating="true"/>
			</view>
		</component>)";
} // namespace

class DialogSurface : public UiTestSuite
{
  protected:
	DialogSurface()
	{
		UI::Xml::Presenters::add<DialogsPresenter>("test_dialogs");
		m_userDir = fs::temp_directory_path() / fmt::format("duetscreen-dialogs-{:d}", getpid());
		fs::remove_all(m_userDir);
		fs::create_directories(m_userDir / "test" / "components");
		std::ofstream(m_userDir / "test" / "manifest.json")
			<< R"({"id": "test", "ui_api": "1.0", "components": ["components/*.xml"]})";
		std::ofstream(m_userDir / "test" / "components" / "dialogs_screen.xml") << DIALOGS_SCREEN;
		std::ofstream(m_userDir / "test" / "components" / "form_screen.xml") << FORM_SCREEN;
		std::ofstream(m_userDir / "test" / "components" / "lone_screen.xml") << LONE_SCREEN;
		UI::Xml::deinit();
		UI::Xml::init(UI::Xml::Paths{.builtin = "assets/ui", .user = m_userDir});
	}

	~DialogSurface() override
	{
		m_view.reset();
		fs::remove_all(m_userDir);
	}

	/** Show `component` filling the screen */
	UI::Xml::XmlView& show(const char* component)
	{
		m_view = std::make_unique<UI::Xml::XmlView>(component, lv_screen_active(), component);
		m_view->setSize(lv_pct(100), lv_pct(100));
		m_view->show();
		runTimers();
		return *m_view;
	}

	/** The dialogs screen's presenter */
	DialogsPresenter& presenter() const { return static_cast<DialogsPresenter&>(*m_view->getPresenter()); }

	/** The surface's widget called `name` (its backdrop, card, header buttons) */
	static lv_obj_t* chrome(const char* name) { return lv_obj_find_by_name(TestDialogs::host().root(), name); }

	/** Whether the surface's widget called `name` shows */
	static bool chromeShows(const char* name) { return !lv_obj_has_flag(chrome(name), LV_OBJ_FLAG_HIDDEN); }

	fs::path m_userDir;
	std::unique_ptr<UI::Xml::XmlView> m_view;
};

TEST_F(DialogSurface, ADialogOpenedFromAnotherIsItsNextStep)
{
	show("dialogs_screen");
	presenter().a.set(1);
	ASSERT_NE(TestDialogs::shownNamed("a"), nullptr);
	EXPECT_EQ(TestDialogs::title(), "Dialog A");
	EXPECT_FALSE(chromeShows("back")) << "the first step has no back button";
	EXPECT_TRUE(chromeShows("close"));

	// B opens while A is open: it is A's next step, coming from A
	presenter().b.set(1);
	ASSERT_NE(TestDialogs::shownNamed("b"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 2u);
	EXPECT_EQ(TestDialogs::title(), "Dialog B");
	EXPECT_EQ(std::string(lv_label_get_text(chrome("from"))), "Dialog A");
	EXPECT_TRUE(chromeShows("back"));

	// Back: B is dismissed, A shows again as it was
	TestDialogs::tapHeader("back");
	EXPECT_NE(TestDialogs::shownNamed("a"), nullptr);
	EXPECT_EQ(presenter().calls, (std::vector<std::string>{"dismissed b"}));
	EXPECT_EQ(presenter().b.get(), 0);

	// Close: the whole dialog
	presenter().b.set(1);
	TestDialogs::tapHeader("close");
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	EXPECT_EQ(presenter().calls, (std::vector<std::string>{"dismissed b", "dismissed b", "dismissed a"}));
	EXPECT_TRUE(lv_obj_has_flag(TestDialogs::host().root(), LV_OBJ_FLAG_HIDDEN));
}

TEST_F(DialogSurface, ClosingAStepByItsSubjectTakesTheStepsAboveIt)
{
	show("dialogs_screen");
	presenter().a.set(1);
	presenter().b.set(1);

	// B closed by its own subject: back to A, nothing dismissed
	presenter().b.set(0);
	EXPECT_NE(TestDialogs::shownNamed("a"), nullptr);
	EXPECT_TRUE(presenter().calls.empty());

	// A closed by its subject takes B with it, which is told
	presenter().b.set(1);
	presenter().a.set(0);
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	EXPECT_EQ(presenter().calls, (std::vector<std::string>{"dismissed b"}));
}

TEST_F(DialogSurface, TappingOutsideClosesTheDialogUnlessAStepIsBlocking)
{
	show("dialogs_screen");
	presenter().a.set(1);
	presenter().b.set(1);
	lv_obj_send_event(chrome("backdrop"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(TestDialogs::shown(), nullptr);

	// A blocking step: no close or back button, and tapping outside does nothing
	presenter().calls.clear();
	presenter().a.set(1);
	presenter().bBlocking.set(1);
	ASSERT_NE(TestDialogs::shownNamed("blocking"), nullptr);
	EXPECT_FALSE(chromeShows("close"));
	EXPECT_FALSE(chromeShows("back"));
	lv_obj_send_event(chrome("backdrop"), LV_EVENT_CLICKED, nullptr);
	EXPECT_NE(TestDialogs::shownNamed("blocking"), nullptr);
	EXPECT_TRUE(presenter().calls.empty());
	presenter().bBlocking.set(0);
	presenter().a.set(0);

	// A blocking dialog without a title has nothing for its header to show: no header
	presenter().untitled.set(1);
	ASSERT_NE(TestDialogs::shownNamed("untitled"), nullptr);
	EXPECT_FALSE(chromeShows("header"));
	presenter().untitled.set(0);
}

TEST_F(DialogSurface, AnAlertSetsTheDialogAsideUntilItIsAnswered)
{
	show("dialogs_screen");
	presenter().a.set(1);
	presenter().b.set(1);
	presenter().alert.set(1);
	ASSERT_NE(TestDialogs::shownNamed("alert"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 1u);
	EXPECT_FALSE(chromeShows("back"));

	// Closing the alert brings the dialog back as it was, untold
	TestDialogs::tapHeader("close");
	EXPECT_NE(TestDialogs::shownNamed("b"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 2u);
	EXPECT_EQ(presenter().calls, (std::vector<std::string>{"dismissed alert"}));

	// Navigation closing the user's dialog leaves an alert
	presenter().alert.set(1);
	UI::closeDialogs();
	EXPECT_NE(TestDialogs::shownNamed("alert"), nullptr);
	presenter().alert.set(0);
	EXPECT_EQ(TestDialogs::shown(), nullptr);
}

TEST_F(DialogSurface, ADialogGoesWithItsView)
{
	show("dialogs_screen");
	presenter().a.set(1);
	auto lone = std::make_unique<UI::Xml::XmlView>("lone_screen", lv_screen_active(), "lone_screen");
	lone->show();
	static_cast<DialogsPresenter&>(*lone->getPresenter()).a.set(1);
	ASSERT_NE(TestDialogs::shownNamed("lone"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 2u);

	// The lone dialog's view goes: so does its step, and the dialog under it shows again
	lone.reset();
	EXPECT_NE(TestDialogs::shownNamed("a"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 1u);
}

TEST_F(DialogSurface, ADialogOpenFromTheStartOpensOnceTheViewIsBuilt)
{
	show("form_screen");
	ASSERT_NE(TestDialogs::shownNamed("form"), nullptr);
	EXPECT_EQ(TestDialogs::title(), "Form");
}

TEST_F(DialogSurface, PromptsHandTheirAnswerBackOnceGone)
{
	show("dialogs_screen");

	// A number, typed and confirmed
	std::vector<float> numbers;
	UI::Xml::Prompts::askNumber(
		{.header = "Speed", .min = 0, .max = 100, .value = 50, .onConfirm = [&](float v) { numbers.push_back(v); }});
	ASSERT_NE(TestDialogs::shownNamed("dialog_number"), nullptr);
	EXPECT_EQ(TestDialogs::title(), "Speed");
	EXPECT_EQ(TestDialogs::text("range"), "0 to 100");
	TestDialogs::enterOnPad("75");
	EXPECT_EQ(numbers, std::vector<float>{75});
	EXPECT_EQ(TestDialogs::shown(), nullptr);

	// A choice, asked from a dialog: its next step, choosing goes back to the dialog
	presenter().a.set(1);
	std::string chosen;
	UI::Xml::Prompts::askChoice({.title = "Colour",
								 .choices = {{.value = "r", .title = "Red"}, {.value = "g", .title = "Green"}},
								 .chosen = "g",
								 .onChoose = [&](const std::string& v) { chosen = v; }});
	ASSERT_NE(TestDialogs::shownNamed("dialog_choice"), nullptr);
	EXPECT_EQ(TestDialogs::host().shownSteps(), 2u);
	ASSERT_EQ(TestDialogs::choiceCount(), 2u);
	EXPECT_EQ(TestDialogs::chosenIndex(), 1);
	TestDialogs::choose(0);
	EXPECT_EQ(chosen, "r");
	EXPECT_NE(TestDialogs::shownNamed("a"), nullptr);

	// A question left without answering: no
	int yes = 0;
	int no = 0;
	UI::Xml::Prompts::askQuestion(
		{.title = "Sure?", .text = "Really?", .onYes = [&] { yes++; }, .onNo = [&] { no++; }});
	TestDialogs::tapHeader("back");
	EXPECT_EQ(no, 1);
	EXPECT_EQ(yes, 0);
	EXPECT_NE(TestDialogs::shownNamed("a"), nullptr);
	presenter().a.set(0);
	runTimers(); // The prompts gone are deleted after the event that answered them
}

TEST_F(DialogSurface, TheNumberPadTypesLikeACalculator)
{
	show("dialogs_screen");
	std::vector<float> numbers;
	const auto ask = [&](float min)
	{
		UI::Xml::Prompts::askNumber({.header = "Speed",
									 .min = min,
									 .max = 100,
									 .value = 50,
									 .onConfirm = [&](float v) { numbers.push_back(v); }});
		ASSERT_NE(TestDialogs::shownNamed("dialog_number"), nullptr);
	};
	const auto typed = [] { return TestDialogs::text("text"); };
	const auto okEnabled = [] { return !lv_obj_has_state(TestDialogs::find("ok"), LV_STATE_DISABLED); };

	// Backspace takes the last character, of the value it starts with too
	ask(0);
	EXPECT_EQ(typed(), "50");
	TestDialogs::tap("backspace");
	EXPECT_EQ(typed(), "5");
	TestDialogs::tap("cancel");

	// The first key replaces the value it starts with; then keys add to it
	ask(0);
	EXPECT_EQ(typed(), "50");
	TestDialogs::typeOnPad("7");
	EXPECT_EQ(typed(), "7");
	TestDialogs::typeOnPad("2.5.");
	EXPECT_EQ(typed(), "72.5") << "one dot at most";

	// Backspace takes the last key; the sign does nothing for a number that can't be negative
	TestDialogs::tap("backspace");
	EXPECT_EQ(typed(), "72.");
	EXPECT_TRUE(lv_obj_has_state(TestDialogs::find("sign"), LV_STATE_DISABLED));
	TestDialogs::tap("sign");
	EXPECT_EQ(typed(), "72.");

	// OK only for a number within the limits
	EXPECT_TRUE(okEnabled());
	TestDialogs::typeOnPad("9");
	TestDialogs::tap("clear");
	EXPECT_EQ(typed(), "");
	EXPECT_TRUE(okEnabled()) << "nothing typed is 0";
	TestDialogs::typeOnPad("101");
	EXPECT_FALSE(okEnabled());
	TestDialogs::tap("ok");
	EXPECT_NE(TestDialogs::shownNamed("dialog_number"), nullptr);

	// Cancel goes back, handing nothing back
	TestDialogs::tap("cancel");
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	runTimers();
	EXPECT_TRUE(numbers.empty());

	// A number that may be negative: the sign key works
	ask(-50);
	TestDialogs::enterOnPad("25-");
	EXPECT_EQ(numbers, std::vector<float>{-25});
	runTimers();
}

TEST_F(DialogSurface, APromptGoesWithItsAsker)
{
	show("dialogs_screen");
	lv_obj_t* asker = lv_obj_create(m_view->getRootPtr());
	int answers = 0;
	UI::Xml::Prompts::askNumber(
		{.header = "Speed", .min = 0, .max = 100, .onConfirm = [&](float) { answers++; }, .asker = asker});
	ASSERT_NE(TestDialogs::shownNamed("dialog_number"), nullptr);
	lv_obj_delete(asker);
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	runTimers();
	EXPECT_EQ(answers, 0);
}

TEST_F(DialogSurface, ATallDialogScrollsWithinTheRoomAndKeepsItsFooterInView)
{
	show("form_screen");
	lv_area_t room;
	lv_area_t card;
	lv_area_t footer;
	lv_obj_get_coords(chrome("backdrop"), &room);
	lv_obj_get_coords(chrome("card"), &card);
	ASSERT_NE(TestDialogs::find("done"), nullptr);
	lv_obj_get_coords(TestDialogs::find("done"), &footer);
	EXPECT_GE(card.y1, room.y1);
	EXPECT_LE(card.y2, room.y2) << "the card fits the room";
	EXPECT_LE(footer.y2, card.y2) << "its footer shows";
	EXPECT_TRUE(lv_obj_has_flag(chrome("body"), LV_OBJ_FLAG_SCROLLABLE));
	EXPECT_GT(lv_obj_get_scroll_bottom(chrome("body")), 0) << "the rest scrolls";
}

TEST_F(DialogSurface, TheCardKeepsAboveTheKeyboardAndShowsTheBoxTypedIn)
{
	show("form_screen");
	lv_obj_t* last = TestDialogs::find("box_23");
	ASSERT_NE(last, nullptr);
	lv_obj_send_event(last, LV_EVENT_FOCUSED, nullptr);
	runTimers();

	lv_obj_t* keyboard = lv_obj_find_by_name(m_view->getRootPtr(), "keyboard");
	ASSERT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
	lv_area_t keys;
	lv_area_t card;
	lv_area_t body;
	lv_area_t box;
	lv_obj_get_coords(keyboard, &keys);
	lv_obj_get_coords(chrome("card"), &card);
	lv_obj_get_coords(chrome("body"), &body);
	lv_obj_get_coords(last, &box);
	EXPECT_LT(card.y2, keys.y1) << "the card keeps above the keyboard";
	EXPECT_LE(body.y2, card.y2) << "its body within it, scrolling";
	EXPECT_GE(box.y1, body.y1) << "the box is scrolled into view";
	EXPECT_LE(box.y2, body.y2);

	// The keyboard going gives the room back
	lv_obj_send_event(last, LV_EVENT_READY, nullptr);
	runTimers();
	lv_obj_get_coords(chrome("card"), &card);
	EXPECT_GT(card.y2, keys.y1);
}

TEST_F(DialogSurface, ScrollingTheCardKeepsTheKeyboardButTappingOutsidePutsItAway)
{
	// With an input group, as the screen has: a press outside the box only tells the box it was left
	lv_group_t* group = lv_group_create();
	lv_group_set_default(group);
	lv_indev_t* mouse = lv_test_indev_get_indev(LV_INDEV_TYPE_POINTER);
	lv_indev_set_group(mouse, group);
	show("form_screen");
	lv_obj_t* keyboard = lv_obj_find_by_name(m_view->getRootPtr(), "keyboard");

	const auto drag = [mouse](lv_point_t from, lv_point_t to)
	{
		lv_test_mouse_move_to(from.x, from.y);
		lv_test_mouse_press();
		lv_indev_read(mouse);
		for (int step = 1; step <= 10; step++)
		{
			lv_test_mouse_move_to(from.x + (to.x - from.x) * step / 10, from.y + (to.y - from.y) * step / 10);
			lv_indev_read(mouse);
		}
		lv_test_mouse_release();
		lv_indev_read(mouse);
		runTimers();
	};
	const auto middleOf = [](lv_obj_t* obj)
	{
		lv_area_t area;
		lv_obj_get_coords(obj, &area);
		return lv_point_t{(area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2};
	};

	// Tapping the box brings up the keyboard
	const lv_point_t box = middleOf(TestDialogs::find("box_0"));
	drag(box, box);
	ASSERT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));

	// Dragging the card's contents up scrolls them, and the keyboard stays
	lv_area_t body;
	lv_obj_get_coords(chrome("body"), &body);
	const int32_t scrolled = lv_obj_get_scroll_y(chrome("body"));
	drag({body.x1 + 8, body.y2 - 8}, {body.x1 + 8, body.y1 + 8});
	EXPECT_GT(lv_obj_get_scroll_y(chrome("body")), scrolled);
	EXPECT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));

	// Tapping another box hands the keyboard on: it stays up, and the card stays where it is scrolled to
	const int32_t at = lv_obj_get_scroll_y(chrome("body"));
	lv_obj_t* other = nullptr;
	for (int i = 23; i >= 0 && other == nullptr; i--)
	{
		lv_obj_t* box = TestDialogs::find(fmt::format("box_{:d}", i).c_str());
		lv_area_t area;
		lv_obj_get_coords(box, &area);
		if (area.y1 >= body.y1 && area.y2 <= body.y2)
		{
			other = box; // The lowest box in view
		}
	}
	ASSERT_NE(other, nullptr);
	const lv_point_t next = middleOf(other);
	drag(next, next);
	EXPECT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQ(lv_keyboard_get_textarea(keyboard), other);
	EXPECT_EQ(lv_obj_get_scroll_y(chrome("body")), at);

	// A tap on the card's title, outside the box, puts it away
	const lv_point_t title = middleOf(chrome("title"));
	drag(title, title);
	EXPECT_TRUE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));

	lv_indev_set_group(mouse, nullptr);
	lv_group_set_default(nullptr);
	m_view.reset();
	lv_group_delete(group);
}

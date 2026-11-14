/*
 * DialogHost.h
 *
 * The dialog surface: one card over a dimmed backdrop, which every dialog on a screen shares. The home screen places
 * it over its main window with `<ds_dialog_host>`, so the sidebar and the E-stop stay usable; a screen without one
 * gets one covering it the first time a dialog opens there.
 *
 * The surface shows one step at a time (see DialogStep.h). The first step to open becomes the dialog; anything opened
 * while it is open becomes its next step, and the card slides sideways to it. The card's chrome is the XML component
 * `dialog_host` (assets/ui/components/dialogs/dialog_host.xml): a header with the step's title, the title of the
 * step it came from, a back button from the second step and a close button, then the step's content.
 *
 *  - The close button and tapping outside the card close the whole dialog, unless any of its steps is blocking.
 *  - The back button goes back one step; the back key does that, or on the first step closes the dialog.
 *  - A step closed by its owner takes the steps above it with it; a step the user leaves without answering tells its
 *    owner (DialogStep::dismissed()).
 *  - An alert is shown before everything else: the user's dialog is set aside, as it is, and comes back once every
 *    alert is answered. Alerts wait their turn, one at a time.
 *  - While the on-screen keyboard is up, the card keeps above it (see makeRoom()).
 *
 * The XML engine's elements are registered by registerXml(); a step is opened with open() and closed with close().
 */

#pragma once

#include "DialogStep.h"
#include "UI/Core/Navigation.h"
#include "helix_xml.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace UI::Xml
{
	class DialogHostPresenter;
	class XmlView;

	class DialogHost : public DialogLayer
	{
	  public:
		/**
		 * Make the surface inside `parent`, filling it
		 * @param navigation Whether it is the one navigation closes and goes back through (see Navigation.h)
		 */
		DialogHost(lv_obj_t* parent, bool navigation);

		/** Put every step's content back where it lives, and stop being navigation's */
		~DialogHost() override;

		DialogHost(const DialogHost&) = delete;
		DialogHost& operator=(const DialogHost&) = delete;

		/** The surface's root widget: its chrome's */
		lv_obj_t* root() const;

		/** Register `<ds_dialog_host>`, the chrome's presenter and `<ds_dialog>` (DsDialog.h) */
		static void registerXml();

		/** The surface of the screen `obj` is on, made over that whole screen if it has none */
		static DialogHost& of(lv_obj_t* obj);

		/** The surface whose card `obj` is in, or nullptr */
		static DialogHost* holding(lv_obj_t* obj);

		/** Whether `host` is a surface still, e.g. one remembered while its screen may have gone */
		static bool exists(const DialogHost* host);

		/** The surface `step` is on, or nullptr */
		static DialogHost* showing(const DialogStep& step);

		/** Show `step`: the dialog, or its next step, or an alert. Its owner keeps it, and must close() or forget() it. */
		void open(DialogStep& step);

		/** As open(), for a step the surface owns from now on: it is deleted when it goes */
		void open(std::unique_ptr<DialogStep> step);

		/** Close `step`, as its owner chose: it goes without being told, and the steps above it are dismissed */
		void close(DialogStep& step);

		/** `step` is being deleted: take it off the surface without telling it, as close() */
		void forget(DialogStep& step);

		/**
		 * `step` has been answered: take just it off the surface, without telling it. One the surface owns is deleted
		 * once the event being handled is over, so the code answering (its presenter) can finish first.
		 */
		void finish(DialogStep& step);

		/** Where the contents of the steps the surface owns live while not shown (hidden) */
		lv_obj_t* parking() const { return m_parking; }

		/** Whether `step` is on the surface, shown or not */
		bool has(const DialogStep& step) const;

		/** The content of the step shown, or nullptr if nothing is */
		lv_obj_t* shownContent() const;

		/** How many steps the dialog shown has (an alert's, else the user's); 0 if nothing is shown */
		size_t shownSteps() const { return shown().size(); }

		/** The close button: dismiss the whole dialog shown, unless a step of it is blocking */
		void closeShown();

		/** The back button: dismiss the step on top, unless it is blocking */
		void goBack();

		/** Tapping outside the card: as the close button */
		void tappedOutside() { closeShown(); }

		/**
		 * The on-screen keyboard is up, its top at `keyboardTop` (screen coordinates): keep the card above it, and
		 * scroll `focused` into view inside it
		 */
		void makeRoom(int32_t keyboardTop, lv_obj_t* focused);

		/** The keyboard has gone: the card may use the whole surface again */
		void giveRoomBack();

		/* Navigation (see DialogLayer in Navigation.h) */

		/** The back key: go back a step, or close the dialog on its first; false if nothing is open */
		bool back() override;

		/** Dismiss the user's dialog (not an alert), e.g. when the page changes */
		void closeAll() override;

		/** Dismiss the steps above the highest one `obj` is in, or every step if it is in none */
		void closeNotAround(lv_obj_t* obj) override;

		/** Whether a dialog or alert is showing */
		bool isOpen() const override;

	  private:
		/** A step on the surface */
		struct Entry
		{
			DialogStep* step;					 // The step
			std::unique_ptr<DialogStep> owned{}; // The step, if the surface owns it
		};

		/** How the card changes to the step now on top */
		enum class Motion
		{
			none,	  // At once
			forward,  // The new step slides in from the right
			backward, // The step gone back to slides in from the left
		};

		/** The steps shown: the first alert's, else the user's */
		std::vector<Entry>& shown();

		/** The steps shown, unchanged */
		const std::vector<Entry>& shown() const;

		/** The stack `step` is in, and where, if it is on the surface */
		std::optional<std::pair<std::vector<Entry>*, size_t>> find(const DialogStep& step);

		/** Add `entry` to the surface and show what is now on top */
		void add(Entry entry);

		/**
		 * Take the steps of `stack` from `index` up off the surface, the top first, telling those `dismissFrom` and
		 * above they were dismissed, and deleting those the surface owns
		 */
		void remove(std::vector<Entry>& stack, size_t index, size_t dismissFrom);

		/** Scroll the box the keyboard types into into view, once the card has made room for the keyboard */
		static void scrollToFocusedCb(void* host);

		/** Delete the steps that have gone, once nothing is using them */
		static void deleteRetiredCb(void* host);

		/** Whether any step of `stack` is blocking */
		static bool anyBlocking(const std::vector<Entry>& stack);

		/** Show the step now on top (or nothing), moving to it as `motion` says */
		void update(Motion motion);

		/** Move `step`'s content back home, hidden */
		void park(DialogStep& step);

		/** Show the footer written in `content`, if any, under the body; else hide the footer's place */
		void showFooter(lv_obj_t* content);

		/** Put the footer shown back in `content`, where it was written */
		void returnFooter(lv_obj_t* content);

		/** The footer shown was deleted: forget it */
		static void footerDeletedCb(lv_event_t* e);

		/** Size and place the card for `step`, in the room there is */
		void layout(DialogStep& step);

		/** Slide `content` in from `fromX` (pixels) to its place in the body */
		void slideIn(lv_obj_t* content, int32_t fromX);

		/** Tell the chrome's presenter what the header shows */
		void updateHeader();

		/** Tell navigation whether a dialog is open */
		void notifyNavigation() const;

		lv_obj_t* m_parent;					   // The widget the surface fills
		std::unique_ptr<XmlView> m_chrome;		   // The dialog_host component: backdrop, card, header, body
		std::shared_ptr<DialogHostPresenter> m_presenter; // The chrome's presenter
		lv_obj_t* m_card = nullptr;				   // The card
		lv_obj_t* m_body = nullptr;				   // Where the step's content goes
		lv_obj_t* m_footer = nullptr;			   // Where the step's footer goes, under the body; hidden without one
		lv_obj_t* m_shownFooter = nullptr;		   // The footer of the step shown, moved there from its content
		uint32_t m_footerIndex = 0;				   // Where that footer goes back to among its content's children
		lv_obj_t* m_parking = nullptr;			   // Hidden: where owned steps' contents live while not shown
		bool m_navigation;						   // Whether navigation goes through it

		std::vector<Entry> m_steps;			 // The user's dialog, its first step first
		std::vector<std::vector<Entry>> m_alerts; // Alerts, the one shown first; each is a stack of its own
		DialogStep* m_top = nullptr;		 // The step whose content is in the card
		std::vector<std::unique_ptr<DialogStep>> m_retired; // Owned steps gone, to delete after the current event
		std::optional<int32_t> m_keyboardTop; // The keyboard's top, screen coordinates, while it is up
		lv_obj_t* m_focused = nullptr;		 // The text box the keyboard types into, while it is up
	};
} // namespace UI::Xml

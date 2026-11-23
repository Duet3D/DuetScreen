/*
 * DsPages.h
 *
 * <ds_pages>: pages that share one place, the first shown to start with. Opening another (open(), or the global action
 * `actions.navigation.open_page` from inside the stack) slides it in over the one shown, under a header with its title
 * and a button back to the page it came from; going back (that button, or the back key while the stack is on screen,
 * see addBackLayer() in Navigation.h) slides back. Each page is a <ds_pages-page> inside it. With `bind_page`, a text
 * subject names the page shown: setting it shows that page (going back to it if it was opened before), and the user
 * moving between pages sets it. Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "UI/Widgets/Buttons/Button.h"
#include "helix_xml.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class DsPages : public LvContainer
	{
	  public:
		/** Create a stack, with no pages, inside `parent` */
		DsPages(const std::string& name, lv_obj_t* parent);

		/** Stop the back key going back through the stack */
		~DsPages() override;

		/** Register `<ds_pages>` and `<ds_pages-page>` with the XML engine */
		static void registerXml();

		/** Show the page called `name` over the one shown, unless it is shown. @return false if there is no such page */
		bool open(std::string_view name);

		/** Go back to the page the one shown came from. @return false if it is the first, with nowhere to go back to */
		bool back() override;

		/** The name of the page shown, or empty if there are no pages */
		std::string_view shownPage() const;

		/** The innermost stack around `obj` (or `obj` itself) with a page called `name`, or nullptr */
		static DsPages* around(lv_obj_t* obj, std::string_view name);

	  private:
		/** One page */
		struct Page
		{
			std::unique_ptr<LvContainer> content; // Where the page's XML children are built
			std::string name;					  // The page's name, which opens it
			std::string title;					  // Its title, shown in the header
			std::string titleTag;				  // Its title's translation key, or empty if the title isn't translated
		};

		/** `<ds_pages>` create callback: create the C++ stack, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_pages>` apply callback: lv_obj attributes */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_pages-page>` create callback: add a page, whose content area its children are built in */
		static void* xmlCreatePage(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_pages-page>` apply callback: lv_obj attributes on the page's content area */
		static void xmlApplyPage(lv_xml_parser_state_t* state, const char** attrs);

		/** Translate the pages' titles again after a language change */
		static void languageChangedCb(lv_event_t* e);

		/** `bind_page`'s subject changed: show the page it names */
		static void pageObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/**
		 * Show the page called `name` (the first if empty): go back to it if it was opened on the way to the one shown,
		 * else open it over the one shown. Nothing if there is no such page (yet: it may be written later)
		 */
		void showNamed(std::string_view name);

		/** Set `bind_page`'s subject, if there is one, to the page shown */
		void tellShown();

		/** Add a page called `name` with `title` (its translation key `titleTag`, if it has one); the first is shown */
		LvContainer& addPage(std::string name, std::string title, std::string titleTag);

		/** Index of the page called `name`, or the number of pages if there is none */
		size_t indexOf(std::string_view name) const;

		/** Show page `to` in place of page `from`, sliding forward (1) or back (-1), and the header to suit it */
		void slideTo(size_t from, size_t to, int32_t direction);

		/** Show the header for the page shown: hidden on the first, else its title and the way back */
		void showHeader();

		LvContainer m_header{"header", getRoot()}; // Over a page opened from another: the way back, and its title
		Button m_back{"back", m_header, ""};		  // Back to the page the one shown came from, named after it
		LvLabel m_title{"title", m_header};		  // The title of the page shown
		LvContainer m_body{"body", getRoot()};	  // Where the pages are, one shown at a time
		std::vector<Page> m_pages;				  // The pages, in the order written
		std::vector<size_t> m_shown;			  // The pages opened, each from the one before; the one shown last
		lv_subject_t* m_page = nullptr;			  // `bind_page`: names the page shown; nullptr if not bound
		bool m_telling = false;					  // Setting m_page, so its change isn't followed back
	};
} // namespace UI::Xml

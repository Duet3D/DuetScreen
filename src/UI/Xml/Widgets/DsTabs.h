/*
 * DsTabs.h
 *
 * <ds_tabs>: DuetScreen's TabView (a row of tab buttons over the selected tab's content) in XML.
 *
 *     <ds_tabs width="100%" height="100%">
 *         <ds_tabs-tab name="macros" title_tag="file.macros">
 *             …the tab's content…
 *         </ds_tabs-tab>
 *         <ds_tabs-tab name="jobs" title_tag="file.jobs" bind_disabled="machine.job_active">
 *             …
 *         </ds_tabs-tab>
 *     </ds_tabs>
 *
 * The first tab starts selected. A tab's title is `title_tag` translated, or `title`. `bind_disabled` disables the
 * tab's button while an integer subject is non-zero. `select_on` selects the tab whenever a subject changes to a
 * non-empty string or non-zero integer, e.g. the status tab when a job starts: `select_on="job.file"`. Both elements
 * take every lv_obj attribute; a tab's apply to its content area.
 */

#pragma once

#include "UI/Components/Containers/TabView.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsTabs : public TabView
	{
	  public:
		/** Create a tab view, with no tabs, inside `parent` */
		DsTabs(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_tabs>` and `<ds_tabs-tab>` with the XML engine */
		static void registerXml();

	  private:
		/** Which tab a `bind_disabled` or `select_on` observer acts on */
		struct TabRef
		{
			DsTabs* tabs;		// The tab view
			size_t index;		// The tab
			bool armed = false; // select_on: the first notification is the current value, not a change
		};

		/** `<ds_tabs>` create callback: create the C++ tab view, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_tabs>` apply callback: lv_obj attributes */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_tabs-tab>` create callback: add a tab, whose content area its children are built in */
		static void* xmlCreateTab(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_tabs-tab>` apply callback: lv_obj attributes on the content area, then `bind_disabled` and `select_on` */
		static void xmlApplyTab(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_disabled` observer */
		static void disabledObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `select_on` observer */
		static void selectObserverCb(lv_observer_t* observer, lv_subject_t* subject);
	};
} // namespace UI::Xml

/*
 * DsTabs.cpp
 */

#include "DsTabs.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "i18n/i18n.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The DsTabs that `tab`, a tab's content area, belongs to, and its index; tabs is nullptr if none */
		std::pair<DsTabs*, size_t> tabOf(lv_obj_t* tab)
		{
			lv_obj_t* content = lv_obj_get_parent(tab);
			lv_obj_t* root = content ? lv_obj_get_parent(content) : nullptr;
			auto* tabs = root ? dynamic_cast<DsTabs*>(LvObj::fromPtr(root)) : nullptr;
			if (tabs == nullptr)
			{
				return {nullptr, 0};
			}
			for (size_t i = 0; i < tabs->getTabCount(); i++)
			{
				if (tabs->getTab(i)->getRootPtr() == tab)
				{
					return {tabs, i};
				}
			}
			return {nullptr, 0};
		}
	} // namespace

	DsTabs::DsTabs(const std::string& name, lv_obj_t* parent)
		: TabView(name, parent)
	{
	}

	void DsTabs::registerXml()
	{
		Widgets::add({.name = "ds_tabs",
					  .extends = "lv_obj",
					  .description = "A row of tab buttons over the selected tab's content. Each tab is a "
									 "`<ds_tabs-tab>` inside it; the first starts selected.",
					  .example = R"(<ds_tabs width="100%" height="200">
	<ds_tabs-tab name="macros" title_tag="file.macros">
		<lv_label text="The first tab's content"/>
	</ds_tabs-tab>
	<ds_tabs-tab name="jobs" title_tag="file.jobs" bind_disabled="machine.job_active">
		<lv_label text="The second tab's content"/>
	</ds_tabs-tab>
</ds_tabs>)"},
					 xmlCreate,
					 xmlApply);
		Widgets::add(
			{.name = "ds_tabs-tab",
			 .extends = "lv_obj",
			 .description = "One tab of a `<ds_tabs>`, written inside it. Its content is the tab's page; the "
							"lv_obj attributes apply to that page.",
			 .attributes = {{"title", "The tab button's text, if there is no `title_tag`"},
							{"title_tag", "Translation key for the tab button's text", AttributeKind::Translation},
							{"bind_disabled",
							 "Disable the tab's button while this int subject is non-zero",
							 AttributeKind::Subject},
							{"select_on",
							 "Select the tab whenever this subject changes to a non-empty text or non-zero "
							 "int, e.g. the status tab when a job starts: `select_on=\"job.file\"`",
							 AttributeKind::Subject}}},
			xmlCreateTab,
			xmlApplyTab);
	}

	void* DsTabs::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* tabs = new DsTabs(attrOr(attrs, "name", "tabs"), parentOf(state));
		deleteWith(tabs->getRootPtr(), tabs);
		return tabs->getRootPtr();
	}

	void DsTabs::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}

	void* DsTabs::xmlCreateTab(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* tabs = dynamic_cast<DsTabs*>(LvObj::fromPtr(parentOf(state)));
		if (tabs == nullptr)
		{
			LOG_WARN("<ds_tabs-tab> must be inside <ds_tabs>");
			return lv_obj_create(parentOf(state));
		}

		std::string title;
		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			title = _(tag);
		}
		else
		{
			title = attrOr(attrs, "title", "");
		}
		return tabs->addTab(title).getRootPtr();
	}

	void DsTabs::xmlApplyTab(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* tab = itemOf(state);
		const auto [tabs, index] = tabOf(tab);
		if (tabs == nullptr)
		{
			return;
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_disabled"))
		{
			auto* ref = new TabRef{tabs, index};
			deleteWith(tab, ref);
			lv_subject_add_observer_obj(subject, disabledObserverCb, tab, ref);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "select_on"))
		{
			auto* ref = new TabRef{tabs, index};
			deleteWith(tab, ref);
			lv_subject_add_observer_obj(subject, selectObserverCb, tab, ref);
		}
	}

	void DsTabs::selectObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* ref = static_cast<TabRef*>(lv_observer_get_user_data(observer));
		if (!ref->armed)
		{
			ref->armed = true;
			return;
		}
		const bool set = subject->type == LV_SUBJECT_TYPE_STRING ? lv_subject_get_string(subject)[0] != '\0'
																 : lv_subject_get_int(subject) != 0;
		if (set)
		{
			ref->tabs->setActiveTab(ref->index);
		}
	}

	void DsTabs::disabledObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		const auto* ref = static_cast<const TabRef*>(lv_observer_get_user_data(observer));
		ref->tabs->disableTab(ref->index, lv_subject_get_int(subject) != 0);
	}
} // namespace UI::Xml

/*
 * DsDropdown.cpp
 */

#include "DsDropdown.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "src/xml/parsers/lv_xml_dropdown_parser.h"
#include <vector>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsDropdown::DsDropdown(const std::string& name, lv_obj_t* parent)
		: DsDropdown(name, LvObjRef(parent))
	{
	}

	DsDropdown::DsDropdown(const std::string& name, LvObjRef&& parent)
		: LvDropdown(name, parent)
	{
	}

	void DsDropdown::registerXml()
	{
		Widgets::add(
			{.name = "ds_dropdown",
			 .extends = "lv_dropdown",
			 .description =
				 "A drop-down list whose options and text follow subjects. The option chosen reaches a `value_changed` "
				 "action as its text (ActionArgs::text()) and its index (ActionArgs::value()).",
			 .example = R"(<ds_dropdown bind_options="filaments" bind_text="filament_text">
	<action trigger="value_changed" name="temperature.pick_filament"/>
</ds_dropdown>)",
			 .attributes =
				 {{"bind_options", "Follow a text subject holding the options, one per line", AttributeKind::Subject},
				  {"bind_text",
				   "Follow a text subject shown on the button instead of the chosen option, e.g. what is loaded",
				   AttributeKind::Subject},
				  {"bind_selected",
				   "Follow an int subject for the index of the option shown chosen, kept when the options change",
				   AttributeKind::Subject}},
			 .exampleSubjects = {{"filaments", "string", "PLA\nPETG\nABS"}, {"filament_text", "string", "PLA"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsDropdown::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* dropdown = new DsDropdown(attrOr(attrs, "name", "dropdown"), parentOf(state));
		deleteWith(dropdown->getRootPtr(), dropdown);
		return dropdown->getRootPtr();
	}

	void DsDropdown::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		// lv_dropdown's own bind_selected would be applied before the options exist, so it is handled here, after them
		std::vector<const char*> others;
		for (int i = 0; attrs[i] != nullptr; i += 2)
		{
			if (!lv_streq(attrs[i], "bind_selected"))
			{
				others.push_back(attrs[i]);
				others.push_back(attrs[i + 1]);
			}
		}
		others.push_back(nullptr);
		lv_xml_dropdown_apply(state, others.data());

		lv_obj_t* dropdown = itemOf(state);
		auto* owner = static_cast<DsDropdown*>(LvObj::fromPtr(dropdown));
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_options"))
		{
			lv_subject_add_observer_obj(subject, optionsObserverCb, dropdown, owner);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_text"))
		{
			lv_subject_add_observer_obj(subject, textObserverCb, dropdown, owner);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_selected"))
		{
			owner->m_selected = subject;
			lv_subject_add_observer_obj(subject, selectedObserverCb, dropdown, owner);
		}
	}

	void DsDropdown::optionsObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& owner = *static_cast<DsDropdown*>(lv_observer_get_user_data(observer));
		lv_obj_t* dropdown = lv_observer_get_target_obj(observer);
		lv_dropdown_set_options(dropdown, lv_subject_get_string(subject));
		if (owner.m_selected != nullptr) // New options start at the first; show the bound one again
		{
			lv_dropdown_set_selected(dropdown, static_cast<uint32_t>(lv_subject_get_int(owner.m_selected)));
		}
	}

	void DsDropdown::selectedObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		lv_dropdown_set_selected(lv_observer_get_target_obj(observer),
								 static_cast<uint32_t>(lv_subject_get_int(subject)));
	}

	void DsDropdown::textObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& owner = *static_cast<DsDropdown*>(lv_observer_get_user_data(observer));
		owner.m_text = lv_subject_get_string(subject);
		lv_obj_t* dropdown = lv_observer_get_target_obj(observer);
		lv_dropdown_set_text(dropdown, owner.m_text.c_str());
		lv_obj_invalidate(dropdown);
	}
} // namespace UI::Xml

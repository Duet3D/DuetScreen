/*
 * DsDropdown.cpp
 */

#include "DsDropdown.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_dropdown_parser.h"

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
		lv_xml_register_widget("ds_dropdown", xmlCreate, xmlApply);
	}

	void* DsDropdown::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* dropdown = new DsDropdown(attrOr(attrs, "name", "dropdown"), parentOf(state));
		deleteWith(dropdown->getRootPtr(), dropdown);
		return dropdown->getRootPtr();
	}

	void DsDropdown::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_dropdown_apply(state, attrs);

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
	}

	void DsDropdown::optionsObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		lv_dropdown_set_options(lv_observer_get_target_obj(observer), lv_subject_get_string(subject));
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

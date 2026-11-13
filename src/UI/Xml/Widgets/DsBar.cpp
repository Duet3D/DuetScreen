/*
 * DsBar.cpp
 */

#include "DsBar.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsBar::DsBar(const std::string& name, lv_obj_t* parent)
		: DsBar(name, LvObjRef(parent))
	{
	}

	DsBar::DsBar(const std::string& name, LvObjRef&& parent)
		: Bar(name, parent)
	{
	}

	void DsBar::registerXml()
	{
		Widgets::add({.name = "ds_bar",
					  .extends = "lv_obj",
					  .description = "A progress bar that draws its value as text, inside the filled part when it fits",
					  .example = R"(<ds_bar min="0" max="100" label_format="{}%" bind_value="progress"/>)",
					  .attributes = {{"min", "Bottom of the range (default 0)"},
									 {"max", "Top of the range (default 100)"},
									 {"bind_min",
									  "Follow an int subject for the bottom of the range, e.g. a heater's minimum",
									  AttributeKind::Subject},
									 {"bind_max",
									  "Follow an int subject for the top of the range, e.g. a heater's maximum",
									  AttributeKind::Subject},
									 {"bind_value", "Follow an int subject for the value", AttributeKind::Subject},
									 {"label_format", "fmt format for the value's text (default \"{}\")"}},
					  .exampleSubjects = {{"progress", "int", "42"}}},
					 xmlCreate,
					 xmlApply);
	}

	void* DsBar::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* bar = new DsBar(attrOr(attrs, "name", "bar"), parentOf(state));
		deleteWith(bar->getRootPtr(), bar);
		return bar->getRootPtr();
	}

	void DsBar::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& bar = static_cast<DsBar&>(*LvObj::fromPtr(root));
		const char* min = lv_xml_get_value_of(attrs, "min");
		const char* max = lv_xml_get_value_of(attrs, "max");
		if (min || max)
		{
			bar.setRange(min ? lv_xml_atoi(min) : lv_bar_get_min_value(root),
						 max ? lv_xml_atoi(max) : lv_bar_get_max_value(root));
		}
		if (const char* format = lv_xml_get_value_of(attrs, "label_format"))
		{
			bar.setLabelFormat(format);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_min"))
		{
			lv_subject_add_observer_obj(subject, minObserverCb, root, &bar);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_max"))
		{
			lv_subject_add_observer_obj(subject, maxObserverCb, root, &bar);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_value"))
		{
			lv_bar_bind_value(root, subject);
		}
	}

	void DsBar::minObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& bar = *static_cast<DsBar*>(lv_observer_get_user_data(observer));
		bar.setMinValue(lv_subject_get_int(subject));
	}

	void DsBar::maxObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& bar = *static_cast<DsBar*>(lv_observer_get_user_data(observer));
		bar.setMaxValue(lv_subject_get_int(subject));
	}
} // namespace UI::Xml

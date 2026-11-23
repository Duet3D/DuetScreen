/*
 * DsArc.cpp
 */

#include "DsArc.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "src/xml/parsers/lv_xml_arc_parser.h"
#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The attributes ds_arc handles itself rather than lv_arc */
		constexpr std::array<std::string_view, 3> OWN_ATTRIBUTES = {"bind_value", "bind_min", "bind_max"};
	} // namespace

	void DsArc::registerXml()
	{
		Widgets::add(
			{.name = "ds_arc",
			 .extends = "lv_arc",
			 .description = "An arc that shows a value, such as a gauge's: a read-only lv_arc, without a knob, whose "
							"value and range follow subjects. The track is the main part's arc, the value the "
							"indicator's. Takes every lv_arc attribute but `bind_value`, which this replaces.",
			 .example =
				 R"(<ds_arc bind_value="presenter.temperature" bind_max="presenter.limit" bg_start_angle="135" bg_end_angle="45"
		width="120" height="120">
	<style name="gauge_arc"/>
	<style name="gauge_arc_indic" selector="indicator"/>
</ds_arc>)",
			 .attributes = {{"bind_value", "Follow an int subject for the value", AttributeKind::Subject},
							{"bind_min",
							 "Follow an int subject for the bottom of the range, e.g. a heater's minimum",
							 AttributeKind::Subject},
							{"bind_max",
							 "Follow an int subject for the top of the range, e.g. a heater's maximum",
							 AttributeKind::Subject}},
			 .exampleSubjects = {{"presenter.temperature", "int", "210"}, {"presenter.limit", "int", "300"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsArc::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		lv_obj_t* arc = lv_arc_create(parentOf(state));
		lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_remove_style(arc, nullptr, LV_PART_KNOB); // The theme's knob: a value shown has none
		return arc;
	}

	void DsArc::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		// lv_arc's attributes, without those handled here
		std::vector<const char*> arcAttrs;
		for (size_t i = 0; attrs[i] != nullptr; i += 2)
		{
			if (std::find(OWN_ATTRIBUTES.begin(), OWN_ATTRIBUTES.end(), std::string_view(attrs[i])) ==
				OWN_ATTRIBUTES.end())
			{
				arcAttrs.push_back(attrs[i]);
				arcAttrs.push_back(attrs[i + 1]);
			}
		}
		arcAttrs.push_back(nullptr);
		arcAttrs.push_back(nullptr);
		lv_xml_arc_apply(state, arcAttrs.data());

		lv_obj_t* arc = itemOf(state);
		// A new range clamps the value, so the range's observers show the bound value again (passed as their data)
		lv_subject_t* value = findSubject(state, attrs, "bind_value");
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_min"))
		{
			lv_subject_add_observer_obj(subject, minObserverCb, arc, value);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_max"))
		{
			lv_subject_add_observer_obj(subject, maxObserverCb, arc, value);
		}
		if (value != nullptr)
		{
			lv_subject_add_observer_obj(value, valueObserverCb, arc, nullptr);
		}
	}

	void DsArc::valueObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		lv_arc_set_value(static_cast<lv_obj_t*>(lv_observer_get_target(observer)), lv_subject_get_int(subject));
	}

	void DsArc::minObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* arc = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
		lv_arc_set_range(arc, lv_subject_get_int(subject), lv_arc_get_max_value(arc));
		showValue(arc, static_cast<lv_subject_t*>(lv_observer_get_user_data(observer)));
	}

	void DsArc::maxObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* arc = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
		lv_arc_set_range(arc, lv_arc_get_min_value(arc), lv_subject_get_int(subject));
		showValue(arc, static_cast<lv_subject_t*>(lv_observer_get_user_data(observer)));
	}

	void DsArc::showValue(lv_obj_t* arc, lv_subject_t* value)
	{
		if (value != nullptr)
		{
			lv_arc_set_value(arc, lv_subject_get_int(value));
		}
	}
} // namespace UI::Xml

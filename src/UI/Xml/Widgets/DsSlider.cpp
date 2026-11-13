/*
 * DsSlider.cpp
 */

#include "DsSlider.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/View/Actions.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "i18n/i18n.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>
#include <cstdlib>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsSlider::DsSlider(const std::string& name, lv_obj_t* parent)
		: Slider(name, parent)
	{
		setValueChangedCallback(
			[this](float value)
			{
				if (!m_settingQuietly)
				{
					lv_obj_send_event(getRootPtr(), Actions::valueConfirmedEvent(), &value);
				}
			});
	}

	void DsSlider::registerXml()
	{
		Widgets::add(
			{.name = "ds_slider",
			 .extends = "lv_obj",
			 .description = "A slider with a label, its value and -/+ buttons, and optionally a number pad. Raises "
							"`value_confirmed` with the value when the user finishes an edit (releases it, presses -/+ "
							"or types a value), or with `send=\"change\"` on every movement. Values written by "
							"`bind_value` are not edits and raise nothing, so a view can show live data and send "
							"changes without echoing the data back to the printer.",
			 .example =
				 R"(<ds_slider min="0" max="100" increment="1" decimals="0" label_tag="fan.speed" bind_value="requested" numberpad="true">
	<action trigger="value_confirmed" name="fans.set_speed"/>
</ds_slider>)",
			 .attributes = {{"min", "Bottom of the range, may have a fraction (default 0)"},
							{"max", "Top of the range, may have a fraction (default 100)"},
							{"increment", "Step of the -/+ buttons, may have a fraction, e.g. \"0.01\" (default 1)"},
							{"decimals", "Decimal places the value is shown with (default 2)"},
							{"label", "The label, if there is no `label_tag`"},
							{"label_tag", "Translation key for the label"},
							{"bind_label", "Follow a text subject for the label"},
							{"label_shown",
							 "\"false\" to hide the label, which still heads the number pad (default true)"},
							{"bind_value", "Follow an int subject for the value"},
							{"value_scale",
							 "How much `bind_value` is scaled by, for a value with a fraction, e.g. a chroma of 0.02 "
							 "held as 2 with value_scale=\"100\" (default 1)"},
							{"send",
							 "`confirmed` to raise `value_confirmed` when an edit is finished (default), or `change` "
							 "on every movement, for a setting seen as it changes such as brightness"},
							{"out_of_range",
							 "Let a typed value go past the slider's `upper` end, `lower` end or `both`, e.g. a "
							 "timeout longer than the slider reaches"},
							{"numberpad",
							 "\"true\" to type the value on the number pad (the view registered as `numberpad`) when "
							 "the value is tapped (default false)"}}},
			xmlCreate,
			xmlApply);
	}

	void DsSlider::setValueQuietly(float value)
	{
		m_settingQuietly = true;
		setValue(value);
		m_settingQuietly = false;
	}

	void DsSlider::setRangeQuietly(float min, float max)
	{
		m_settingQuietly = true;
		setRange(min, max);
		m_settingQuietly = false;
	}

	void* DsSlider::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* slider = new DsSlider(attrOr(attrs, "name", "slider"), parentOf(state));
		deleteWith(slider->getRootPtr(), slider);
		return slider->getRootPtr();
	}

	void DsSlider::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& slider = static_cast<DsSlider&>(*LvObj::fromPtr(root));

		const char* min = lv_xml_get_value_of(attrs, "min");
		const char* max = lv_xml_get_value_of(attrs, "max");
		if (min || max)
		{
			slider.setRangeQuietly(min ? std::strtof(min, nullptr) : slider.getMin(),
								   max ? std::strtof(max, nullptr) : slider.getMax());
		}
		if (const char* increment = lv_xml_get_value_of(attrs, "increment"))
		{
			slider.setIncrementValue(std::strtof(increment, nullptr));
		}
		if (const char* decimals = lv_xml_get_value_of(attrs, "decimals"))
		{
			slider.setDisplayDecimals(lv_xml_atoi(decimals));
		}
		if (const char* tag = lv_xml_get_value_of(attrs, "label_tag"))
		{
			slider.setLabel(_(tag));
		}
		else if (const char* label = lv_xml_get_value_of(attrs, "label"))
		{
			slider.setLabel(label);
		}
		if (const char* scale = lv_xml_get_value_of(attrs, "value_scale"))
		{
			slider.m_valueScale = std::max(1.0f, std::strtof(scale, nullptr));
		}
		if (!lv_xml_to_bool(attrOr(attrs, "label_shown", "true")))
		{
			slider.getLabel().hide(); // Still the number pad's header
		}
		if (lv_streq(attrOr(attrs, "send", "confirmed"), "change"))
		{
			slider.setSendMode(SendMode::VALUE_CHANGED);
		}
		if (const char* outOfRange = lv_xml_get_value_of(attrs, "out_of_range"))
		{
			if (lv_streq(outOfRange, "upper"))
			{
				slider.setOutOfRangeMode(OutOfRange::UPPER);
			}
			else if (lv_streq(outOfRange, "lower"))
			{
				slider.setOutOfRangeMode(OutOfRange::LOWER);
			}
			else if (lv_streq(outOfRange, "both"))
			{
				slider.setOutOfRangeMode(OutOfRange::BOTH);
			}
		}
		if (lv_xml_to_bool(attrOr(attrs, "numberpad", "false")))
		{
			slider.setNumberPad(findView<ModalNumberPad>("numberpad"));
		}

		if (lv_subject_t* subject = findSubject(state, attrs, "bind_label"))
		{
			lv_subject_add_observer_obj(subject, labelObserverCb, root, &slider);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_value"))
		{
			lv_subject_add_observer_obj(subject, valueObserverCb, root, &slider);
		}
	}

	void DsSlider::labelObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsSlider*>(lv_observer_get_user_data(observer))->setLabel(lv_subject_get_string(subject));
	}

	void DsSlider::valueObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* slider = static_cast<DsSlider*>(lv_observer_get_user_data(observer));
		// Don't fight the user while they are typing a value
		if (!slider->isFocused())
		{
			slider->setValueQuietly(static_cast<float>(lv_subject_get_int(subject)) / slider->m_valueScale);
		}
	}
} // namespace UI::Xml

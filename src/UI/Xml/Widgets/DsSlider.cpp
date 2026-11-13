/*
 * DsSlider.cpp
 */

#include "DsSlider.h"
#include "UI/Xml/Runtime/Services.h"
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
			 .description =
				 "A slider with a label, its value and -/+ buttons, and optionally a number pad. Raises "
				 "`value_confirmed` with the value when the user finishes an edit (releases it, presses -/+ "
				 "or types a value), or with `send=\"change\"` on every movement. Values written by "
				 "`bind_value` are not edits and raise nothing, so a view can show live data and send "
				 "changes without echoing the data back to the printer. Also raises `value_changed` as it is "
				 "dragged.",
			 .example =
				 R"(<ds_slider label_tag="settings.brightness" min="0" max="100" increment="1" decimals="0" bind_value="brightness"
		   numberpad="true" send="change" width="100%">
	<action trigger="value_confirmed" name="settings.brightness"/>
</ds_slider>)",
			 .attributes = {{"min", "Bottom of the range, may have a fraction (default 0)"},
							{"max", "Top of the range, may have a fraction (default 100)"},
							{"increment", "Step of the -/+ buttons, may have a fraction, e.g. \"0.01\" (default 1)"},
							{"decimals", "Decimal places the value is shown with (default 2)"},
							{"label", "The label, if there is no `label_tag`"},
							{"label_tag", "Translation key for the label", AttributeKind::Translation},
							{"bind_label", "Follow a text subject for the label", AttributeKind::Subject},
							{"label_shown",
							 "\"false\" to hide the label, which still heads the number pad (default true)"},
							{"bind_value", "Follow an int subject for the value", AttributeKind::Subject},
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
							 "What tapping the value does: `true` types it on the shared number pad, `own` raises "
							 "`value_tapped` with the value for the view to show its own, `false` types it in place "
							 "(default)"},
							{"default",
							 "A value to go back to: a Reset button shows while the value is anything else, and "
							 "raises `value_confirmed` with it"},
							{"bind_min",
							 "Follow an int subject for the bottom of the range (scaled by `value_scale`)",
							 AttributeKind::Subject},
							{"bind_max",
							 "Follow an int subject for the top of the range (scaled by `value_scale`)",
							 AttributeKind::Subject}},
			 .exampleSubjects = {{"brightness", "int", "60"}}},
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

	bool DsSlider::askForValue()
	{
		switch (m_numberPad)
		{
		case NumberPad::NONE:
			return false;
		case NumberPad::OWN:
		{
			float value = getValue();
			lv_obj_send_event(getRootPtr(), Actions::valueTappedEvent(), &value);
			return true;
		}
		case NumberPad::SHARED:
			break;
		}
		// The request goes with the slider: a fan going while its number pad is open takes the slider with it
		return Services::askNumber({.header = std::string(getLabelText()),
									.min = lowestTypable(),
									.max = highestTypable(),
									.value = getValue(),
									.onConfirm = [this](float value) { setValue(value); },
									.asker = getRootPtr()});
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
		// Applied again for each placement of a component extending it, so only what is written changes anything
		if (const char* numberPad = lv_xml_get_value_of(attrs, "numberpad"))
		{
			slider.m_numberPad = lv_streq(numberPad, "own")	 ? NumberPad::OWN
								 : lv_xml_to_bool(numberPad) ? NumberPad::SHARED
															 : NumberPad::NONE;
		}
		if (const char* value = lv_xml_get_value_of(attrs, "default"))
		{
			slider.setDefaultValue(std::strtof(value, nullptr));
		}

		if (lv_subject_t* subject = findSubject(state, attrs, "bind_label"))
		{
			lv_subject_add_observer_obj(subject, labelObserverCb, root, &slider);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_min"))
		{
			lv_subject_add_observer_obj(subject, minObserverCb, root, &slider);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_max"))
		{
			lv_subject_add_observer_obj(subject, maxObserverCb, root, &slider);
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

	void DsSlider::minObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* slider = static_cast<DsSlider*>(lv_observer_get_user_data(observer));
		slider->setRangeQuietly(static_cast<float>(lv_subject_get_int(subject)) / slider->m_valueScale,
								slider->getMax());
	}

	void DsSlider::maxObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* slider = static_cast<DsSlider*>(lv_observer_get_user_data(observer));
		slider->setRangeQuietly(slider->getMin(),
								static_cast<float>(lv_subject_get_int(subject)) / slider->m_valueScale);
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

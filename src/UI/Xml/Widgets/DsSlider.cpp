/*
 * DsSlider.cpp
 */

#include "DsSlider.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/View/Actions.h"
#include "WidgetSupport.h"
#include "i18n/i18n.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

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
		lv_xml_register_widget("ds_slider", xmlCreate, xmlApply);
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
			slider.setRangeQuietly(min ? static_cast<float>(lv_xml_atoi(min)) : slider.getMin(),
								   max ? static_cast<float>(lv_xml_atoi(max)) : slider.getMax());
		}
		if (const char* increment = lv_xml_get_value_of(attrs, "increment"))
		{
			slider.setIncrementValue(static_cast<float>(lv_xml_atoi(increment)));
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
			slider->setValueQuietly(static_cast<float>(lv_subject_get_int(subject)));
		}
	}
} // namespace UI::Xml

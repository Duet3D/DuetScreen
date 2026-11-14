/*
 * ValueSelector.cpp
 */

#include "ValueSelector.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		std::vector<ValueSelector*> s_live; // Every selector, to be found by id

		/** Where the presets are kept, under the selector's storage key */
		StorageKeyRunTime<std::vector<float>> presetsKey(const std::string& storageKey)
		{
			return StorageKeyRunTime<std::vector<float>>(storageKey + ":values", {});
		}

		/** Where the value is kept, under the selector's storage key */
		StorageKeyRunTime<float> valueKey(const std::string& storageKey)
		{
			return StorageKeyRunTime<float>(storageKey + ":selected", std::numeric_limits<float>::quiet_NaN());
		}

		/** A value as shown, e.g. "0.1" */
		std::string format(float value)
		{
			return fmt::format("{:g}", value);
		}
	} // namespace

	void ValueChoiceRow::fields(FieldVisitor& f)
	{
		f.field("text", text, "The preset, e.g. \"10\"");
		f.field("selected", selected, "1 if it is the value chosen, else 0");
	}

	ValueSelector::ValueSelector(Settings settings)
		: m_settings(std::move(settings))
		, m_presets(m_settings.presets)
	{
		if (auto stored = StorageHelper::getData(presetsKey(m_settings.storageKey)); !stored.empty())
		{
			m_presets = std::move(stored);
		}
		float value =
			m_presets.empty() ? m_settings.min : m_presets[std::min(m_settings.initial, m_presets.size() - 1)];
		if (m_settings.keepValue)
		{
			if (const float stored = StorageHelper::getData(valueKey(m_settings.storageKey)); !std::isnan(stored))
			{
				value = stored;
			}
		}
		m_value = std::clamp(value, m_settings.min, m_settings.max);
		m_valueText.set(format(m_value));
		showPresets();
		s_live.push_back(this);
	}

	ValueSelector::~ValueSelector()
	{
		std::erase(s_live, this);
	}

	ValueSelector* ValueSelector::find(std::string_view id)
	{
		const auto it =
			std::find_if(s_live.begin(), s_live.end(), [id](const ValueSelector* s) { return s->id() == id; });
		return it == s_live.end() ? nullptr : *it;
	}

	void ValueSelector::fields(FieldVisitor& f)
	{
		const std::string& id = m_settings.id;
		const std::string name = id.substr(id.find('.') + 1);
		f.field(name + ".title", m_title, "The title, e.g. \"Feed Distance (mm)\"");
		f.field(name + ".value", m_valueText, "The value, e.g. \"0.1\" (actions.selector.* with arg `" + id + "`)");
		f.field(name + ".unit", m_unitText, "The value's unit, e.g. \"mm\"; empty if it has none");
		f.list(name + ".choices",
			   m_choices,
			   "The presets, each with `text` and `selected` (actions.selector.pick, actions.selector.edit_choice)");
	}

	void ValueSelector::setTitle(std::string_view title)
	{
		m_title.set(title);
	}

	void ValueSelector::setUnit(std::string_view unit)
	{
		m_unit = unit;
		m_unitText.set(unit);
	}

	void ValueSelector::setValue(float value)
	{
		m_value = std::clamp(value, m_settings.min, m_settings.max);
		m_valueText.set(format(m_value));
		showPresets();
		if (m_settings.keepValue)
		{
			StorageHelper::setData(valueKey(m_settings.storageKey), m_value);
		}
	}

	void ValueSelector::pick(size_t index)
	{
		if (index < m_presets.size())
		{
			setValue(m_presets[index]);
		}
	}

	void ValueSelector::step(int direction)
	{
		setValue(m_value + static_cast<float>(direction) * m_settings.step);
	}

	void ValueSelector::edit()
	{
		Prompts::askNumber({.header = _("multi_value_selector.numberpad_input_header"),
							.min = m_settings.min,
							.max = m_settings.max,
							.value = m_value,
							.unit = m_unit,
							.onConfirm = [this](float value) { setValue(value); }});
	}

	void ValueSelector::editPreset(size_t index)
	{
		if (index >= m_presets.size())
		{
			return;
		}
		Prompts::askNumber({.header = _("multi_value_selector.numberpad_new_value_header"),
							.min = m_settings.min,
							.max = m_settings.max,
							.value = m_presets[index],
							.unit = m_unit,
							.onConfirm = [this, index](float value)
							{
								setPreset(index, value);
								setValue(value);
							}});
	}

	void ValueSelector::setPreset(size_t index, float value)
	{
		if (index >= m_presets.size())
		{
			LOG_ERROR("{:s} has no preset {:d}", m_settings.id, index);
			return;
		}
		m_presets[index] = std::clamp(value, m_settings.min, m_settings.max);
		showPresets();
		StorageHelper::setData(presetsKey(m_settings.storageKey), m_presets);
	}

	void ValueSelector::showPresets()
	{
		m_choices.sync(m_presets.size(),
					   [this](ValueChoiceRow& row, size_t i)
					   {
						   row.owner = this;
						   row.text.set(format(m_presets[i]));
						   row.selected.set(m_presets[i] == m_value ? 1 : 0);
					   });
	}
} // namespace UI::Xml

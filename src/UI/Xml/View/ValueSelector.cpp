/*
 * ValueSelector.cpp
 */

#include "ValueSelector.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
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
	}

	ValueSelector::ValueSelector(Settings settings)
		: m_settings(std::move(settings))
		, m_presets(m_settings.presets)
		, m_value(m_settings.min)
	{
		if (!m_settings.storageKey.empty())
		{
			if (auto stored = StorageHelper::getData(presetsKey(m_settings.storageKey)); !stored.empty())
			{
				m_presets = std::move(stored);
			}
			if (const float stored = StorageHelper::getData(valueKey(m_settings.storageKey)); !std::isnan(stored))
			{
				m_value = std::clamp(stored, m_settings.min, m_settings.max);
			}
		}
		m_valueText.set(format(m_value));
		showPresets();
	}

	void ValueSelector::fields(FieldVisitor& f)
	{
		const std::string& id = m_settings.id;
		f.field(id + ".title", m_title, "The title, e.g. \"Feed Distance (mm)\"");
		f.field(id + ".value", m_valueText, "The value, e.g. \"0.1\"");
		f.list(id + ".choices", m_choices, "The presets, each with `text`");
	}

	void ValueSelector::setTitle(std::string_view title)
	{
		m_title.set(title);
	}

	void ValueSelector::setValue(float value)
	{
		m_value = std::clamp(value, m_settings.min, m_settings.max);
		m_valueText.set(format(m_value));
		if (!m_settings.storageKey.empty())
		{
			StorageHelper::setData(valueKey(m_settings.storageKey), m_value);
		}
	}

	void ValueSelector::setPreset(size_t index, float value)
	{
		if (index >= m_presets.size())
		{
			LOG_ERROR("{:s} has no preset {:d}", m_settings.id, index);
			return;
		}
		m_presets[index] = value;
		showPresets();
		if (!m_settings.storageKey.empty())
		{
			StorageHelper::setData(presetsKey(m_settings.storageKey), m_presets);
		}
	}

	void ValueSelector::showPresets()
	{
		m_choices.sync(m_presets.size(),
					   [this](ValueChoiceRow& row, size_t i)
					   {
						   row.owner = this;
						   row.text.set(format(m_presets[i]));
					   });
	}

	void ValueSelector::ask(std::string header, float start, std::function<void(float)> onConfirm)
	{
		Services::askNumber({.header = std::move(header),
							 .min = m_settings.min,
							 .max = m_settings.max,
							 .value = start,
							 .onConfirm = std::move(onConfirm)});
	}

	std::vector<ValueSelector::Action> ValueSelector::actions(std::initializer_list<ValueSelector*> selectors)
	{
		const std::vector<ValueSelector*> all(selectors);

		// The selector an action is about: its choices row's, else the one its arg names
		auto selectorOf = [all](const ActionArgs& args) -> ValueSelector*
		{
			if (const auto* row = args.row<ValueChoiceRow>())
			{
				return row->owner;
			}
			const auto it = std::find_if(
				all.begin(), all.end(), [&args](const ValueSelector* s) { return s->m_settings.id == args.arg; });
			if (it == all.end())
			{
				LOG_WARN("{:s}: no selector called '{:s}'", args.name, args.arg);
				return nullptr;
			}
			return *it;
		};

		return {
			{"selector.decrement",
			 "Step the selector named by `arg` (its id) down",
			 [selectorOf](const ActionArgs& args)
			 {
				 if (auto* s = selectorOf(args))
				 {
					 s->setValue(s->m_value - s->m_settings.step);
				 }
			 }},
			{"selector.increment",
			 "Step the selector named by `arg` (its id) up",
			 [selectorOf](const ActionArgs& args)
			 {
				 if (auto* s = selectorOf(args))
				 {
					 s->setValue(s->m_value + s->m_settings.step);
				 }
			 }},
			{"selector.edit",
			 "Ask for a value for the selector named by `arg` (its id) with a number pad",
			 [selectorOf](const ActionArgs& args)
			 {
				 if (auto* s = selectorOf(args))
				 {
					 s->ask(_("multi_value_selector.numberpad_input_header"),
							s->m_value,
							[s](float value) { s->setValue(value); });
				 }
			 }},
			{"selector.pick",
			 "Use the preset. From a row of a selector's `choices`.",
			 [](const ActionArgs& args)
			 {
				 const auto* row = args.row<ValueChoiceRow>();
				 if (row == nullptr || row->owner == nullptr)
				 {
					 LOG_WARN("selector.pick must come from a choices row");
					 return;
				 }
				 ValueSelector& s = *row->owner;
				 const auto index = static_cast<size_t>(row->index.get());
				 if (index < s.m_presets.size())
				 {
					 s.setValue(s.m_presets[index]);
				 }
			 }},
			{"selector.edit_choice",
			 "Ask for a new value for the preset with a number pad, and use it. From a row of a selector's `choices`.",
			 [](const ActionArgs& args)
			 {
				 const auto* row = args.row<ValueChoiceRow>();
				 if (row == nullptr || row->owner == nullptr)
				 {
					 LOG_WARN("selector.edit_choice must come from a choices row");
					 return;
				 }
				 ValueSelector* s = row->owner;
				 const auto index = static_cast<size_t>(row->index.get());
				 if (index >= s->m_presets.size())
				 {
					 return;
				 }
				 s->ask(_("multi_value_selector.numberpad_new_value_header"),
						s->m_presets[index],
						[s, index](float value)
						{
							s->setPreset(index, value);
							s->setValue(value);
						});
			 }},
		};
	}
} // namespace UI::Xml

/*
 * FactorDialog.cpp
 */

#include "FactorDialog.h"
#include "Debug.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	void FactorRow::fields(Xml::FieldVisitor& f)
	{
		f.field("label", label, "What it is, e.g. \"Extruder 0\"");
		f.field("value", value, "The percentage, e.g. 100");
		f.field("min", min, "The bottom of its slider");
		f.field("max", max, "The top of its slider");
	}

	FactorDialog::FactorDialog(Settings settings)
		: m_settings(std::move(settings))
		, m_title(m_settings.title)
	{
	}

	void FactorDialog::fields(Xml::FieldVisitor& f)
	{
		const std::string& id = m_settings.id;
		f.field(id + ".open", m_open, "1 while the dialog is open");
		f.field(id + ".title", m_title, "Above the sliders, e.g. \"Speed Factor\"");
		f.list(id + ".factors", m_factors, "The percentages, each with `label`, `value`, `min` and `max`");
	}

	void FactorDialog::show(size_t count,
							const std::function<std::string(size_t)>& labelOf,
							const std::function<float(size_t)>& valueOf)
	{
		m_factors.sync(count,
					   [&](FactorRow& row, size_t i)
					   {
						   row.owner = this;
						   row.label.set(labelOf(i));
						   setRow(row, static_cast<int32_t>(std::lround(valueOf(i))));
					   });
	}

	void FactorDialog::open()
	{
		m_open.set(1);
	}

	void FactorDialog::close()
	{
		m_open.set(0);
	}

	void FactorDialog::setRow(FactorRow& row, int32_t value) const
	{
		row.value.set(value);
		if (m_settings.rangeFollowsValue)
		{
			row.min.set(std::max(m_settings.sliderMin, value - 50));
			row.max.set(std::max(m_settings.sliderMax, value + 50));
		}
		else
		{
			row.min.set(m_settings.sliderMin);
			row.max.set(m_settings.sliderMax);
		}
	}

	void FactorDialog::set(size_t index, float value)
	{
		if (index >= m_factors.size())
		{
			return;
		}
		setRow(m_factors[index], static_cast<int32_t>(std::lround(value)));
		if (m_settings.onSet)
		{
			m_settings.onSet(index, value);
		}
	}

	std::vector<FactorDialog::Action> FactorDialog::actions(std::initializer_list<FactorDialog*> dialogs)
	{
		const std::vector<FactorDialog*> all(dialogs);

		// The dialog an action is about: its factor row's, else the one its arg names
		auto dialogOf = [all](const Xml::ActionArgs& args) -> FactorDialog*
		{
			if (const auto* row = args.row<FactorRow>())
			{
				return row->owner;
			}
			const auto it = std::find_if(
				all.begin(), all.end(), [&args](const FactorDialog* d) { return d->m_settings.id == args.arg; });
			if (it == all.end())
			{
				LOG_WARN("{:s}: no factor dialog called '{:s}'", args.name, args.arg);
				return nullptr;
			}
			return *it;
		};
		// The index of the factor row an action came from, if it came from one
		auto indexOf = [](const Xml::ActionArgs& args) -> std::optional<size_t>
		{
			const auto* row = args.row<FactorRow>();
			if (row == nullptr)
			{
				LOG_WARN("{:s} must come from a row of a factor dialog's `factors`", args.name);
				return std::nullopt;
			}
			return static_cast<size_t>(row->index.get());
		};

		return {
			{"factor.set",
			 "Use the slider's edited percentage (`value_confirmed`, dragged or typed) for its factor. From a row of "
			 "`factors`.",
			 [dialogOf, indexOf](const Xml::ActionArgs& args)
			 {
				 auto* d = dialogOf(args);
				 const auto index = indexOf(args);
				 const auto value = args.value();
				 if (d != nullptr && index && value)
				 {
					 d->set(*index, *value);
				 }
			 }},
			{"factor.close",
			 "Close the dialog named by `arg` (its id), e.g. when it is dismissed",
			 [dialogOf](const Xml::ActionArgs& args)
			 {
				 if (auto* d = dialogOf(args))
				 {
					 d->close();
				 }
			 }},
		};
	}
} // namespace UI

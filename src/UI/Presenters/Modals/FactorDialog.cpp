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
		, m_pad(m_settings.id + ".pad")
	{
		// Typing a value uses it and is done with the dialog
		m_pad.setOnConfirmed([this]() { close(); });
	}

	void FactorDialog::fields(Xml::FieldVisitor& f)
	{
		const std::string& id = m_settings.id;
		f.field(id + ".open", m_open, "1 while the dialog is open");
		f.field(id + ".title", m_title, "Above the sliders, e.g. \"Speed Factor\"");
		f.list(id + ".factors", m_factors, "The percentages, each with `label`, `value`, `min` and `max`");
		f.field(id + ".pad_shown", m_padShown, "1 while the number pad is shown, for the factor being typed");
		m_pad.fields(f);
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
		if (m_editing && *m_editing >= count)
		{
			m_editing.reset();
			m_padShown.set(0);
			m_pad.drop();
		}
	}

	void FactorDialog::open()
	{
		m_editing.reset();
		m_padShown.set(0);
		m_pad.drop();
		if (m_settings.padOnOpen && m_factors.size() > 0)
		{
			edit(0);
		}
		m_open.set(1);
	}

	void FactorDialog::close()
	{
		m_open.set(0);
		m_editing.reset();
		m_padShown.set(0);
		m_pad.drop();
	}

	void FactorDialog::edit(size_t index)
	{
		if (index >= m_factors.size())
		{
			return;
		}
		FactorRow& row = m_factors[index];
		m_editing = index;
		m_pad.ask(Xml::NumberRequest{.header = std::string(row.label.get()),
									 .min = m_settings.min,
									 .max = m_settings.max,
									 .value = static_cast<float>(row.value.get()),
									 .onConfirm = [this, index](float value) { set(index, value); }});
		m_padShown.set(1);
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

		std::vector<Action> actions{
			{"factor.set",
			 "Use the slider's edited percentage (`value_confirmed`) for its factor. From a row of `factors`.",
			 [dialogOf, indexOf](const Xml::ActionArgs& args)
			 {
				 auto* d = dialogOf(args);
				 const auto index = indexOf(args);
				 const auto value = args.value();
				 if (d != nullptr && index && value)
				 {
					 d->set(*index, *value);
					 if (d->m_editing == index)
					 {
						 d->m_pad.showValue(*value);
					 }
				 }
			 }},
			{"factor.dragged",
			 "The slider moved (`value_changed`): the number pad follows it if it is for that factor. From a row of "
			 "`factors`.",
			 [dialogOf, indexOf](const Xml::ActionArgs& args)
			 {
				 auto* d = dialogOf(args);
				 const auto index = indexOf(args);
				 const auto value = args.value();
				 if (d != nullptr && index && value && d->m_editing == index)
				 {
					 d->m_pad.showValue(*value);
				 }
			 }},
			{"factor.edit",
			 "Type a percentage for the factor on the dialog's number pad (`value_tapped`). From a row of `factors`.",
			 [dialogOf, indexOf](const Xml::ActionArgs& args)
			 {
				 auto* d = dialogOf(args);
				 if (const auto index = indexOf(args); d != nullptr && index)
				 {
					 d->edit(*index);
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

		// Their number pads' actions, which name the pad (`<id>.pad`) as their arg
		std::vector<Xml::NumberEntry*> pads;
		for (FactorDialog* d : all)
		{
			pads.push_back(&d->m_pad);
		}
		for (auto& action : Xml::NumberEntry::actions(pads))
		{
			actions.push_back(std::move(action));
		}
		return actions;
	}
} // namespace UI

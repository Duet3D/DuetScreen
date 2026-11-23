/*
 * FactorDialog.cpp
 */

#include "FactorDialog.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI::Xml
{
	void FactorPresetRow::fields(FieldVisitor& f)
	{
		f.field("text", text, "The quick value, e.g. \"100%\" (actions.factors.preset)");
		f.field("chosen", chosen, "1 if it is the factor's value");
	}

	void FactorRow::fields(FieldVisitor& f)
	{
		f.field("label", label, "What it is, e.g. \"Extruder 0\"; empty if the dialog's title says it");
		f.field("detail", detail, "More about it, e.g. the tools that use the extruder; empty for nothing");
		f.field("value", value, "The percentage, e.g. 100");
		f.field("text", text, "The percentage as shown, e.g. \"100%\"");
		f.field("min", min, "The bottom of its slider");
		f.field("max", max, "The top of its slider");
		f.list("presets", presets, "Quick values, each with `text` and `chosen` (actions.factors.preset)");
	}

	FactorDialog::FactorDialog(Settings settings)
		: m_settings(std::move(settings))
		, m_title(m_settings.title)
	{
	}

	void FactorDialog::fields(FieldVisitor& f)
	{
		const std::string& id = m_settings.id;
		f.field(id + ".open", m_open, "1 while the dialog is open");
		f.field(id + ".title", m_title, "Above the sliders, e.g. \"Speed Factor\"");
		f.list(id + ".factors",
			   m_factors,
			   "The percentages, each with `label`, `value`, `min` and `max` (actions.factors.set)");
	}

	void FactorDialog::show(size_t count,
							const std::function<std::string(size_t)>& labelOf,
							const std::function<float(size_t)>& valueOf,
							const std::function<std::string(size_t)>& detailOf)
	{
		m_factors.sync(count,
					   [&](FactorRow& row, size_t i)
					   {
						   row.owner = this;
						   row.label.set(labelOf(i));
						   row.detail.set(detailOf ? detailOf(i) : std::string());
						   row.presets.sync(m_settings.presets.size(),
											[this, &row](FactorPresetRow& preset, size_t p)
											{
												preset.factor = &row;
												preset.percent = m_settings.presets[p];
												preset.text.set(fmt::format("{:d}%", preset.percent));
											});
						   setRow(row, static_cast<int32_t>(std::lround(valueOf(i))));
					   });
	}

	void FactorDialog::setTitle(std::string_view title)
	{
		m_title.set(title);
	}

	void FactorDialog::open()
	{
		m_open.set(1);
	}

	void FactorDialog::close()
	{
		m_open.set(0);
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

	void FactorDialog::setRow(FactorRow& row, int32_t value) const
	{
		row.value.set(value);
		row.text.set(fmt::format("{:d}%", value));
		for (size_t p = 0; p < row.presets.size(); ++p)
		{
			row.presets[p].chosen.set(row.presets[p].percent == value ? 1 : 0);
		}
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
} // namespace UI::Xml

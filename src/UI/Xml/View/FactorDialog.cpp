/*
 * FactorDialog.cpp
 */

#include "FactorDialog.h"
#include <algorithm>
#include <cmath>

namespace UI::Xml
{
	void FactorRow::fields(FieldVisitor& f)
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

/*
 * Surface.cpp
 */

#include "Surface.h"
#include <cmath>
#include <limits>

namespace UI::Xml
{
	Surface::Surface()
	{
		m_subject.initPointer(this);
	}

	void Surface::set(Values values)
	{
		m_values = std::move(values);
		m_subject.notify();
	}

	void Surface::clear()
	{
		set(Values{});
	}

	float Surface::sample(size_t i, size_t j) const
	{
		if (i >= m_values.columns || j >= m_values.rows || j * m_values.columns + i >= m_values.samples.size())
		{
			return std::numeric_limits<float>::quiet_NaN();
		}
		return m_values.samples[j * m_values.columns + i];
	}
} // namespace UI::Xml

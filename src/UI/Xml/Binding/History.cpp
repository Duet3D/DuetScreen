/*
 * History.cpp
 */

#include "History.h"
#include "Debug.h"
#include <algorithm>

namespace UI::Xml
{
	void SeriesRow::fields(FieldVisitor& f)
	{
		f.field("name", name, "Name of the series, e.g. for a legend");
	}

	History::History(uint32_t samplesPerSecond, size_t capacity)
		: m_samplesPerSecond(samplesPerSecond)
		, m_capacity(capacity)
	{
	}

	void History::addSampleListener(SampleListener* listener)
	{
		if (std::find(m_sampleListeners.begin(), m_sampleListeners.end(), listener) == m_sampleListeners.end())
		{
			m_sampleListeners.push_back(listener);
		}
	}

	void History::removeSampleListener(SampleListener* listener)
	{
		std::erase(m_sampleListeners, listener);
	}

	void History::notifySampled()
	{
		ZoneScoped;
		// Copy: a listener may remove itself
		const auto listeners = m_sampleListeners;
		for (SampleListener* listener : listeners)
		{
			if (std::find(m_sampleListeners.begin(), m_sampleListeners.end(), listener) != m_sampleListeners.end())
			{
				listener->onSampled();
			}
		}
	}
} // namespace UI::Xml

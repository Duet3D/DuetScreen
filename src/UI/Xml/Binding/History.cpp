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
		f.field("key",
				key,
				"What its source knows the series by, so a `<ds_graph_key>` can find it: for "
				"source.temperature.temperatures, its heaters' sensor number");
		f.field("shown",
				shown,
				"1 while graphs draw the series, 0 while it is hidden; tapping its legend entry or a `<ds_graph_key>` "
				"for it toggles it, in every graph of the history");
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

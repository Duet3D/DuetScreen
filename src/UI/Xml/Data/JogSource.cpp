/*
 * JogSource.cpp
 */

#include "JogSource.h"
#include "DataSources.h"
#include "Storage.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"

namespace UI::Xml
{
	JogSource::JogSource()
		: DataSource("jog")
		, m_distance({.id = "jog.distance",
					  .storageKey = std::string(ID_MVS_JOG_DISTANCES),
					  .presets = {0.1f, 0.5f, 1, 5, 10, 25, 50},
					  .initial = 4,
					  .min = 0.01f,
					  .max = 1000})
		, m_feedrate({.id = "jog.feedrate",
					  .storageKey = std::string(ID_MVS_JOG_FEEDRATES),
					  .presets = {5, 10, 25, 50, 100, 200, 300},
					  .initial = 3,
					  .min = 1,
					  .max = 1000})
	{
	}

	void JogSource::fields(FieldVisitor& f)
	{
		m_distance.fields(f);
		m_feedrate.fields(f);
	}

	void JogSource::update()
	{
		m_distance.setTitle(_("move.distance", Units::getDisplayedDistanceUnit()));
		m_distance.setUnit(Units::getDisplayedDistanceUnit());
		m_feedrate.setTitle(_("move.feedrate", Units::getDisplayedSpeedUnit()));
		m_feedrate.setUnit(Units::getDisplayedSpeedUnit());
	}

	std::optional<float> JogSource::distance()
	{
		const auto* source = DataSources::find<JogSource>();
		return source != nullptr ? std::optional(source->m_distance.value()) : std::nullopt;
	}

	std::optional<float> JogSource::feedrate()
	{
		const auto* source = DataSources::find<JogSource>();
		return source != nullptr ? std::optional(source->m_feedrate.value()) : std::nullopt;
	}
} // namespace UI::Xml

/*
 * JogSource.cpp
 */

#include "JogSource.h"
#include "DataSources.h"
#include "PrinterModel.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"

namespace UI::Xml
{
	namespace
	{
		/** The jog distances, in millimetres or in inches */
		ValueSelector::Presets distances(bool metric)
		{
			const std::string key(ID_MVS_JOG_DISTANCES);
			return metric ? ValueSelector::Presets{.storageKey = key,
												   .values = {0.1f, 0.5f, 1, 5, 10, 25, 50},
												   .initial = 4,
												   .min = 0.01f,
												   .max = 1000}
						  : ValueSelector::Presets{.storageKey = key + "_in",
												   .values = {0.001f, 0.01f, 0.05f, 0.1f, 0.5f, 1, 2},
												   .initial = 4,
												   .min = 0.001f,
												   .max = 40};
		}

		/** The jog speeds: a printer's in mm/s, a CNC machine's in mm/min, or either's in inches per minute */
		ValueSelector::Presets feedrates(bool metric, bool cnc)
		{
			const std::string key(ID_MVS_JOG_FEEDRATES);
			if (cnc)
			{
				return metric ? ValueSelector::Presets{.storageKey = key + "_cnc",
													   .values = {100, 250, 500, 1000, 2000, 3000, 5000},
													   .initial = 3,
													   .min = 1,
													   .max = 60000}
							  : ValueSelector::Presets{.storageKey = key + "_cnc_in",
													   .values = {5, 10, 20, 40, 80, 120, 200},
													   .initial = 3,
													   .min = 0.1f,
													   .max = 2400};
			}
			return metric ? ValueSelector::Presets{.storageKey = key,
												   .values = {5, 10, 25, 50, 100, 200, 300},
												   .initial = 3,
												   .min = 1,
												   .max = 1000}
						  : ValueSelector::Presets{.storageKey = key + "_in",
												   .values = {10, 25, 50, 100, 250, 500, 750},
												   .initial = 3,
												   .min = 1,
												   .max = 2400};
		}
	} // namespace

	JogSource::JogSource()
		: DataSource("jog")
		, m_distance({.id = "jog.distance", .presets = distances(true)})
		, m_feedrate({.id = "jog.feedrate", .presets = feedrates(true, false)})
	{
	}

	void JogSource::fields(FieldVisitor& f)
	{
		m_distance.fields(f);
		m_feedrate.fields(f);
	}

	void JogSource::onInit()
	{
		watchObjectModel({"state.machineMode"});
	}

	void JogSource::update()
	{
		const bool metric = Units::getCurrentDisplayedUnitSystem() == Units::UnitSystem::Metric;
		bool cnc = false;
		{
			MODEL_LOCK();
			cnc = PrinterModel::root()["state"]["machineMode"].text() == "CNC";
		}
		m_perMinute = cnc && metric;
		const std::string speedUnit = m_perMinute ? _("units.mm_per_minute") : Units::getDisplayedSpeedUnit();
		m_distance.use(distances(metric));
		m_distance.setTitle(_("move.distance", Units::getDisplayedDistanceUnit()));
		m_distance.setUnit(Units::getDisplayedDistanceUnit());
		m_feedrate.use(feedrates(metric, cnc));
		m_feedrate.setTitle(_("move.feedrate", speedUnit));
		m_feedrate.setUnit(speedUnit);
	}

	std::optional<float> JogSource::distance()
	{
		const auto* source = DataSources::find<JogSource>();
		return source != nullptr ? std::optional(source->m_distance.value()) : std::nullopt;
	}

	std::optional<float> JogSource::feedrateGcode()
	{
		const auto* source = DataSources::find<JogSource>();
		if (source == nullptr)
		{
			return std::nullopt;
		}
		// Per minute already, the speed only needs the Duet's distance unit; mm/s is made per minute too
		const float speed = source->m_feedrate.value();
		return source->m_perMinute ? Units::convertDisplayedDistanceToDuetUnits(speed)
								   : Units::convertDisplayedSpeedToDuetUnits(speed);
	}
} // namespace UI::Xml

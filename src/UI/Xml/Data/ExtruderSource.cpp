/*
 * ExtruderSource.cpp
 */

#include "ExtruderSource.h"
#include "Debug.h"
#include "ObjectModel/PrinterStatus.h"
#include "PrinterModel.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** How much filament to feed, in millimetres or in inches */
		ValueSelector::Presets distances(bool metric)
		{
			const std::string key(ID_MVS_EXTRUSION_DISTANCES);
			return metric ? ValueSelector::Presets{.storageKey = key, .values = {1.0f, 2.0f, 5.0f, 10.0f}, .min = 0.1f}
						  : ValueSelector::Presets{
								.storageKey = key + "_in", .values = {0.05f, 0.1f, 0.2f, 0.5f}, .min = 0.005f};
		}

		/** How fast to feed it, in mm/s or in inches per minute */
		ValueSelector::Presets feedrates(bool metric)
		{
			const std::string key(ID_MVS_EXTRUSION_FEEDRATES);
			return metric ? ValueSelector::Presets{.storageKey = key, .values = {1.0f, 5.0f, 20.0f, 50.0f}, .min = 0.1f}
						  : ValueSelector::Presets{
								.storageKey = key + "_in", .values = {2.0f, 10.0f, 50.0f, 120.0f}, .min = 0.2f};
		}
	} // namespace

	ExtruderSource::ExtruderSource()
		: DataSource("extruder")
		, m_distance({.id = "extruder.distance", .presets = distances(true), .keepValue = true})
		, m_feedrate({.id = "extruder.feedrate", .presets = feedrates(true), .keepValue = true})
	{
	}

	void ExtruderSource::fields(FieldVisitor& f)
	{
		f.field("extrude_usable",
				m_extrudeUsable,
				"1 if the current tool can extrude: there is one, it is hot enough (or cold extrusion is allowed), and "
				"no job is running");
		f.field("retract_usable",
				m_retractUsable,
				"1 if the current tool can retract: there is one, it is hot enough (or cold extrusion is allowed), and "
				"no job is running");
		m_distance.fields(f);
		m_feedrate.fields(f);
	}

	void ExtruderSource::onInit()
	{
		watchObjectModel({"tools", "heat", "state.currentTool"});
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { update(); });
	}

	void ExtruderSource::update()
	{
		ZoneScoped;
		const bool metric = Units::getCurrentDisplayedUnitSystem() == Units::UnitSystem::Metric;
		m_distance.use(distances(metric));
		m_feedrate.use(feedrates(metric));
		m_distance.setTitle(_("extrude.feed_dist", Units::getDisplayedDistanceUnit()));
		m_distance.setUnit(Units::getDisplayedDistanceUnit());
		m_feedrate.setTitle(_("extrude.feed_rate", Units::getDisplayedSpeedUnit()));
		m_feedrate.setUnit(Units::getDisplayedSpeedUnit());

		MODEL_LOCK();
		const auto tool = PrinterModel::currentTool();
		const bool jobRunning = OM::PrintInProgress() && OM::GetStatus() != OM::PrinterStatus::paused;
		bool canExtrude = tool.has_value() && !jobRunning;
		bool canRetract = canExtrude;
		if (tool)
		{
			// Every heater of the tool must be hot enough. A limit of 0 is how the Duet reports cold extrusion
			// allowed (M302 P1): it then checks no temperature at all, so neither does this, even a heater whose
			// sensor reads nothing (-273°C).
			const OM::Node& heat = PrinterModel::root()["heat"];
			const double coldExtrude = heat["coldExtrudeTemperature"].number();
			const double coldRetract = heat["coldRetractTemperature"].number();
			for (size_t slot = 0; slot < PrinterModel::tool(*tool)["heaters"].size(); ++slot)
			{
				if (const auto heater = PrinterModel::toolHeater(*tool, slot))
				{
					const double current = PrinterModel::heater(*heater)["current"].number();
					canExtrude = canExtrude && (coldExtrude <= 0.0 || current >= coldExtrude);
					canRetract = canRetract && (coldRetract <= 0.0 || current >= coldRetract);
				}
			}
		}
		m_extrudeUsable.set(canExtrude ? 1 : 0);
		m_retractUsable.set(canRetract ? 1 : 0);
	}

	void ExtruderSource::clear()
	{
		m_extrudeUsable.set(0);
		m_retractUsable.set(0);
	}

	void ExtruderSource::extrude(int direction) const
	{
		// Relative extrusion (M83) inside M120/M121, so the printer's own mode is kept
		Services::sendGcode(
			fmt::format("M120\nM83\nG1 E{:g} F{:g}\nM121\n",
						Units::convertDisplayedDistanceToDuetUnits(static_cast<float>(direction) * m_distance.value()),
						Units::convertDisplayedSpeedToDuetUnits(m_feedrate.value())));
	}
} // namespace UI::Xml

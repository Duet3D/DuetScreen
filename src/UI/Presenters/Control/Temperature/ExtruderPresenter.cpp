/*
 * ExtruderPresenter.cpp
 */

#include "ExtruderPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"

namespace UI
{
	ExtruderPresenter::ExtruderPresenter()
		: XmlPresenter("temperature_extrusion")
		, m_distance({.id = "distance",
					  .storageKey = std::string(ID_MVS_EXTRUSION_DISTANCES),
					  .presets = {1.0f, 2.0f, 5.0f, 10.0f},
					  .min = 0.1f})
		, m_feedrate({.id = "feedrate",
					  .storageKey = std::string(ID_MVS_EXTRUSION_FEEDRATES),
					  .presets = {1.0f, 5.0f, 20.0f, 50.0f},
					  .min = 0.1f})
	{
		m_distance.setTitle(_("extrude.feed_dist", Units::getDisplayedDistanceUnit()));
		m_feedrate.setTitle(_("extrude.feed_rate", Units::getDisplayedSpeedUnit()));

		action("extruder.extrude",
			   "Extrude the chosen distance at the chosen feed rate with the selected tool",
			   [this](const Xml::ActionArgs&) { OM::Move::Extrude(m_distance.value(), m_feedrate.value()); });
		action("extruder.retract",
			   "Retract the chosen distance at the chosen feed rate with the selected tool",
			   [this](const Xml::ActionArgs&) { OM::Move::Extrude(-m_distance.value(), m_feedrate.value()); });
		for (auto& [name, description, handler] : Xml::ValueSelector::actions({&m_distance, &m_feedrate}))
		{
			action(name, description, handler);
		}
	}

	void ExtruderPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field(
			"extrude_usable",
			m_extrudeUsable,
			"1 if the selected tool can extrude: there is one, it is hot enough (or cold extrusion is allowed), and no "
			"job is running");
		f.field(
			"retract_usable",
			m_retractUsable,
			"1 if the selected tool can retract: there is one, it is hot enough (or cold extrusion is allowed), and no "
			"job is running");
		m_distance.fields(f);
		m_feedrate.fields(f);
	}

	void ExtruderPresenter::onInit()
	{
		m_watch = Xml::OmWatch({"tools", "heat", "state.currentTool"}, [this] { update(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { update(); });
	}

	void ExtruderPresenter::onActivate()
	{
		update();
	}

	void ExtruderPresenter::onDisconnect()
	{
		m_extrudeUsable.set(0);
		m_retractUsable.set(0);
	}

	void ExtruderPresenter::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto tool = Xml::PrinterModel::currentTool();
		const bool jobRunning = OM::PrintInProgress() && OM::GetStatus() != OM::PrinterStatus::paused;
		bool canExtrude = tool.has_value() && !jobRunning;
		bool canRetract = canExtrude;
		if (tool)
		{
			// Every heater of the tool must be hot enough. A limit of 0 is how the Duet reports cold extrusion
			// allowed (M302 P1): it then checks no temperature at all, so neither does this, even a heater whose
			// sensor reads nothing (-273°C).
			const OM::Node& heat = Xml::PrinterModel::root()["heat"];
			const double coldExtrude = heat["coldExtrudeTemperature"].number();
			const double coldRetract = heat["coldRetractTemperature"].number();
			for (size_t slot = 0; slot < Xml::PrinterModel::tool(*tool)["heaters"].size(); ++slot)
			{
				if (const auto heater = Xml::PrinterModel::toolHeater(*tool, slot))
				{
					const double current = Xml::PrinterModel::heater(*heater)["current"].number();
					canExtrude = canExtrude && (coldExtrude <= 0.0 || current >= coldExtrude);
					canRetract = canRetract && (coldRetract <= 0.0 || current >= coldRetract);
				}
			}
		}
		m_extrudeUsable.set(canExtrude ? 1 : 0);
		m_retractUsable.set(canRetract ? 1 : 0);
	}
} // namespace UI

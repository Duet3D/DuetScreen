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
		registerEventListener<EventType::ToolData>([this]() { update(); });
		registerEventListener<EventType::HeaterData>([this]() { update(); });
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
		auto tool = OM::GetCurrentTool();
		const bool jobRunning = OM::PrintInProgress() && OM::GetStatus() != OM::PrinterStatus::paused;
		bool canExtrude = tool != nullptr && !jobRunning;
		bool canRetract = canExtrude;
		if (tool != nullptr)
		{
			// Every heater of the tool must be hot enough
			const float coldExtrude = OM::Heat::GetColdExtrudeTemperature();
			const float coldRetract = OM::Heat::GetColdRetractTemperature();
			tool->IterateHeaters(
				[&](OM::ToolHeaterPtr heater, size_t)
				{
					if (heater == nullptr || heater->heater == nullptr)
					{
						return;
					}
					canExtrude = canExtrude && heater->heater->current >= coldExtrude;
					canRetract = canRetract && heater->heater->current >= coldRetract;
				});
		}
		m_extrudeUsable.set(canExtrude ? 1 : 0);
		m_retractUsable.set(canRetract ? 1 : 0);
	}
} // namespace UI

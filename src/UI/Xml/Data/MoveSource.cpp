/*
 * MoveSource.cpp
 */

#include "MoveSource.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>

namespace UI::Xml
{
	namespace
	{
		/** `value` in thousandths of `max`, 0-1000 */
		int32_t permille(float value, int32_t max)
		{
			return std::clamp(
				static_cast<int32_t>(std::lround(1000.0f * value / static_cast<float>(std::max(max, 1)))), 0, 1000);
		}
	} // namespace

	void AxisRow::fields(FieldVisitor& f)
	{
		f.field("letter", letter, "The axis' letter, e.g. \"X\"");
		f.field("position", position, "User position, two decimal places, e.g. \"12.50\"");
		f.field("homed", homed, "1 when the axis is homed, else 0");
		f.field("visible", visible, "1 if the axis is shown to the user, else 0");
	}

	MoveSource::MoveSource()
		: DataSource("move")
	{
	}

	void MoveSource::fields(FieldVisitor& f)
	{
		f.list("move.axes", m_axes, "One row per axis, in slot order");
		f.field("move.speed_factor", m_speedFactor, "The speed override, e.g. \"100%\"");
		f.field("move.extrusion_factor",
				m_extrusionFactor,
				"The extrusion override of the current tool's extruders (every extruder's, if there is no current "
				"tool), e.g. \"100%\"");
		f.field("move.babystep", m_babystep, "The Z babystep, e.g. \"0.05 mm\"");
		f.field("move.speed_text", m_speedText, "The current move's top and requested speeds, e.g. \"0/0 mm/s\"");
		f.field("move.speed_top",
				m_speedTop,
				"The current move's top speed, in thousandths of the fastest axis' maximum (0-1000), for a gauge");
		f.field("move.speed_requested",
				m_speedRequested,
				"The current move's requested speed, in thousandths of the fastest axis' maximum (0-1000)");
		f.field("move.flow_text", m_flowText, "The volumetric flow, e.g. \"0.0 mm³/s\"");
		f.field("move.flow",
				m_flow,
				"The volumetric flow, in thousandths of the current tool's fastest extruder's maximum (0-1000)");
	}

	void MoveSource::onInit()
	{
		registerEventListener<EventType::AxesData>([this]() { showAxes(); });
		registerEventListener<EventType::ExtruderData>([this]() { showExtrusionFactor(); });
		registerEventListener<EventType::SpeedFactor>([this]() { showSpeedFactor(); });
		registerEventListener<EventType::CurrentMoveRequestedSpeed>([this]() { showSpeeds(); });
		registerEventListener<EventType::CurrentMoveTopSpeed>([this]() { showSpeeds(); });
		registerEventListener<EventType::CurrentMoveExtrusionSpeed>([this]() { showFlow(); });
	}

	void MoveSource::update()
	{
		showAxes();
		showSpeedFactor();
		showExtrusionFactor();
		showSpeeds();
		showFlow();
	}

	void MoveSource::clear()
	{
		m_axes.clear();
		m_speedFactor.set(_("status.speed_multiplier", 100u));
		m_extrusionFactor.set(_("status.flow_multiplier", 100u));
		m_babystep.set(_("status.babystep_value", 0.0f));
		m_speedText.set(_("status.speed_label", 0.0f, 0.0f));
		m_speedTop.set(0);
		m_speedRequested.set(0);
		m_flowText.set(_("status.flow_rate", 0.0f));
		m_flow.set(0);
	}

	uint32_t MoveSource::extrusionFactor()
	{
		MODEL_LOCK();
		uint32_t factors = 0;
		uint32_t count = 0;
		auto add = [&](OM::Move::ExtruderAxisPtr extruder)
		{
			factors += static_cast<uint32_t>(std::lround(100 * extruder->factor));
			count++;
		};
		if (auto tool = OM::GetCurrentTool())
		{
			tool->IterateExtruders([&](OM::Move::ExtruderAxisPtr extruder, size_t) { add(extruder); });
		}
		else
		{
			OM::Move::IterateExtruderAxesWhile(
				[&](OM::Move::ExtruderAxisPtr extruder, size_t)
				{
					add(extruder);
					return true;
				});
		}
		return count > 0 ? factors / count : 100u;
	}

	void MoveSource::showAxes()
	{
		ZoneScoped;
		{
			MODEL_LOCK();
			int32_t maxSpeed = 1;
			m_axes.sync(OM::Move::GetAxisCount(),
						[&maxSpeed](AxisRow& row, size_t slot)
						{
							if (auto axis = OM::Move::GetAxisBySlot(slot))
							{
								row.letter.set(axis->letter);
								row.position.set(fmt::format("{:.2f}", axis->userPosition));
								row.homed.set(axis->homed ? 1 : 0);
								row.visible.set(axis->visible ? 1 : 0);
								maxSpeed = std::max(maxSpeed, axis->maxSpeed);
							}
						});
			m_maxSpeed = maxSpeed;
			auto z = OM::Move::GetAxisByLetter('Z');
			m_babystep.set(_("status.babystep_value", z ? z->babystep : 0.0f));
		}
		showSpeeds();
	}

	void MoveSource::showSpeedFactor()
	{
		m_speedFactor.set(
			_("status.speed_multiplier", static_cast<uint32_t>(std::lround(100 * OM::Move::GetSpeedFactor()))));
	}

	void MoveSource::showExtrusionFactor()
	{
		m_extrusionFactor.set(_("status.flow_multiplier", extrusionFactor()));
		{
			MODEL_LOCK();
			// The current tool's extruders; with no tool (or none with extruders) the range stays as it was
			int32_t maxRate = 0;
			if (auto tool = OM::GetCurrentTool())
			{
				tool->IterateExtruders([&](OM::Move::ExtruderAxisPtr extruder, size_t)
									   { maxRate = std::max(maxRate, extruder->maxSpeed); });
			}
			if (maxRate > 0)
			{
				m_maxExtrusionRate = maxRate;
			}
		}
		showFlow();
	}

	void MoveSource::showSpeeds()
	{
		const float top = OM::Move::GetCurrentMoveTopSpeed();
		const float requested = OM::Move::GetCurrentMoveRequestedSpeed();
		m_speedTop.set(permille(std::round(top), m_maxSpeed));
		m_speedRequested.set(permille(std::round(requested), m_maxSpeed));
		m_speedText.set(_("status.speed_label", top, requested));
	}

	void MoveSource::showFlow()
	{
		const float volumetric = OM::Move::GetVolumetricFlow();
		m_flow.set(permille(std::round(volumetric), m_maxExtrusionRate));
		m_flowText.set(_("status.flow_rate", volumetric));
	}
} // namespace UI::Xml

/*
 * MoveSource.cpp
 */

#include "MoveSource.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <cmath>

namespace UI::Xml
{
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
	}

	void MoveSource::onInit()
	{
		registerEventListener<EventType::AxesData>([this]() { showAxes(); });
		registerEventListener<EventType::ExtruderData>([this]() { showExtrusionFactor(); });
		registerEventListener<EventType::SpeedFactor>([this]() { showSpeedFactor(); });
	}

	void MoveSource::update()
	{
		showAxes();
		showSpeedFactor();
		showExtrusionFactor();
	}

	void MoveSource::clear()
	{
		m_axes.clear();
		m_speedFactor.set(_("status.speed_multiplier", 100u));
		m_extrusionFactor.set(_("status.flow_multiplier", 100u));
		m_babystep.set(_("status.babystep_value", 0.0f));
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
		MODEL_LOCK();
		m_axes.sync(OM::Move::GetAxisCount(),
					[](AxisRow& row, size_t slot)
					{
						if (auto axis = OM::Move::GetAxisBySlot(slot))
						{
							row.letter.set(axis->letter);
							row.position.set(fmt::format("{:.2f}", axis->userPosition));
							row.homed.set(axis->homed ? 1 : 0);
							row.visible.set(axis->visible ? 1 : 0);
						}
					});
		auto z = OM::Move::GetAxisByLetter('Z');
		m_babystep.set(_("status.babystep_value", z ? z->babystep : 0.0f));
	}

	void MoveSource::showSpeedFactor()
	{
		m_speedFactor.set(
			_("status.speed_multiplier", static_cast<uint32_t>(std::lround(100 * OM::Move::GetSpeedFactor()))));
	}

	void MoveSource::showExtrusionFactor()
	{
		m_extrusionFactor.set(_("status.flow_multiplier", extrusionFactor()));
	}
} // namespace UI::Xml

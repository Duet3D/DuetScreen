/*
 * MoveSource.cpp
 */

#include "MoveSource.h"
#include "PrinterModel.h"
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

	MoveSource::MoveSource()
		: DataSource("move")
	{
	}

	void MoveSource::fields(FieldVisitor& f)
	{
		f.field("speed_factor", m_speedFactor, "The speed override, e.g. \"100%\"");
		f.field("extrusion_factor",
				m_extrusionFactor,
				"The extrusion override of the current tool's extruders (every extruder's, if there is no current "
				"tool), e.g. \"100%\"");
		f.field("babystep", m_babystep, "The Z babystep, e.g. \"0.05 mm\"");
		f.field("speed_text", m_speedText, "The current move's top and requested speeds, e.g. \"0/0 mm/s\"");
		f.field("speed_top",
				m_speedTop,
				"The current move's top speed, in thousandths of the fastest axis' maximum (0-1000), for a gauge");
		f.field("speed_requested",
				m_speedRequested,
				"The current move's requested speed, in thousandths of the fastest axis' maximum (0-1000)");
		f.field("flow_text", m_flowText, "The volumetric flow, e.g. \"0.0 mm³/s\"");
		f.field("flow",
				m_flow,
				"The volumetric flow, in thousandths of the current tool's fastest extruder's maximum (0-1000)");
	}

	void MoveSource::onInit()
	{
		watchObjectModel({"move", "tools", "state.currentTool"});
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
		m_speedFactor.set(_("status.speed_multiplier", 100u));
		m_extrusionFactor.set(_("status.flow_multiplier", 100u));
		m_babystep.set(_("status.babystep_value", 0.0f));
		m_speedText.set(_("status.speed_label", 0.0f, 0.0f));
		m_speedTop.set(0);
		m_speedRequested.set(0);
		m_flowText.set(_("status.flow_rate", 0.0f));
		m_flow.set(0);
	}

	std::vector<size_t> MoveSource::extruders()
	{
		std::vector<size_t> numbers;
		const OM::Node& all = move()["extruders"];
		if (const auto tool = PrinterModel::currentTool())
		{
			const OM::Node& toolExtruders = PrinterModel::tool(*tool)["extruders"];
			for (size_t i = 0; i < toolExtruders.size(); ++i)
			{
				if (const int64_t number = toolExtruders[i].integer(-1);
					number >= 0 && all[static_cast<size_t>(number)].exists())
				{
					numbers.push_back(static_cast<size_t>(number));
				}
			}
			return numbers;
		}
		for (size_t number = 0; number < all.size(); ++number)
		{
			if (all[number].exists())
			{
				numbers.push_back(number);
			}
		}
		return numbers;
	}

	uint32_t MoveSource::extrusionFactor()
	{
		MODEL_LOCK();
		uint32_t factors = 0;
		const auto numbers = extruders();
		for (const size_t number : numbers)
		{
			factors += static_cast<uint32_t>(std::lround(100 * move()["extruders"][number]["factor"].number()));
		}
		return numbers.empty() ? 100u : factors / static_cast<uint32_t>(numbers.size());
	}

	float MoveSource::speedFactor()
	{
		MODEL_LOCK();
		return static_cast<float>(move()["speedFactor"].number(1));
	}

	const OM::Node& MoveSource::move()
	{
		return PrinterModel::root()["move"];
	}

	void MoveSource::showAxes()
	{
		ZoneScoped;
		{
			MODEL_LOCK();
			// The fastest axis sets the speed gauges' range; the axis' speed is in mm/min
			int32_t maxSpeed = 1;
			double babystep = 0;
			const OM::Node& axes = move()["axes"];
			for (size_t number = 0; number < axes.size(); ++number)
			{
				maxSpeed = std::max(maxSpeed, static_cast<int32_t>(axes[number]["speed"].integer(0) / 60));
				if (axes[number]["letter"].text() == "Z")
				{
					babystep = axes[number]["babystep"].number();
				}
			}
			m_maxSpeed = maxSpeed;
			m_babystep.set(_("status.babystep_value", static_cast<float>(babystep)));
		}
		showSpeeds();
	}

	void MoveSource::showSpeedFactor()
	{
		MODEL_LOCK();
		m_speedFactor.set(
			_("status.speed_multiplier", static_cast<uint32_t>(std::lround(100 * move()["speedFactor"].number(1)))));
	}

	void MoveSource::showExtrusionFactor()
	{
		m_extrusionFactor.set(_("status.flow_multiplier", extrusionFactor()));
		{
			MODEL_LOCK();
			// The current tool's extruders (speed in mm/min); with no tool (or none with extruders) the range stays
			int32_t maxRate = 0;
			if (PrinterModel::currentTool())
			{
				for (const size_t number : extruders())
				{
					maxRate =
						std::max(maxRate, static_cast<int32_t>(move()["extruders"][number]["speed"].integer(0) / 60));
				}
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
		float top = 0;
		float requested = 0;
		{
			MODEL_LOCK();
			top = static_cast<float>(move()["currentMove"]["topSpeed"].number());
			requested = static_cast<float>(move()["currentMove"]["requestedSpeed"].number());
		}
		m_speedTop.set(permille(std::round(top), m_maxSpeed));
		m_speedRequested.set(permille(std::round(requested), m_maxSpeed));
		m_speedText.set(_("status.speed_label", top, requested));
	}

	float MoveSource::volumetricFlow()
	{
		MODEL_LOCK();
		const auto tool = PrinterModel::currentTool();
		if (!tool)
		{
			return 0.0f;
		}
		// The filament's cross section, weighted by the tool's mix, averaged over its extruders, times the
		// extrusion rate
		const OM::Node& toolNode = PrinterModel::tool(*tool);
		const OM::Node& toolExtruders = toolNode["extruders"];
		float area = 0;
		size_t count = 0;
		for (size_t i = 0; i < toolExtruders.size(); ++i)
		{
			const int64_t number = toolExtruders[i].integer(-1);
			const OM::Node& extruder =
				number >= 0 ? move()["extruders"][static_cast<size_t>(number)] : OM::Node::missing();
			if (!extruder.exists())
			{
				continue;
			}
			count++;
			const float radius = static_cast<float>(extruder["filamentDiameter"].number()) / 2.0f;
			area += static_cast<float>(toolNode["mix"][i].number()) * static_cast<float>(M_PI) * radius * radius;
		}
		if (count == 0)
		{
			return 0.0f;
		}
		return area / static_cast<float>(count) * static_cast<float>(move()["currentMove"]["extrusionRate"].number());
	}

	void MoveSource::showFlow()
	{
		const float volumetric = volumetricFlow();
		m_flow.set(permille(std::round(volumetric), m_maxExtrusionRate));
		m_flowText.set(_("status.flow_rate", volumetric));
	}
} // namespace UI::Xml

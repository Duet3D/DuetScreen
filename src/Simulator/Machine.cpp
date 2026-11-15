/*
 * Machine.cpp
 *
 * The machine's state and how it moves on with time. G-code is in MachineGcode.cpp, the object model in
 * MachineModel.cpp.
 */

#include "Machine.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Sim
{
	namespace
	{
		/** Seconds per step when time is advanced: smaller steps for a smooth approach to a target */
		constexpr double STEP = 0.1;

		/** A heater approaches its target by this fraction of the difference each second, before limits */
		constexpr float APPROACH = 0.35f;

		/** Workplaces there are (G54 to G59.3) */
		constexpr size_t WORKPLACES = 9;

		/** Spindles change speed by this much each second, rpm */
		constexpr float SPINDLE_ACCELERATION = 8000.0f;

		/** A heater is warm enough to print within this of its target, °C */
		constexpr float WARM_ENOUGH = 3.0f;

		/** Within this of its target a heater is holding its temperature, and its readings noisy, °C */
		constexpr float SETTLING = 2.0f;

		/** The thermostatic fans' heater turns them on above this, °C */
		constexpr float THERMOSTATIC_ON = 45.0f;
	} // namespace

	Machine::Machine(const Config& config)
	{
		reconfigure(config);
	}

	void Machine::reconfigure(const Config& config)
	{
		m_config = config;

		m_heaters.resize(m_config.heaters.size());
		for (size_t i = 0; i < m_heaters.size(); i++)
		{
			HeaterState& heater = m_heaters[i];
			if (heater.current == 0.0f)
			{
				heater.current = heater.reading = AMBIENT;
			}
			if (m_config.heaters[i].fault)
			{
				heater.state = "fault";
			}
			else if (heater.state == "fault")
			{
				heater.state = "off";
			}
		}

		m_tools.resize(m_config.tools.size());
		if (m_currentTool >= static_cast<int32_t>(m_tools.size()))
		{
			m_currentTool = -1;
		}

		const size_t oldAxes = m_axes.size();
		m_axes.resize(m_config.axes.size());
		for (size_t i = 0; i < m_axes.size(); i++)
		{
			AxisState& axis = m_axes[i];
			const Axis& limits = m_config.axes[i];
			axis.workplaceOffsets.resize(WORKPLACES, 0.0f);
			if (i >= oldAxes)
			{
				axis.position = axis.target = limits.min;
			}
			axis.position = std::clamp(axis.position, limits.min, std::max(limits.min, limits.max));
			axis.target = std::clamp(axis.target, limits.min, std::max(limits.min, limits.max));
		}

		const size_t oldExtruders = m_extruders.size();
		m_extruders.resize(m_config.extruders.size());
		for (size_t e = oldExtruders; e < m_extruders.size(); e++)
		{
			m_extruders[e].filament = m_config.extruders[e].filament;
		}
		m_fans.resize(m_config.fans.size());
		m_spindles.resize(m_config.spindles.size());

		if (m_job.running && m_job.file >= m_config.files.size())
		{
			endJob();
		}
	}

	std::string Machine::status() const
	{
		if (m_forcedStatus)
		{
			return *m_forcedStatus;
		}
		if (m_halted)
		{
			return "halted";
		}
		if (m_transition)
		{
			return m_transition->status;
		}
		if (m_job.running)
		{
			return "processing";
		}
		const bool moving = std::any_of(
			m_axes.begin(), m_axes.end(), [](const AxisState& axis) { return axis.position != axis.target; });
		return moving ? "busy" : "idle";
	}

	float Machine::temperature(size_t heater) const
	{
		return heater < m_heaters.size() ? m_heaters[heater].reading : AMBIENT;
	}

	std::optional<size_t> Machine::toolExtruder() const
	{
		if (m_currentTool < 0)
		{
			return std::nullopt;
		}
		const auto& extruders = m_config.tools[static_cast<size_t>(m_currentTool)].extruders;
		if (extruders.empty() || extruders.front() < 0 || static_cast<size_t>(extruders.front()) >= m_extruders.size())
		{
			return std::nullopt;
		}
		return static_cast<size_t>(extruders.front());
	}

	float Machine::position(size_t axis) const
	{
		return axis < m_axes.size() ? m_axes[axis].position : 0.0f;
	}

	void Machine::advance(double seconds)
	{
		std::normal_distribution<float> noise(0.0f, 1.0f);
		while (seconds > 0.0)
		{
			const double dt = std::min(seconds, STEP);
			const float fdt = static_cast<float>(dt);
			seconds -= dt;
			m_upTime += dt;

			// A pause, resume, cancel or tool change finishes
			if (m_transition)
			{
				m_transition->remaining -= dt;
				if (m_transition->remaining <= 0.0)
				{
					const std::string then = m_transition->then;
					m_transition.reset();
					if (then == "cancelled")
					{
						endJob();
					}
					else if (then == "paused")
					{
						m_transition = Transition{.status = "paused", .remaining = INFINITY, .then = ""};
					}
				}
			}

			// Heaters approach their targets: fast while far away, slowing as they get close
			for (size_t i = 0; i < m_heaters.size(); i++)
			{
				HeaterState& heater = m_heaters[i];
				const Heater& config = m_config.heaters[i];
				float target = AMBIENT;
				if (heater.state == "active")
				{
					target = heater.active;
				}
				else if (heater.state == "standby")
				{
					target = heater.standby;
				}
				target = std::max(target, AMBIENT);
				const float heatRate = std::max(0.1f, config.heatingRate);
				const float coolRate = std::max(0.1f, config.coolingRate);
				const float change = std::clamp((target - heater.current) * APPROACH, -coolRate, heatRate);
				heater.current += change * fdt;
				// Power: full while heating hard, enough to hold the temperature once there
				const float holding = (target - AMBIENT) / std::max(1.0f, config.max - AMBIENT);
				heater.pwm =
					target > AMBIENT ? std::clamp(holding + std::max(0.0f, change) / heatRate, 0.0f, 1.0f) : 0.0f;
				// Noisy only while holding its temperature: a heater heating or cooling reads smoothly, the noise
				// fading in over the last few degrees
				const float settled = std::clamp(1.0f - std::fabs(target - heater.current) / SETTLING, 0.0f, 1.0f);
				heater.reading = heater.current + (m_noise > 0.0f ? noise(m_random) * m_noise * settled : 0.0f);
			}

			// Fans: thermostatic ones follow the first tool's heater; the rest run as asked
			const float hotend = !m_config.tools.empty() && !m_config.tools.front().heaters.empty()
									 ? temperature(static_cast<size_t>(m_config.tools.front().heaters.front()))
									 : AMBIENT;
			for (size_t i = 0; i < m_fans.size(); i++)
			{
				if (m_config.fans[i].thermostatic)
				{
					m_fans[i].requested = hotend > THERMOSTATIC_ON ? 1.0f : 0.0f;
				}
				m_fans[i].actual = m_fans[i].requested;
			}

			// Spindles speed up and slow down
			for (SpindleState& spindle : m_spindles)
			{
				const float target = spindle.state == "stopped" ? 0.0f : static_cast<float>(spindle.active);
				const float step = SPINDLE_ACCELERATION * fdt;
				const float current = static_cast<float>(spindle.current);
				spindle.current =
					static_cast<int32_t>(std::lround(current + std::clamp(target - current, -step, step)));
			}

			// Axes travel towards where they were sent, together, at the feed rate
			float distance = 0.0f;
			for (const AxisState& axis : m_axes)
			{
				distance += (axis.target - axis.position) * (axis.target - axis.position);
			}
			distance = std::sqrt(distance);
			if (distance > 0.0f)
			{
				const float travel = m_feedRate * m_speedFactor * fdt;
				const float fraction = std::min(1.0f, travel / distance);
				for (AxisState& axis : m_axes)
				{
					axis.position =
						fraction >= 1.0f ? axis.target : axis.position + (axis.target - axis.position) * fraction;
				}
			}

			// A job prints: once warm, it works through its layers, the head tracing each one
			if (m_job.running && !m_transition && !m_halted)
			{
				m_job.duration += dt;
				if (m_job.warming)
				{
					m_job.warmUp += dt;
					const bool warm = std::all_of(
						m_heaters.begin(),
						m_heaters.end(),
						[](const HeaterState& heater)
						{ return heater.state != "active" || heater.current >= heater.active - WARM_ENOUGH; });
					m_job.warming = !warm;
				}
				else
				{
					const JobFile& file = m_config.files[m_job.file];
					m_job.progress += dt * static_cast<double>(m_speedFactor) / std::max(1, file.printTime);
					const float layers = static_cast<float>(std::max(1, file.layers));
					const float layer = std::floor(static_cast<float>(m_job.progress) * layers);
					const float turn = static_cast<float>(m_job.progress) * layers * 2.0f * std::numbers::pi_v<float>;
					for (size_t i = 0; i < std::min<size_t>(3, m_axes.size()); i++)
					{
						const Axis& limits = m_config.axes[i];
						const float middle = (limits.min + limits.max) / 2.0f;
						const float radius = (limits.max - limits.min) / 6.0f;
						float position = middle;
						if (i == 0)
						{
							position = middle + radius * std::cos(turn);
						}
						else if (i == 1)
						{
							position = middle + radius * std::sin(turn);
						}
						else if (m_config.mode == MachineMode::fff)
						{
							position = limits.min + (layer + 1.0f) * 0.2f;
						}
						m_axes[i].position = m_axes[i].target = std::clamp(position, limits.min, limits.max);
					}
					if (const auto extruder = toolExtruder())
					{
						m_extruders[*extruder].position += 1.5f * m_extruders[*extruder].factor * fdt;
					}
					if (m_job.progress >= 1.0)
					{
						m_job.progress = 1.0;
						endJob();
					}
				}
			}
			else if (m_job.running && m_transition && m_transition->status == "paused")
			{
				m_job.duration += dt;
				m_job.pauseDuration += dt;
			}
		}
	}

	void Machine::showMessageBox(int32_t mode)
	{
		MessageBox box{.seq = ++m_messageBoxSeq,
					   .mode = std::clamp(mode, 0, 7),
					   .title = "Simulated message",
					   .message = "A message box of mode " + std::to_string(mode) + " from the simulated Duet",
					   .cancelButton = mode >= 3};
		switch (box.mode)
		{
		case 2:
		case 3:
			box.axisControls = 0b111; // X, Y and Z
			break;
		case 4:
			box.choices = {"Red", "Green", "Blue"};
			box.defaultValue = 0;
			break;
		case 5:
			box.min = 0.0f;
			box.max = 100.0f;
			box.defaultValue = 50;
			break;
		case 6:
			box.min = 0.0f;
			box.max = 10.0f;
			box.defaultValue = 2.5f;
			break;
		case 7:
			box.min = 1.0f;
			box.max = 20.0f;
			box.defaultValue = "text";
			break;
		default:
			break;
		}
		m_messageBox = box;
	}

	void Machine::endJob()
	{
		if (!m_job.running)
		{
			return;
		}
		m_job.running = false;
		m_job.warming = false;
		m_job.lastFileName = "0:/gcodes/" + m_config.files[std::min(m_job.file, m_config.files.size() - 1)].name;
		m_job.lastDuration = m_job.duration;
		m_transition.reset();
	}

	void Machine::stopEverything()
	{
		for (HeaterState& heater : m_heaters)
		{
			if (heater.state != "fault")
			{
				heater.state = "off";
			}
		}
		for (ToolState& tool : m_tools)
		{
			tool.state = "off";
		}
		for (FanState& fan : m_fans)
		{
			fan.requested = 0.0f;
		}
		for (SpindleState& spindle : m_spindles)
		{
			spindle.state = "stopped";
		}
		for (AxisState& axis : m_axes)
		{
			axis.target = axis.position;
		}
	}
} // namespace Sim

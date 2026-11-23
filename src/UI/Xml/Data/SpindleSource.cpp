/*
 * SpindleSource.cpp
 */

#include "SpindleSource.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** The quick speeds, as fractions of the spindle's highest */
		constexpr float QUICK_SPEEDS[] = {0.25f, 0.5f, 0.75f, 1.0f};

		/** Quick speeds are rounded to this, rpm */
		constexpr float QUICK_ROUNDING = 100.0f;
	} // namespace

	void SpindlePresetRow::fields(FieldVisitor& f)
	{
		f.field("text", text, "The speed, e.g. \"12 000\" (actions.spindle.preset)");
		f.field("chosen", chosen, "1 if the tool starts its spindle at it");
	}

	SpindleSource::SpindleSource()
		: DataSource("spindle")
	{
	}

	void SpindleSource::fields(FieldVisitor& f)
	{
		f.field("shown", m_shown, "1 if the tool in use has a spindle");
		f.field("title", m_title, "Which, e.g. \"Spindle · T1\"");
		f.field("running", m_running, "1 while it turns");
		f.field("state", m_state, "\"Off\", \"Forward\" or \"Reverse\"");
		f.field("current", m_current, "How fast it turns, rpm, e.g. \"12 000\"");
		f.field("summary", m_summary, "How it starts, e.g. \"Set 12 000 rpm, forward\"");
		f.field("reverse",
				m_reverseChosen,
				"1 if it starts in reverse (actions.spindle.direction); while it turns, 1 if it turns in reverse");
		f.field("can_reverse", m_canReverse, "1 if it can turn in reverse");
		f.list("presets", m_presets, "Quick speeds, a quarter of its highest up to its highest");
		f.field("presets_shown", m_presetsShown, "1 if there are quick speeds: its highest speed is known");
	}

	void SpindleSource::onInit()
	{
		watchObjectModel({"tools", "state.currentTool", "spindles"});
	}

	std::optional<SpindleSource::InUse> SpindleSource::inUse()
	{
		const auto tool = PrinterModel::currentTool();
		if (!tool)
		{
			return std::nullopt;
		}
		const int64_t spindle = PrinterModel::tool(*tool)["spindle"].integer(-1);
		if (spindle < 0)
		{
			return std::nullopt;
		}
		return InUse{.tool = *tool, .spindle = static_cast<size_t>(spindle)};
	}

	bool SpindleSource::reverse(size_t spindle) const
	{
		const std::string_view state = PrinterModel::root()["spindles"][spindle]["state"].text();
		if (state == "forward" || state == "reverse")
		{
			return state == "reverse";
		}
		const auto chosen = m_reverse.find(spindle);
		return chosen != m_reverse.end() && chosen->second;
	}

	void SpindleSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto used = inUse();
		if (!used)
		{
			clear();
			return;
		}
		const OM::Node& spindle = PrinterModel::root()["spindles"][used->spindle];
		const std::string_view state = spindle["state"].text();
		const bool running = state == "forward" || state == "reverse";
		const bool backwards = reverse(used->spindle);
		const auto rpm = static_cast<int32_t>(std::lround(PrinterModel::tool(used->tool)["spindleRpm"].number()));
		const auto max = static_cast<float>(spindle["max"].number());

		m_shown.set(1);
		m_title.set(_("spindle.title", used->tool));
		m_running.set(running ? 1 : 0);
		m_state.set(running ? _(backwards ? "spindle.reverse" : "spindle.forward") : _("spindle.off"));
		m_current.set(rpmText(std::llabs(std::llround(spindle["current"].number()))));
		m_summary.set(_(backwards ? "spindle.summary_reverse" : "spindle.summary_forward", rpmText(rpm)));
		m_reverseChosen.set(backwards ? 1 : 0);
		m_canReverse.set(spindle["canReverse"].boolean() ? 1 : 0);
		const size_t quick = max > 0 ? std::size(QUICK_SPEEDS) : 0;
		m_presets.sync(quick,
					   [max, rpm](SpindlePresetRow& row, size_t i)
					   {
						   row.rpm = static_cast<int32_t>(std::round(max * QUICK_SPEEDS[i] / QUICK_ROUNDING) *
														  QUICK_ROUNDING);
						   row.text.set(rpmText(row.rpm));
						   row.chosen.set(row.rpm == rpm ? 1 : 0);
					   });
		m_presetsShown.set(quick > 0 ? 1 : 0);
	}

	void SpindleSource::clear()
	{
		m_shown.set(0);
		m_title.set("");
		m_running.set(0);
		m_state.set("");
		m_current.set("");
		m_summary.set("");
		m_reverseChosen.set(0);
		m_canReverse.set(0);
		m_presets.clear();
		m_presetsShown.set(0);
	}

	void SpindleSource::start()
	{
		std::string gcode;
		{
			MODEL_LOCK();
			const auto used = inUse();
			if (!used)
			{
				return;
			}
			const double rpm = std::round(PrinterModel::tool(used->tool)["spindleRpm"].number());
			gcode = fmt::format("{:s} P{:d} S{:g}\n", reverse(used->spindle) ? "M4" : "M3", used->spindle, rpm);
		}
		Services::sendGcode(gcode);
	}

	void SpindleSource::choose(bool reverse)
	{
		bool running = false;
		{
			MODEL_LOCK();
			const auto used = inUse();
			if (!used)
			{
				return;
			}
			m_reverse[used->spindle] = reverse;
			const std::string_view state = PrinterModel::root()["spindles"][used->spindle]["state"].text();
			running = state == "forward" || state == "reverse";
		}
		if (running)
		{
			start();
			return;
		}
		update();
	}

	void SpindleSource::setSpeed(int32_t rpm)
	{
		std::optional<size_t> tool;
		{
			MODEL_LOCK();
			if (const auto used = inUse())
			{
				tool = used->tool;
			}
		}
		if (tool)
		{
			Services::sendGcode(fmt::format("M568 P{:d} F{:d}\n", *tool, std::max(0, rpm)));
		}
	}

	std::string SpindleSource::rpmText(int64_t rpm)
	{
		std::string digits = std::to_string(std::llabs(rpm));
		for (auto at = static_cast<int64_t>(digits.size()) - 3; at > 0; at -= 3)
		{
			digits.insert(static_cast<size_t>(at), " ");
		}
		return rpm < 0 ? "-" + digits : digits;
	}
} // namespace UI::Xml

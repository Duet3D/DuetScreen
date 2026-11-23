/*
 * SpindleActions.cpp
 */

#include "SpindleActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Data/SpindleSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <limits>
#include <optional>

namespace UI::SpindleActions
{
	namespace
	{
		/** A tool's spindle, as an action needs it */
		struct ToolSpindle
		{
			size_t tool;	// The tool's number
			size_t spindle; // Its spindle's number
			float rpm;		// The speed the tool turns it at (tools[].spindleRpm)
			float max;		// The spindle's highest speed
		};

		/** The spindle of the tool an action is about, its `arg`; nullopt if it has none */
		std::optional<ToolSpindle> spindleOf(const Xml::ActionArgs& args)
		{
			const auto number = args.argInt();
			if (!number || *number < 0)
			{
				LOG_WARN("actions.spindle.* need the tool's number as their arg, e.g. arg=\"{{row.index}}\"");
				return std::nullopt;
			}
			MODEL_LOCK();
			const auto tool = static_cast<size_t>(*number);
			const OM::Node& node = Xml::PrinterModel::tool(tool);
			const double spindle = node["spindle"].number(-1);
			if (!node.exists() || spindle < 0)
			{
				return std::nullopt;
			}
			const OM::Node& info = Xml::PrinterModel::root()["spindles"][static_cast<size_t>(spindle)];
			return ToolSpindle{.tool = tool,
							   .spindle = static_cast<size_t>(spindle),
							   .rpm = static_cast<float>(node["spindleRpm"].number()),
							   .max = static_cast<float>(info["max"].number())};
		}

		/** What − and + on a tool's spindle speed take off or add, rpm */
		constexpr float SPEED_STEP = 1000.0f;

		/** Set the tool's spindle speed `steps` steps faster (or slower, if negative), within its limits (M568 F) */
		void step(const Xml::ActionArgs& args, int steps)
		{
			if (const auto spindle = spindleOf(args))
			{
				const float top = spindle->max > 0 ? spindle->max : std::numeric_limits<float>::max();
				const float rpm = std::clamp(spindle->rpm + static_cast<float>(steps) * SPEED_STEP, 0.0f, top);
				Xml::Services::sendGcode(fmt::format("M568 P{:d} F{:g}\n", spindle->tool, std::round(rpm)));
			}
		}

		/** The spindle source; nullptr, with a warning, while the data sources aren't running */
		Xml::SpindleSource* source()
		{
			auto* spindles = Xml::DataSources::find<Xml::SpindleSource>();
			if (spindles == nullptr)
			{
				LOG_WARN("The spindle source isn't running");
			}
			return spindles;
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal(
			"spindle.set_rpm",
			"Ask for the speed the tool numbered `arg` turns its spindle at, and set it (M568 F)",
			[](const Xml::ActionArgs& args)
			{
				const auto spindle = spindleOf(args);
				if (!spindle)
				{
					return;
				}
				Xml::Prompts::askNumber(
					{.header = _("spindle.speed"),
					 .min = 0,
					 .max = spindle->max > 0 ? spindle->max : 100000,
					 .value = spindle->rpm,
					 .onConfirm = [tool = spindle->tool](float rpm)
					 { Xml::Services::sendGcode(fmt::format("M568 P{:d} F{:g}\n", tool, std::round(rpm))); }});
			});
		Xml::Actions::addGlobal("spindle.faster",
								"Turn the spindle of the tool numbered `arg` 1000 rpm faster from now on (M568 F)",
								[](const Xml::ActionArgs& args) { step(args, 1); });
		Xml::Actions::addGlobal("spindle.slower",
								"Turn the spindle of the tool numbered `arg` 1000 rpm slower from now on (M568 F)",
								[](const Xml::ActionArgs& args) { step(args, -1); });
		Xml::Actions::addGlobal("spindle.start",
								"Start the tool in use's spindle at its speed, in the direction chosen (M3, or M4 in "
								"reverse)",
								[](const Xml::ActionArgs&)
								{
									if (auto* spindles = source())
									{
										spindles->start();
									}
								});
		Xml::Actions::addGlobal("spindle.direction",
								"Choose the direction the tool in use's spindle starts in, `arg` `forward` or "
								"`reverse`; while it turns, turn it that way now",
								[](const Xml::ActionArgs& args)
								{
									if (auto* spindles = source())
									{
										spindles->choose(args.arg == "reverse");
									}
								});
		Xml::Actions::addGlobal("spindle.preset",
								"Start the tool in use's spindle at a quick speed from now on (M568 F). From a row of "
								"source.spindle.presets.",
								[](const Xml::ActionArgs& args)
								{
									const auto* row = args.row<Xml::SpindlePresetRow>();
									auto* spindles = source();
									if (row == nullptr || spindles == nullptr)
									{
										LOG_WARN(
											"actions.spindle.preset must come from a row of source.spindle.presets");
										return;
									}
									spindles->setSpeed(row->rpm);
								});
		Xml::Actions::addGlobal("spindle.stop",
								"Stop the spindle of the tool numbered `arg` (M5)",
								[](const Xml::ActionArgs& args)
								{
									if (const auto spindle = spindleOf(args))
									{
										Xml::Services::sendGcode(fmt::format("M5 P{:d}\n", spindle->spindle));
									}
								});
	}
} // namespace UI::SpindleActions

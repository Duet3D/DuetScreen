/*
 * SpindleActions.cpp
 */

#include "SpindleActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <cmath>
#include <fmt/format.h>
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

		/** Turn the tool's spindle at its speed: forwards (M3) or in reverse (M4) */
		void turn(const Xml::ActionArgs& args, bool forward)
		{
			if (const auto spindle = spindleOf(args))
			{
				Xml::Services::sendGcode(fmt::format(
					"{:s} P{:d} S{:g}\n", forward ? "M3" : "M4", spindle->spindle, std::round(spindle->rpm)));
			}
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
		Xml::Actions::addGlobal("spindle.forward",
								"Turn the spindle of the tool numbered `arg` forwards at its speed (M3)",
								[](const Xml::ActionArgs& args) { turn(args, true); });
		Xml::Actions::addGlobal("spindle.reverse",
								"Turn the spindle of the tool numbered `arg` in reverse at its speed (M4)",
								[](const Xml::ActionArgs& args) { turn(args, false); });
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

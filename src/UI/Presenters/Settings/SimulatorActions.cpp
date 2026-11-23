/*
 * SimulatorActions.cpp
 */

#include "SimulatorActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/SimulatorSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <fmt/format.h>
#include <string>
#if DUET_SIMULATOR
#  include "Simulator/Simulator.h"
#endif

namespace UI::SimulatorActions
{
	namespace
	{
		/** The simulated Duet's source; nullptr, with a warning, while the data sources aren't running */
		Xml::SimulatorSource* simulator()
		{
			auto* source = Xml::DataSources::find<Xml::SimulatorSource>();
			if (source == nullptr)
			{
				LOG_WARN("The simulated Duet's source isn't running");
			}
			return source;
		}

		/** `actions.simulator.add` */
		void add(const Xml::ActionArgs& args)
		{
			const auto part = Xml::SimulatorSource::partNamed(args.arg);
			auto* source = simulator();
			if (!part || source == nullptr)
			{
				LOG_WARN("actions.simulator.add: no part called '{:s}'", args.arg);
				return;
			}
			source->add(*part);
		}

		/** `actions.simulator.remove` */
		void remove(const Xml::ActionArgs& args)
		{
			const auto* row = args.row<Xml::SimulatorRow>();
			auto* source = simulator();
			if (row == nullptr || source == nullptr)
			{
				LOG_WARN("actions.simulator.remove: not in a row of source.simulator's parts");
				return;
			}
			source->remove(row->item());
		}

		/** `actions.simulator.set` */
		void set(const Xml::ActionArgs& args)
		{
			const auto* row = args.row<Xml::SimulatorRow>();
			auto* source = simulator();
			if (row == nullptr || source == nullptr)
			{
				LOG_WARN("actions.simulator.set: not in a row of source.simulator's parts");
				return;
			}
			const std::string field = args.arg;
			if (Xml::SimulatorSource::isText(field))
			{
				source->setText(row->item(), field, args.text());
				return;
			}
			if (const auto edited = args.edited())
			{
				if (!source->setNumber(row->item(), field, *edited))
				{
					LOG_WARN("actions.simulator.set: no field '{:s}' to set", field);
				}
				return;
			}
			if (source->toggle(row->item(), field))
			{
				return;
			}
			const auto number = source->number(row->item(), field);
			if (!number)
			{
				LOG_WARN("actions.simulator.set: no field '{:s}' to ask for", field);
				return;
			}
			// Ask for it; the row may be gone by the time it is answered, so the part is kept by its place
			const Xml::SimulatorItem item = row->item();
			Xml::Prompts::askNumber(
				{.header = fmt::format("{:s} · {:s}", row->title.get(), _("simulator.fields." + field)),
				 .min = number->min,
				 .max = number->max,
				 .value = number->value,
				 .unit = number->unit,
				 .onConfirm =
					 [item, field](float value)
				 {
					 if (auto* source = simulator())
					 {
						 source->setNumber(item, field, value);
					 }
				 },
				 .asker = args.target});
		}

		/** `actions.simulator.toggle` */
		void toggle(const Xml::ActionArgs& args)
		{
			const auto* choice = args.row<Xml::SimulatorChoiceRow>();
			const auto* tool = args.row<Xml::SimulatorToolRow>();
			auto* source = simulator();
			if (choice == nullptr || tool == nullptr || source == nullptr)
			{
				LOG_WARN("actions.simulator.toggle: not in a row of a tool's heaters or fans");
				return;
			}
			source->toggle(static_cast<size_t>(tool->index.get()),
						   {.part = choice->part, .index = static_cast<size_t>(choice->index.get())});
		}

		/** `actions.simulator.message` */
		void message([[maybe_unused]] const Xml::ActionArgs& args)
		{
#if DUET_SIMULATOR
			const auto mode = args.argInt();
			if (!mode || *mode < 0 || *mode > 7)
			{
				LOG_WARN("actions.simulator.message: '{:s}' isn't a message box's mode, 0-7", args.arg);
				return;
			}
			Sim::Simulator::get().showMessageBox(*mode);
#else
			LOG_WARN("actions.simulator.message: this build has no simulated Duet");
#endif
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("simulator.add",
								"Add a part to the simulated Duet, with values to start from: `arg` names its list in "
								"source.simulator, e.g. `heaters`",
								add);
		Xml::Actions::addGlobal("simulator.remove",
								"Remove a part of the simulated Duet; tools that used it no longer do. From a row of "
								"source.simulator's parts.",
								remove);
		Xml::Actions::addGlobal(
			"simulator.set",
			"Change the field `arg` names of a part of the simulated Duet, from its row of source.simulator's parts: a "
			"name to the text typed, a choice to the option picked; an on/off field is turned over, and a number asked "
			"for on the number pad",
			set);
		Xml::Actions::addGlobal("simulator.toggle",
								"Give the tool the heater or fan, or take it away. From a row of a tool's `heaters` or "
								"`fans` in source.simulator.tools.",
								toggle);
		Xml::Actions::addGlobal("simulator.message",
								"Make the simulated Duet show a message box of mode `arg`, 0-7, as M291 would",
								message);
	}
} // namespace UI::SimulatorActions

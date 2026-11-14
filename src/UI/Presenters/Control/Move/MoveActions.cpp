/*
 * MoveActions.cpp
 */

#include "MoveActions.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/AxesSource.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <fmt/format.h>

namespace UI::MoveActions
{
	namespace
	{
		/** The axis letter an action is about: its row of `source.axes.others`', else its `arg`'s first character */
		char axisOf(const Xml::ActionArgs& args)
		{
			if (const auto* row = args.row<Xml::AxisRow>())
			{
				const auto letter = row->letter.get();
				return letter.empty() ? '\0' : letter.front();
			}
			return args.arg.empty() ? '\0' : args.arg.front();
		}

		/** Whether the printer has an axis called `letter` */
		bool hasAxis(char letter)
		{
			MODEL_LOCK();
			return Xml::PrinterModel::axis(letter).exists();
		}

		/** Home the axis called `letter` */
		void home(char letter)
		{
			if (hasAxis(letter))
			{
				Xml::Services::sendGcode(fmt::format("G28 {:s}\n", Xml::PrinterModel::axisGcode(letter)));
			}
		}

		/** Jog the axis called `letter` by the chosen distance at the chosen speed, forwards or backwards */
		void jog(char letter, bool forward)
		{
			const auto distance = Xml::JogSource::distance();
			const auto feedrate = Xml::JogSource::feedrate();
			if (!distance || !feedrate || !hasAxis(letter))
			{
				return;
			}
			// M120/M121 keep the printer's movement mode as it was
			Xml::Services::sendGcode(
				fmt::format("M120\nG91\nG1 {:s}{:g} F{:g}\nM121\n",
							Xml::PrinterModel::axisGcode(letter),
							Units::convertDisplayedDistanceToDuetUnits((forward ? 1.0f : -1.0f) * *distance),
							Units::convertDisplayedSpeedToDuetUnits(*feedrate)));
		}

		/** Ask for a position for the axis called `letter`, and move there at the chosen jog speed */
		void askPosition(char letter)
		{
			const auto feedrate = Xml::JogSource::feedrate();
			const bool machine = StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE);
			Xml::Prompts::NumberRequest request;
			{
				MODEL_LOCK();
				const OM::Node& axis = Xml::PrinterModel::axis(letter);
				if (!axis.exists() || !feedrate)
				{
					return;
				}
				const auto position = static_cast<float>(axis[machine ? "machinePosition" : "userPosition"].number());
				request = {.header = _("move.set_position", letter),
						   .min = static_cast<float>(axis["min"].number()),
						   .max = static_cast<float>(axis["max"].number()),
						   .value = Units::convertDistanceToCurrentDisplayedUnit(position),
						   .onConfirm = [letter, machine, feedrate](float value)
						   {
							   Xml::Services::sendGcode(
								   fmt::format("M120\nG90\n{:s}G1 {:s}{:g} F{:g}\nM121\n",
											   machine ? "G53 " : "",
											   Xml::PrinterModel::axisGcode(letter),
											   Units::convertDisplayedDistanceToDuetUnits(value),
											   Units::convertDisplayedSpeedToDuetUnits(*feedrate)));
						   }};
			}
			Xml::Prompts::askNumber(std::move(request));
		}

		/** `actions.move.home` */
		void homeAction(const Xml::ActionArgs& args)
		{
			if (args.arg == "all")
			{
				Xml::Prompts::confirm({.title = _("move.home_all_confirm.title"),
									   .text = _("move.home_all_confirm.text"),
									   .onYes = []() { Xml::Services::sendGcode("G28"); },
									   .asker = args.target});
			}
			else if (args.arg == "XY")
			{
				Xml::Prompts::confirm({.title = _("move.home_xy_confirm.title"),
									   .text = _("move.home_xy_confirm.text"),
									   .onYes =
										   []()
									   {
										   home('X');
										   home('Y');
									   },
									   .asker = args.target});
			}
			else if (const char letter = axisOf(args))
			{
				Xml::Prompts::confirm({.title = _("move.home_generic_confirm.title", letter),
									   .text = _("move.home_generic_confirm.text", letter),
									   .onYes = [letter]() { home(letter); },
									   .asker = args.target});
			}
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("move.jog",
								"Jog by the chosen distance at the chosen speed (source.jog). `arg` is `X+`, `Z-`, …; "
								"from a row of `source.axes.others`, `+` or `-`.",
								"tr.actions.move_jog",
								[](const Xml::ActionArgs& args)
								{
									if (const char letter = axisOf(args))
									{
										jog(letter, !args.arg.ends_with('-'));
									}
								});
		Xml::Actions::addGlobal("move.home",
								"Home: `arg` is `X`, `Y`, `Z`, `XY` or `all`, or raise it from a row of "
								"`source.axes.others`. Asks first if confirmations are on.",
								"tr.actions.move_home",
								homeAction);
		Xml::Actions::addGlobal("move.set_position",
								"Ask for a position and move there. `arg` is the axis letter, or raise it from a row "
								"of `source.axes.others`.",
								[](const Xml::ActionArgs& args)
								{
									if (const char letter = axisOf(args))
									{
										askPosition(letter);
									}
								});
		Xml::Actions::addGlobal("move.disable_motors",
								"Disable the motors (M18), asking first if confirmations are on",
								"tr.actions.move_disable_motors",
								[](const Xml::ActionArgs& args)
								{
									Xml::Prompts::confirm({.title = _("move.disable_motors_confirm.title"),
														   .text = _("move.disable_motors_confirm.text"),
														   .onYes = []() { Xml::Services::sendGcode("M18"); },
														   .asker = args.target});
								});
	}
} // namespace UI::MoveActions

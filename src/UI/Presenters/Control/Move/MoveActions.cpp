/*
 * MoveActions.cpp
 */

#include "MoveActions.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/AxesSource.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <charconv>
#include <fmt/format.h>

namespace UI::MoveActions
{
	namespace
	{
		/**
		 * The axis letter an action is about: its row of `source.axes.all` or `others`; the chosen other axis if its
		 * `arg` starts with `other`; else its `arg`'s first character
		 */
		char axisOf(const Xml::ActionArgs& args)
		{
			if (const auto* row = args.row<Xml::AxisRow>())
			{
				const auto letter = row->letter.get();
				return letter.empty() ? '\0' : letter.front();
			}
			if (args.arg.starts_with("other"))
			{
				return Xml::AxesSource::chosenAxis().value_or('\0');
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
			const auto feedrate = Xml::JogSource::feedrateGcode();
			if (!distance || !feedrate || !hasAxis(letter))
			{
				return;
			}
			// M120/M121 keep the printer's movement mode as it was
			Xml::Services::sendGcode(
				fmt::format("M120\nG91\nG1 {:s}{:g} F{:g}\nM121\n",
							Xml::PrinterModel::axisGcode(letter),
							Units::convertDisplayedDistanceToDuetUnits((forward ? 1.0f : -1.0f) * *distance),
							*feedrate));
		}

		/** Ask for a position for the axis called `letter`, and move there at the chosen jog speed */
		void askPosition(char letter)
		{
			const auto feedrate = Xml::JogSource::feedrateGcode();
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
							   Xml::Services::sendGcode(fmt::format("M120\nG90\n{:s}G1 {:s}{:g} F{:g}\nM121\n",
																	machine ? "G53 " : "",
																	Xml::PrinterModel::axisGcode(letter),
																	Units::convertDisplayedDistanceToDuetUnits(value),
																	*feedrate));
						   }};
			}
			Xml::Prompts::askNumber(std::move(request));
		}

		/** The G-code that selects workplace `number`, 1-9: G54 to G59, then G59.1 to G59.3 */
		std::string workplaceGcode(int number)
		{
			return number <= 6 ? fmt::format("G{:d}", 53 + number) : fmt::format("G59.{:d}", number - 6);
		}

		/**
		 * The current position as work zero, in the workplace in use, for the axes `letters` (G10 L20): the workplace
		 * offsets change, so the position becomes 0
		 */
		void setWorkZero(const std::string& letters)
		{
			std::string axes;
			int workplace = 1;
			{
				MODEL_LOCK();
				workplace = static_cast<int>(Xml::PrinterModel::root()["move"]["workplaceNumber"].number()) + 1;
				for (const char letter : letters)
				{
					if (Xml::PrinterModel::axis(letter).exists())
					{
						axes += fmt::format(" {:s}0", Xml::PrinterModel::axisGcode(letter));
					}
				}
			}
			if (!axes.empty())
			{
				Xml::Services::sendGcode(fmt::format("G10 L20 P{:d}{:s}\n", workplace, axes));
			}
		}

		/** `actions.move.zero` */
		void zeroAction(const Xml::ActionArgs& args)
		{
			std::string letters;
			if (args.arg == "all")
			{
				MODEL_LOCK();
				for (const size_t number : Xml::PrinterModel::visibleAxes())
				{
					const std::string_view letter = Xml::PrinterModel::root()["move"]["axes"][number]["letter"].text();
					if (!letter.empty())
					{
						letters += letter.front();
					}
				}
			}
			else if (const char letter = axisOf(args))
			{
				letters = std::string(1, letter);
			}
			if (letters.empty())
			{
				return;
			}
			Xml::Prompts::confirm({.title = _("move.zero_confirm.title"),
								   .text = _("move.zero_confirm.text", letters),
								   .onYes = [letters]() { setWorkZero(letters); },
								   .asker = args.target});
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
		// Shown and chosen in the Move tab (move_coordinates.xml), so declared with the moves it changes
		Xml::Settings::add({.name = "machine_position_mode",
							.description = "1 if positions are shown in, and moves to a position use, machine "
										   "coordinates (G53), ignoring the workplace's offsets",
							.read = [] { return StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE) ? 1 : 0; },
							.write =
								[](int32_t on)
							{
								StorageHelper::setData(ID_MOVE_MACHINE_POSITION_MODE, on != 0);
								Xml::DataSources::updateAll(); // The positions shown change
							}});

		Xml::Actions::addGlobal("move.jog",
								"Jog by the chosen distance at the chosen speed (source.jog). `arg` is `X+`, `Z-`, …; "
								"`other+` or `other-` for the axis chosen from source.axes.others; from a row of "
								"`source.axes.all` or `others`, `+` or `-`.",
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
								"`source.axes.all` or `others`. Asks first if confirmations are on.",
								"tr.actions.move_home",
								homeAction);
		Xml::Actions::addGlobal("move.set_position",
								"Ask for a position and move there. `arg` is the axis letter, or raise it from a row "
								"of `source.axes.all` or `others`.",
								[](const Xml::ActionArgs& args)
								{
									if (const char letter = axisOf(args))
									{
										askPosition(letter);
									}
								});
		Xml::Actions::addGlobal("move.choose_axis",
								"Choose the axis other than X, Y and Z that is jogged: raise it from a row of "
								"`source.axes.others`, or `arg` is its letter",
								[](const Xml::ActionArgs& args) { Xml::AxesSource::choose(axisOf(args)); });
		Xml::Actions::addGlobal("move.zero",
								"Make the current position work zero in the workplace in use (G10 L20): `arg` is the "
								"axis letter or `all`, or raise it from a row of `source.axes.all` or `others`. Asks "
								"first if confirmations are on.",
								"tr.actions.move_zero",
								zeroAction);
		Xml::Actions::addGlobal("move.workplace",
								"Use workplace `arg`, 1-9: G54 to G59, then G59.1 to G59.3",
								"tr.actions.move_workplace",
								[](const Xml::ActionArgs& args)
								{
									int number = 0;
									std::from_chars(args.arg.data(), args.arg.data() + args.arg.size(), number);
									if (number >= 1 && number <= 9)
									{
										Xml::Services::sendGcode(workplaceGcode(number));
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

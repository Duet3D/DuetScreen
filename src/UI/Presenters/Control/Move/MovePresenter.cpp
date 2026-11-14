/*
 * MovePresenter.cpp
 */

#include "MovePresenter.h"
#include "Debug.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/JogSource.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/NumberEntry.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	namespace
	{
		/** Whether the printer is in a state to home (not busy with a job or still connecting) */
		bool canHome()
		{
			switch (OM::GetStatus())
			{
			case OM::PrinterStatus::connecting:
			case OM::PrinterStatus::pausing:
			case OM::PrinterStatus::resuming:
			case OM::PrinterStatus::processing:
			case OM::PrinterStatus::off:
				return false;
			default:
				return true;
			}
		}

		/** Whether `axis` can be moved: homed, or moves before homing are allowed, and the printer isn't busy */
		/** Whether `axis` can be jogged: homed, or the printer moves axes that aren't (the model lock held) */
		bool canMove(const OM::Node& axis)
		{
			return (axis["homed"].boolean() || !Xml::PrinterModel::noMovesBeforeHoming()) && canHome();
		}

		/** `axis`' position: machine or user, as the user chose */
		float positionOf(const OM::Node& axis, bool machine)
		{
			return static_cast<float>(axis[machine ? "machinePosition" : "userPosition"].number());
		}

		/** An axis' label, e.g. "X: 0.00", in the displayed unit */
		std::string axisLabel(char letter, float position)
		{
			return fmt::format("{}: {:.2f}", letter, Units::convertDistanceToCurrentDisplayedUnit(position));
		}
	} // namespace

	void MoveAxisRow::fields(Xml::FieldVisitor& f)
	{
		f.field("letter", letter, "Axis letter");
		f.field("label", label, "Position in the displayed unit, e.g. \"U: 0.00\"");
		f.field("homed", homed, "1 when homed");
		f.field("jog_usable", jogUsable, "1 if the axis can be jogged or set (homed, or moves before homing allowed)");
		f.field("home_usable", homeUsable, "1 if it can be homed");
	}

	MovePresenter::MovePresenter()
		: XmlPresenter("move")
	{
		action(
			"move.jog",
			"Jog by the chosen distance at the chosen speed. `arg` is `X+`, `Z-`, …; from a row of `axes`, `+` or `-`.",
			[this](const Xml::ActionArgs& args)
			{
				if (const char letter = axisOf(args))
				{
					jog(letter, !args.arg.ends_with('-'));
				}
			});
		action("move.home",
			   "Home: `arg` is `X`, `Y`, `Z`, `XY` or `all`, or raise it from a row of `axes`. Asks first if "
			   "confirmations are on.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (args.arg == "all")
				   {
					   confirmThen(_("move.home_all_confirm.title"),
								   _("move.home_all_confirm.text"),
								   []() { Xml::Services::sendGcode("G28"); });
				   }
				   else if (args.arg == "XY")
				   {
					   confirmThen(_("move.home_xy_confirm.title"),
								   _("move.home_xy_confirm.text"),
								   []()
								   {
									   home('X');
									   home('Y');
								   });
				   }
				   else if (const char letter = axisOf(args))
				   {
					   confirmThen(_("move.home_generic_confirm.title", letter),
								   _("move.home_generic_confirm.text", letter),
								   [letter]() { home(letter); });
				   }
			   });
		action("move.disable_motors",
			   "Disable the motors (M18), asking first if confirmations are on",
			   [this](const Xml::ActionArgs&)
			   {
				   confirmThen(_("move.disable_motors_confirm.title"),
							   _("move.disable_motors_confirm.text"),
							   []() { Xml::Services::sendGcode("M18"); });
			   });
		action("move.set_position",
			   "Ask for a position and move there. `arg` is the axis letter, or raise it from a row of `axes`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const char letter = axisOf(args))
				   {
					   askPosition(letter);
				   }
			   });
		action("move.confirm",
			   "Answer yes to the question (`confirm`): close it and do what it asked",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirm.set(0);
				   if (m_confirmed)
				   {
					   m_confirmed();
				   }
			   });
		action("move.close_dialog",
			   "Answer no to the question (`confirm`): close it without doing anything",
			   [this](const Xml::ActionArgs&) { m_confirm.set(0); });
	}

	void MovePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("connected", m_connected, "1 while connected");
		const std::pair<const char*, NamedAxis*> named[] = {{"x", &m_x}, {"y", &m_y}, {"z", &m_z}};
		for (const auto& [prefix, axis] : named)
		{
			f.field(fmt::format("{}.label", prefix), axis->label, "Position in the displayed unit, e.g. \"X: 0.00\"");
			f.field(fmt::format("{}.homed", prefix), axis->homed, "1 when homed");
			f.field(fmt::format("{}.jog_usable", prefix),
					axis->jogUsable,
					"1 if the axis can be jogged or set (homed, or moves before homing allowed)");
			f.field(fmt::format("{}.home_usable", prefix), axis->homeUsable, "1 if it can be homed");
		}
		f.field("xy.homed", m_xyHomed, "1 when X and Y are homed");
		f.field("xy.home_usable", m_xyHomeUsable, "1 if X and Y can be homed");
		f.field("home_all_usable", m_homeAllUsable, "1 if all axes can be homed");
		f.field("disable_motors_usable", m_disableMotorsUsable, "1 if the motors can be disabled");
		f.list("axes", m_axes, "The axes other than X, Y and Z");
		f.field("axes_shown", m_axesShown, "1 if there are other axes");
		f.field("confirm", m_confirm, "1 while asking before homing or disabling the motors");
		f.field("confirm_title", m_confirmTitle, "The question's title");
		f.field("confirm_text", m_confirmText, "The question");
	}

	void MovePresenter::onInit()
	{
		m_watch = Xml::OmWatch({"move"}, [this] { showAxes(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { showAxes(); });
	}

	void MovePresenter::onActivate()
	{
		m_confirm.set(0);
		showAxes();
	}

	void MovePresenter::onDisconnect()
	{
		m_connected.set(0);
		m_axes.clear();
		showAxes();
	}

	void MovePresenter::showAxes()
	{
		ZoneScoped;
		MODEL_LOCK();
		const bool connected = OM::IsConnected();
		const bool homeable = connected && canHome();
		const bool machine = StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE);
		m_connected.set(connected ? 1 : 0);
		m_homeAllUsable.set(homeable ? 1 : 0);
		m_disableMotorsUsable.set(homeable ? 1 : 0);

		// X, Y and Z have controls of their own; disabled if the printer has no such axis
		NamedAxis* named[] = {&m_x, &m_y, &m_z};
		const char letters[] = {'X', 'Y', 'Z'};
		for (size_t i = 0; i < 3; i++)
		{
			const OM::Node& axis = Xml::PrinterModel::axis(letters[i]);
			const bool exists = axis.exists();
			named[i]->label.set(axisLabel(letters[i], exists ? positionOf(axis, machine) : 0.0f));
			named[i]->homed.set(exists && axis["homed"].boolean() ? 1 : 0);
			named[i]->jogUsable.set(connected && exists && canMove(axis) ? 1 : 0);
			named[i]->homeUsable.set(homeable && exists ? 1 : 0);
		}
		m_xyHomed.set(m_x.homed.get() && m_y.homed.get() ? 1 : 0);
		m_xyHomeUsable.set(m_x.homeUsable.get() && m_y.homeUsable.get() ? 1 : 0);

		// Any others the user sees
		std::vector<size_t> others;
		if (connected)
		{
			for (const size_t number : Xml::PrinterModel::visibleAxes())
			{
				const std::string_view letter = Xml::PrinterModel::root()["move"]["axes"][number]["letter"].text();
				if (!letter.empty() && letter.front() != 'X' && letter.front() != 'Y' && letter.front() != 'Z')
				{
					others.push_back(number);
				}
			}
		}
		m_axes.sync(others.size(),
					[&](MoveAxisRow& row, size_t i)
					{
						const OM::Node& axis = Xml::PrinterModel::root()["move"]["axes"][others[i]];
						const char letter = axis["letter"].text().front();
						row.letter.set(std::string(1, letter));
						row.label.set(axisLabel(letter, positionOf(axis, machine)));
						row.homed.set(axis["homed"].boolean() ? 1 : 0);
						row.jogUsable.set(canMove(axis) ? 1 : 0);
						row.homeUsable.set(homeable ? 1 : 0);
					});
		m_axesShown.set(others.empty() ? 0 : 1);
	}

	void MovePresenter::confirmThen(std::string title, std::string text, std::function<void()> action)
	{
		if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
		{
			action();
			return;
		}
		m_confirmTitle.set(title);
		m_confirmText.set(text);
		m_confirmed = std::move(action);
		m_confirm.set(1);
	}

	void MovePresenter::home(char letter)
	{
		{
			MODEL_LOCK();
			if (!Xml::PrinterModel::axis(letter).exists())
			{
				return;
			}
		}
		Xml::Services::sendGcode(fmt::format("G28 {:s}\n", Xml::PrinterModel::axisGcode(letter)));
	}

	void MovePresenter::jog(char letter, bool forward)
	{
		const auto distance = Xml::JogSource::distance();
		const auto feedrate = Xml::JogSource::feedrate();
		if (!distance || !feedrate)
		{
			return;
		}
		{
			MODEL_LOCK();
			if (!Xml::PrinterModel::axis(letter).exists())
			{
				return;
			}
		}
		// M120/M121 keep the printer's movement mode as it was
		Xml::Services::sendGcode(
			fmt::format("M120\nG91\nG1 {:s}{:g} F{:g}\nM121\n",
						Xml::PrinterModel::axisGcode(letter),
						Units::convertDisplayedDistanceToDuetUnits((forward ? 1.0f : -1.0f) * *distance),
						Units::convertDisplayedSpeedToDuetUnits(static_cast<float>(*feedrate))));
	}

	void MovePresenter::askPosition(char letter)
	{
		const auto feedrate = Xml::JogSource::feedrate();
		const bool machine = StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE);
		Xml::NumberRequest request;
		{
			MODEL_LOCK();
			const OM::Node& axis = Xml::PrinterModel::axis(letter);
			if (!axis.exists() || !feedrate)
			{
				return;
			}
			request = {.header = _("move.set_position", letter),
					   .min = static_cast<float>(axis["min"].number()),
					   .max = static_cast<float>(axis["max"].number()),
					   .value = Units::convertDistanceToCurrentDisplayedUnit(positionOf(axis, machine)),
					   .onConfirm = [letter, machine, feedrate](float value)
					   {
						   Xml::Services::sendGcode(
							   fmt::format("M120\nG90\n{:s}G1 {:s}{:g} F{:g}\nM121\n",
										   machine ? "G53 " : "",
										   Xml::PrinterModel::axisGcode(letter),
										   Units::convertDisplayedDistanceToDuetUnits(value),
										   Units::convertDisplayedSpeedToDuetUnits(static_cast<float>(*feedrate))));
					   }};
		}
		Xml::Services::askNumber(std::move(request));
	}

	char MovePresenter::axisOf(const Xml::ActionArgs& args)
	{
		if (const auto* row = args.row<MoveAxisRow>())
		{
			const auto letter = row->letter.get();
			return letter.empty() ? '\0' : letter.front();
		}
		return args.arg.empty() ? '\0' : args.arg.front();
	}
} // namespace UI

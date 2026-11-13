/*
 * MovePresenter.cpp
 */

#include "MovePresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
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
		bool canMove(const OM::Move::Axis& axis)
		{
			return (axis.homed || !OM::Move::GetNoMovesBeforeHoming()) && canHome();
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

	void MoveChoiceRow::fields(Xml::FieldVisitor& f)
	{
		f.field("text", text, "The value");
		f.field("selected", selected, "1 for the one in use");
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
		action("move.distance",
			   "Use that jog distance. From a row of `distances`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<MoveChoiceRow>())
				   {
					   m_distance = static_cast<size_t>(row->index.get());
					   showChoices();
				   }
			   });
		action("move.feedrate",
			   "Use that jog speed. From a row of `feedrates`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<MoveChoiceRow>())
				   {
					   m_feedrate = static_cast<size_t>(row->index.get());
					   showChoices();
				   }
			   });
		action("move.edit_distance",
			   "Ask for a new value for that jog distance with a number pad. From a row of `distances`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<MoveChoiceRow>();
				   if (row == nullptr)
				   {
					   return;
				   }
				   const auto i = static_cast<size_t>(row->index.get());
				   Xml::Services::askNumber(
					   {.header = _("move.distance_adjust_header", Units::getDisplayedDistanceUnit()),
						.min = 0.01f,
						.max = 1000.0f,
						.value = m_distances.at(i),
						.onConfirm = [this, i](float value)
						{
							m_distances.at(i) = value;
							StorageHelper::setData(ID_MOVE_DISTANCES, m_distances);
							showChoices();
						}});
			   });
		action("move.edit_feedrate",
			   "Ask for a new value for that jog speed with a number pad. From a row of `feedrates`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<MoveChoiceRow>();
				   if (row == nullptr)
				   {
					   return;
				   }
				   const auto i = static_cast<size_t>(row->index.get());
				   Xml::Services::askNumber({.header = _("move.feedrate_adjust_header", Units::getDisplayedSpeedUnit()),
											 .min = 0.01f,
											 .max = 1000.0f,
											 .value = static_cast<float>(m_feedrates.at(i)),
											 .onConfirm = [this, i](float value)
											 {
												 m_feedrates.at(i) = static_cast<uint32_t>(std::lround(value));
												 StorageHelper::setData(ID_MOVE_FEEDRATES, m_feedrates);
												 showChoices();
											 }});
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
		f.field("distance_title", m_distanceTitle, "Distance row title, e.g. \"Move Distance (mm)\"");
		f.field("feedrate_title", m_feedrateTitle, "Speed row title");
		f.list("distances", m_distanceChoices, "The jog distances to choose from");
		f.list("feedrates", m_feedrateChoices, "The jog speeds to choose from");
		f.field("confirm", m_confirm, "1 while asking before homing or disabling the motors");
		f.field("confirm_title", m_confirmTitle, "The question's title");
		f.field("confirm_text", m_confirmText, "The question");
	}

	void MovePresenter::onInit()
	{
		registerEventListener<EventType::AxesData>([this]() { showAxes(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { showAxes(); });

		if (auto distances = StorageHelper::getData(ID_MOVE_DISTANCES); !distances.empty())
		{
			m_distances = std::move(distances);
		}
		if (auto feedrates = StorageHelper::getData(ID_MOVE_FEEDRATES); !feedrates.empty())
		{
			m_feedrates = std::move(feedrates);
		}
	}

	void MovePresenter::onActivate()
	{
		m_confirm.set(0);
		showChoices();
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
			auto axis = OM::Move::GetAxisByLetter(letters[i]);
			const float position = axis ? (machine ? axis->machinePosition : axis->userPosition) : 0.0f;
			named[i]->label.set(axisLabel(letters[i], position));
			named[i]->homed.set(axis && axis->homed ? 1 : 0);
			named[i]->jogUsable.set(connected && axis && canMove(*axis) ? 1 : 0);
			named[i]->homeUsable.set(homeable && axis ? 1 : 0);
		}
		m_xyHomed.set(m_x.homed.get() && m_y.homed.get() ? 1 : 0);
		m_xyHomeUsable.set(m_x.homeUsable.get() && m_y.homeUsable.get() ? 1 : 0);

		// Any others
		std::vector<OM::Move::AxisPtr> others;
		if (connected)
		{
			for (const auto& axis : OM::Move::GetAxes(false))
			{
				if (axis->letter[0] != 'X' && axis->letter[0] != 'Y' && axis->letter[0] != 'Z')
				{
					others.push_back(axis);
				}
			}
		}
		m_axes.sync(others.size(),
					[&](MoveAxisRow& row, size_t i)
					{
						const auto& axis = *others[i];
						row.letter.set(std::string(1, axis.letter[0]));
						row.label.set(axisLabel(axis.letter[0], machine ? axis.machinePosition : axis.userPosition));
						row.homed.set(axis.homed ? 1 : 0);
						row.jogUsable.set(canMove(axis) ? 1 : 0);
						row.homeUsable.set(homeable ? 1 : 0);
					});
		m_axesShown.set(others.empty() ? 0 : 1);
	}

	void MovePresenter::showChoices()
	{
		m_distanceTitle.set(_("move.distance", Units::getDisplayedDistanceUnit()));
		m_feedrateTitle.set(_("move.feedrate", Units::getDisplayedSpeedUnit()));
		m_distance = std::min(m_distance, m_distances.empty() ? 0 : m_distances.size() - 1);
		m_feedrate = std::min(m_feedrate, m_feedrates.empty() ? 0 : m_feedrates.size() - 1);
		m_distanceChoices.sync(m_distances.size(),
							   [this](MoveChoiceRow& row, size_t i)
							   {
								   row.text.set(fmt::format("{}", m_distances[i]));
								   row.selected.set(i == m_distance ? 1 : 0);
							   });
		m_feedrateChoices.sync(m_feedrates.size(),
							   [this](MoveChoiceRow& row, size_t i)
							   {
								   row.text.set(fmt::format("{}", m_feedrates[i]));
								   row.selected.set(i == m_feedrate ? 1 : 0);
							   });
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
		MODEL_LOCK();
		if (auto axis = OM::Move::GetAxisByLetter(letter))
		{
			axis->Home();
		}
	}

	void MovePresenter::jog(char letter, bool forward)
	{
		if (m_distances.empty() || m_feedrates.empty())
		{
			return;
		}
		MODEL_LOCK();
		if (auto axis = OM::Move::GetAxisByLetter(letter))
		{
			axis->MoveRelative((forward ? 1.0f : -1.0f) * m_distances[m_distance], m_feedrates[m_feedrate]);
		}
	}

	void MovePresenter::askPosition(char letter)
	{
		MODEL_LOCK();
		auto axis = OM::Move::GetAxisByLetter(letter);
		if (axis == nullptr || m_feedrates.empty())
		{
			return;
		}
		const bool machine = StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE);
		Xml::Services::askNumber({.header = _("move.set_position", letter),
								  .min = axis->minPosition,
								  .max = axis->maxPosition,
								  .value = Units::convertDistanceToCurrentDisplayedUnit(machine ? axis->machinePosition
																								: axis->userPosition),
								  .onConfirm = [this, letter, machine](float value)
								  {
									  MODEL_LOCK();
									  if (auto axis = OM::Move::GetAxisByLetter(letter))
									  {
										  axis->MoveAbsolute(value, m_feedrates[m_feedrate], machine);
									  }
								  }});
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

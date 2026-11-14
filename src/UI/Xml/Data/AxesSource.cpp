/*
 * AxesSource.cpp
 */

#include "AxesSource.h"
#include "Debug.h"
#include "ObjectModel/PrinterStatus.h"
#include "PrinterModel.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <fmt/format.h>
#include <string>
#include <vector>

namespace UI::Xml
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

		/** Whether `axis` can be jogged: homed, or the printer moves axes that aren't, and not busy (model lock held)
		 */
		bool canMove(const OM::Node& axis)
		{
			return (axis["homed"].boolean() || !PrinterModel::noMovesBeforeHoming()) && canHome();
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

	void AxisRow::fields(FieldVisitor& f)
	{
		f.field("letter", letter, "Axis letter");
		f.field("label", label, "Position in the displayed unit, e.g. \"U: 0.00\"");
		f.field("homed", homed, "1 when homed");
		f.field("jog_usable", jogUsable, "1 if the axis can be jogged or set (homed, or moves before homing allowed)");
		f.field("home_usable", homeUsable, "1 if it can be homed");
	}

	AxesSource::AxesSource()
		: DataSource("axes")
	{
	}

	void AxesSource::fields(FieldVisitor& f)
	{
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
		f.list("others", m_others, "The axes the user sees other than X, Y and Z");
		f.field("others_shown", m_othersShown, "1 if there are other axes");
	}

	void AxesSource::onInit()
	{
		watchObjectModel({"move"});
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { update(); });
	}

	void AxesSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		const bool connected = OM::IsConnected();
		const bool homeable = connected && canHome();
		const bool machine = StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE);
		m_homeAllUsable.set(homeable ? 1 : 0);
		m_disableMotorsUsable.set(homeable ? 1 : 0);

		// X, Y and Z have controls of their own; disabled if the printer has no such axis
		NamedAxis* named[] = {&m_x, &m_y, &m_z};
		const char letters[] = {'X', 'Y', 'Z'};
		for (size_t i = 0; i < 3; i++)
		{
			const OM::Node& axis = PrinterModel::axis(letters[i]);
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
			for (const size_t number : PrinterModel::visibleAxes())
			{
				const std::string_view letter = PrinterModel::root()["move"]["axes"][number]["letter"].text();
				if (!letter.empty() && letter.front() != 'X' && letter.front() != 'Y' && letter.front() != 'Z')
				{
					others.push_back(number);
				}
			}
		}
		m_others.sync(others.size(),
					  [&](AxisRow& row, size_t i)
					  {
						  const OM::Node& axis = PrinterModel::root()["move"]["axes"][others[i]];
						  const char letter = axis["letter"].text().front();
						  row.letter.set(std::string(1, letter));
						  row.label.set(axisLabel(letter, positionOf(axis, machine)));
						  row.homed.set(axis["homed"].boolean() ? 1 : 0);
						  row.jogUsable.set(canMove(axis) ? 1 : 0);
						  row.homeUsable.set(homeable ? 1 : 0);
					  });
		m_othersShown.set(others.empty() ? 0 : 1);
	}

	void AxesSource::clear()
	{
		m_others.clear();
		update();
	}
} // namespace UI::Xml

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
#include <algorithm>
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

		/** Whether the axis called `letter` turns rather than slides: A, B and C, as RepRapFirmware has them */
		bool isRotary(char letter)
		{
			return letter == 'A' || letter == 'B' || letter == 'C';
		}

		/** An axis' position as shown: in the displayed unit, e.g. "150.00", or in degrees, e.g. "90.00°" */
		std::string positionText(char letter, float position)
		{
			return isRotary(letter) ? fmt::format("{:.2f}°", position)
									: fmt::format("{:.2f}", Units::convertDistanceToCurrentDisplayedUnit(position));
		}
	} // namespace

	void AxisRow::fields(FieldVisitor& f)
	{
		f.field("letter", letter, "Axis letter, e.g. \"U\"");
		f.field("position",
				position,
				"Position in the displayed unit, e.g. \"150.00\", or in degrees for a rotary axis (A, B, C), e.g. "
				"\"90.00°\"");
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
		const std::pair<const char*, NamedAxis*> named[] = {{"x", &m_x}, {"y", &m_y}};
		for (const auto& [prefix, axis] : named)
		{
			f.field(fmt::format("{}.homed", prefix), axis->homed, "1 when homed");
			f.field(fmt::format("{}.jog_usable", prefix),
					axis->jogUsable,
					"1 if the axis can be jogged or set (homed, or moves before homing allowed)");
		}
		f.list("all", m_all, "Every axis the user sees, in the printer's order");
		f.field("all_homed", m_allHomed, "1 when every axis the user sees is homed");
		f.field("home_all_usable", m_homeAllUsable, "1 if all axes can be homed");
		f.field("disable_motors_usable", m_disableMotorsUsable, "1 if the motors can be disabled");
		f.list("columns",
			   m_columns,
			   "The axes the user sees other than X and Y, each jogged in a column of its own: Z, then any others");
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

		// X and Y have controls of their own, the XY pad; disabled if the printer has no such axis
		NamedAxis* named[] = {&m_x, &m_y};
		const char letters[] = {'X', 'Y'};
		for (size_t i = 0; i < 2; i++)
		{
			const OM::Node& axis = PrinterModel::axis(letters[i]);
			const bool exists = axis.exists();
			named[i]->homed.set(exists && axis["homed"].boolean() ? 1 : 0);
			named[i]->jogUsable.set(connected && exists && canMove(axis) ? 1 : 0);
		}

		// Every axis the user sees, and those with a column of their own: all but X and Y
		std::vector<size_t> all;
		std::vector<size_t> columns;
		if (connected)
		{
			for (const size_t number : PrinterModel::visibleAxes())
			{
				const std::string_view letter = PrinterModel::root()["move"]["axes"][number]["letter"].text();
				if (letter.empty())
				{
					continue;
				}
				all.push_back(number);
				if (letter.front() != 'X' && letter.front() != 'Y')
				{
					columns.push_back(number);
				}
			}
		}
		const auto show = [&](AxisRow& row, size_t number)
		{
			const OM::Node& axis = PrinterModel::root()["move"]["axes"][number];
			const char letter = axis["letter"].text().front();
			row.letter.set(std::string(1, letter));
			row.position.set(positionText(letter, positionOf(axis, machine)));
			row.homed.set(axis["homed"].boolean() ? 1 : 0);
			row.jogUsable.set(canMove(axis) ? 1 : 0);
			row.homeUsable.set(homeable ? 1 : 0);
		};
		m_all.sync(all.size(), [&](AxisRow& row, size_t i) { show(row, all[i]); });
		m_columns.sync(columns.size(), [&](AxisRow& row, size_t i) { show(row, columns[i]); });
		m_allHomed.set(!all.empty() &&
							   std::all_of(all.begin(),
										   all.end(),
										   [](size_t number)
										   { return PrinterModel::root()["move"]["axes"][number]["homed"].boolean(); })
						   ? 1
						   : 0);
	}

	void AxesSource::clear()
	{
		m_all.clear();
		m_columns.clear();
		update();
	}
} // namespace UI::Xml

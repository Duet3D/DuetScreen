/*
 * AxesSource.cpp
 */

#include "AxesSource.h"
#include "DataSources.h"
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

		/** Whether `axis` is at its end the `forward` (else backward) way, and the printer keeps it there: limits are
		 * on (M564 S1), it is homed (they apply only then) and its machine position is at its maximum (or minimum).
		 * Model lock held. */
		bool atLimit(const OM::Node& axis, bool forward)
		{
			if (!PrinterModel::limitAxes() || !axis["homed"].boolean())
			{
				return false;
			}
			constexpr double TOLERANCE = 0.001; // Positions are reported to a thousandth
			const double position = axis["machinePosition"].number();
			return forward ? position >= axis["max"].number() - TOLERANCE
						   : position <= axis["min"].number() + TOLERANCE;
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
		f.field("chosen", chosen, "In `source.axes.others`: 1 for the axis chosen to be jogged, else 0");
	}

	void AxesSource::NamedAxis::show(const OM::Node& axis, bool connected)
	{
		const bool exists = axis.exists();
		const bool usable = connected && exists && canMove(axis);
		homed.set(exists && axis["homed"].boolean() ? 1 : 0);
		jogUsable.set(usable ? 1 : 0);
		plusUsable.set(usable && !atLimit(axis, true) ? 1 : 0);
		minusUsable.set(usable && !atLimit(axis, false) ? 1 : 0);
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
			namedFields(f, prefix, *axis);
		}
		f.list("all", m_all, "Every axis the user sees, in the printer's order");
		f.field("all_homed", m_allHomed, "1 when every axis the user sees is homed");
		f.field("home_all_usable", m_homeAllUsable, "1 if all axes can be homed");
		f.field("disable_motors_usable", m_disableMotorsUsable, "1 if the motors can be disabled");
		f.list("others", m_others, "The axes the user sees other than X, Y and Z: one is chosen to be jogged");
		f.field("others_shown", m_othersShown, "1 if there are axes other than X, Y and Z");
		f.field("other.letter", m_otherLetter, "The letter of the axis chosen from `others`, e.g. \"U\"");
		namedFields(f, "other", m_other);
	}

	void AxesSource::namedFields(FieldVisitor& f, const char* prefix, NamedAxis& axis)
	{
		f.field(fmt::format("{}.homed", prefix), axis.homed, "1 when homed");
		f.field(fmt::format("{}.jog_usable", prefix),
				axis.jogUsable,
				"1 if the axis can be jogged or set (homed, or moves before homing allowed)");
		f.field(fmt::format("{}.plus_usable", prefix),
				axis.plusUsable,
				"1 if the axis can be jogged forwards: as jog_usable, but 0 at its maximum while the printer keeps "
				"axes within their limits (M564 S1)");
		f.field(fmt::format("{}.minus_usable", prefix),
				axis.minusUsable,
				"1 if the axis can be jogged backwards: as jog_usable, but 0 at its minimum while the printer keeps "
				"axes within their limits (M564 S1)");
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
			named[i]->show(PrinterModel::axis(letters[i]), connected);
		}

		// Every axis the user sees, and those other than X, Y and Z
		std::vector<size_t> all;
		std::vector<size_t> others;
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
				if (letter.front() != 'X' && letter.front() != 'Y' && letter.front() != 'Z')
				{
					others.push_back(number);
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

		// One of the others is jogged: the one chosen, or the first while the chosen one isn't there
		const auto letterOf = [](size_t number)
		{ return PrinterModel::root()["move"]["axes"][number]["letter"].text().front(); };
		const auto chosen =
			std::find_if(others.begin(), others.end(), [&](size_t number) { return letterOf(number) == m_chosen; });
		if (chosen == others.end())
		{
			m_chosen = others.empty() ? '\0' : letterOf(others.front());
		}
		m_others.sync(others.size(),
					  [&](AxisRow& row, size_t i)
					  {
						  show(row, others[i]);
						  row.chosen.set(letterOf(others[i]) == m_chosen ? 1 : 0);
					  });
		m_othersShown.set(others.empty() ? 0 : 1);
		m_otherLetter.set(m_chosen != '\0' ? std::string(1, m_chosen) : std::string());
		m_other.show(PrinterModel::axis(m_chosen), connected); // Doesn't exist while none is chosen
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
		m_others.clear();
		update();
	}
	std::optional<char> AxesSource::chosenAxis()
	{
		const auto* source = DataSources::find<AxesSource>();
		if (source == nullptr || source->m_chosen == '\0')
		{
			return std::nullopt;
		}
		return source->m_chosen;
	}

	void AxesSource::choose(char letter)
	{
		auto* source = DataSources::find<AxesSource>();
		if (source != nullptr && letter != '\0')
		{
			source->m_chosen = letter;
			source->update();
		}
	}
} // namespace UI::Xml

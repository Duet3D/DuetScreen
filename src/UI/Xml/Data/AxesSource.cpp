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

	AxesSource::AxesSource()
		: DataSource("axes")
	{
	}

	void AxesSource::fields(FieldVisitor& f)
	{
		const std::pair<const char*, NamedAxis*> named[] = {{"x", &m_x}, {"y", &m_y}, {"z", &m_z}};
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
		f.list("others", m_others, "The axes the user sees other than X, Y and Z: one is chosen to be jogged");
		f.field("others_shown", m_othersShown, "1 if there are axes other than X, Y and Z");
		f.field("other.letter", m_otherLetter, "The letter of the axis chosen from `others`, e.g. \"U\"");
		f.field("other.homed", m_otherHomed, "1 when the chosen axis is homed");
		f.field("other.jog_usable", m_otherJogUsable, "1 if the chosen axis can be jogged");
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
			named[i]->homed.set(exists && axis["homed"].boolean() ? 1 : 0);
			named[i]->jogUsable.set(connected && exists && canMove(axis) ? 1 : 0);
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
		const OM::Node& other = PrinterModel::axis(m_chosen); // Doesn't exist while none is chosen
		m_otherLetter.set(m_chosen != '\0' ? std::string(1, m_chosen) : std::string());
		m_otherHomed.set(other.exists() && other["homed"].boolean() ? 1 : 0);
		m_otherJogUsable.set(other.exists() && canMove(other) ? 1 : 0);
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

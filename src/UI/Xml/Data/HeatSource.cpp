/*
 * HeatSource.cpp
 */

#include "HeatSource.h"
#include "Debug.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/**
		 * Show a bed or chamber in `row`
		 * @param kind Translated "Bed" or "Chamber"; the number is added when there are several
		 */
		void fill(BedOrChamberRow& row, const PrinterModel::HeatedPlace& place, const std::string& kind, size_t count)
		{
			const OM::Node& heater = PrinterModel::heater(place.heater);
			const std::string name = count > 1 ? fmt::format("{:s} {:d}", kind, place.number) : kind;
			const std::string state = PrinterModel::heaterState(place.heater);
			row.number.set(static_cast<int32_t>(place.number));
			row.update({.heater = static_cast<int32_t>(place.heater),
						.name = name,
						.state = state,
						.current = static_cast<float>(heater["current"].number()),
						.active = static_cast<int32_t>(heater["active"].integer(0)),
						.standby = static_cast<int32_t>(heater["standby"].integer(0)),
						.min = static_cast<float>(heater["min"].number()),
						.max = static_cast<float>(heater["max"].number())});
		}
	} // namespace

	void BedOrChamberRow::fields(FieldVisitor& f)
	{
		f.field("number", number, "Bed or chamber number, as used by M140/M141 P");
		HeaterRow::fields(f);
	}

	HeatSource::HeatSource()
		: DataSource("heat")
	{
	}

	void HeatSource::fields(FieldVisitor& f)
	{
		f.list("beds", m_beds, "One row per bed, in slot order");
		f.list("chambers", m_chambers, "One row per chamber, in slot order");
	}

	void HeatSource::onInit()
	{
		watchObjectModel({"heat", "sensors.analog"});
	}

	void HeatSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto beds = PrinterModel::beds();
		const std::string bed = _("temperature.bed");
		m_beds.sync(beds.size(), [&](BedRow& row, size_t slot) { fill(row, beds[slot], bed, beds.size()); });

		const auto chambers = PrinterModel::chambers();
		const std::string chamber = _("temperature.chamber");
		m_chambers.sync(chambers.size(),
						[&](ChamberRow& row, size_t slot) { fill(row, chambers[slot], chamber, chambers.size()); });
	}

	void HeatSource::clear()
	{
		m_beds.clear();
		m_chambers.clear();
	}
} // namespace UI::Xml

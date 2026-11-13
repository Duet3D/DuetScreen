/*
 * HeatSource.cpp
 */

#include "HeatSource.h"
#include "Debug.h"
#include "ObjectModel/BedOrChamber.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/**
		 * Copy a bed or chamber into `row`
		 * @param kind Translated "Bed" or "Chamber"; the number is added when there are several
		 */
		void fill(BedOrChamberRow& row, OM::BedOrChamber& item, const std::string& kind, size_t count)
		{
			const std::string name = count > 1 ? fmt::format("{:s} {:d}", kind, item.index) : kind;
			row.number.set(static_cast<int32_t>(item.index));
			row.update({.heater = item.heater,
						.name = name,
						.state = item.GetHeaterStatusStr(),
						.current = item.GetCurrentTemp(),
						.active = item.GetActiveTemp(),
						.standby = item.GetStandbyTemp(),
						.min = item.GetMin(),
						.max = item.GetMax()});
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
		registerEventListener<EventType::BedHeaterData>(this, &HeatSource::update);
		registerEventListener<EventType::ChamberHeaterData>(this, &HeatSource::update);
		registerEventListener<EventType::HeaterData>(this, &HeatSource::update);
	}

	void HeatSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		const size_t beds = OM::GetBedCount();
		const std::string bed = _("temperature.bed");
		m_beds.sync(beds,
					[&](BedRow& row, size_t slot)
					{
						if (auto item = OM::GetBedBySlot(slot))
						{
							fill(row, *item, bed, beds);
						}
					});

		const size_t chambers = OM::GetChamberCount();
		const std::string chamber = _("temperature.chamber");
		m_chambers.sync(chambers,
						[&](ChamberRow& row, size_t slot)
						{
							if (auto item = OM::GetChamberBySlot(slot))
							{
								fill(row, *item, chamber, chambers);
							}
						});
	}

	void HeatSource::clear()
	{
		m_beds.clear();
		m_chambers.clear();
	}
} // namespace UI::Xml

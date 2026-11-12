/*
 * HeatSource.h
 *
 * Publishes the object model's beds and chambers.
 *
 *   beds              list, one row per bed in slot order
 *     index           position in the list
 *     number          bed number, as used by M140 P
 *     (heater fields: see HeaterRow.h; `name` is "Bed", or "Bed 1" when there are several)
 *   chambers          list, one row per chamber, as for beds (M141 P)
 */

#pragma once

#include "DataSource.h"
#include "HeaterRow.h"

namespace UI::Xml
{
	/** A bed or a chamber */
	class BedOrChamberRow : public HeaterRow
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		IntSubject number; // Bed or chamber number
	};

	/** One bed */
	class BedRow : public BedOrChamberRow
	{
	};

	/** One chamber */
	class ChamberRow : public BedOrChamberRow
	{
	};

	/** Publishes the `beds` and `chambers` lists */
	class HeatSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		HeatSource();

		/** Declare the `beds` and `chambers` lists */
		void fields(FieldVisitor& f) override;

		/** Re-read every bed and chamber */
		void update() override;

		/** Empty the lists */
		void clear() override;

	  protected:
		/** Listen for bed, chamber and heater changes */
		void onInit() override;

	  private:
		List<BedRow> m_beds;		 // One row per bed
		List<ChamberRow> m_chambers; // One row per chamber
	};
} // namespace UI::Xml

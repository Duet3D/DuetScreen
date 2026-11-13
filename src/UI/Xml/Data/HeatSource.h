/*
 * HeatSource.h
 *
 * Publishes the object model's beds and chambers. Each row is a HeaterRow (see HeaterRow.h) with a number.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
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

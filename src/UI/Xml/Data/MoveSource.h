/*
 * MoveSource.h
 *
 * Publishes the printer's motion: each axis' position, the speed and extrusion overrides and the Z babystep.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** One axis */
	class AxisRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<4> letter;	// e.g. "X"
		StringSubject<16> position; // User position, e.g. "12.50"
		IntSubject homed;			// 1 when homed
		IntSubject visible;			// 1 if the axis is shown to the user
	};

	/** Publishes `move.*` */
	class MoveSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		MoveSource();

		/** Declare `move.*` */
		void fields(FieldVisitor& f) override;

		/** Re-read everything */
		void update() override;

		/** Empty the list and reset the overrides */
		void clear() override;

		/**
		 * The extrusion override, percent: the average of the current tool's extruders, or of every extruder if there
		 * is no current tool; 100 if there are none
		 */
		static uint32_t extrusionFactor();

	  protected:
		/** Listen for axis, extruder and override changes */
		void onInit() override;

	  private:
		/** Show every axis and the babystep */
		void showAxes();

		/** Show the speed override */
		void showSpeedFactor();

		/** Show the extrusion override */
		void showExtrusionFactor();

		List<AxisRow> m_axes;					 // One row per axis
		StringSubject<16> m_speedFactor;		 // e.g. "100%"
		StringSubject<16> m_extrusionFactor;	 // e.g. "100%"
		StringSubject<24> m_babystep;			 // e.g. "0 mm"
	};
} // namespace UI::Xml

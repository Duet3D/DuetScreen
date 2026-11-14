/*
 * MoveSource.h
 *
 * Publishes the printer's motion: each axis' position, the speed and extrusion overrides, the Z babystep, and the
 * current move's speed and volumetric flow, also as gauges.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "ObjectModel/Tree/Node.h"
#include "UI/Xml/Binding/Subjects.h"
#include <vector>

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

		/** The speed override, as a fraction (1 is 100%) */
		static float speedFactor();

	  protected:
		/** Follow the move, the tools and the current tool */
		void onInit() override;

	  private:
		/** The object model's move (the model lock held) */
		static const OM::Node& move();

		/** The current tool's extruders, or every extruder if there is no current tool (the model lock held) */
		static std::vector<size_t> extruders();

		/** The current move's volumetric flow, mm³/s: the current tool's filament cross section by its extrusion
		 * rate; 0 with no current tool */
		static float volumetricFlow();

		/** Show every axis and the babystep */
		void showAxes();

		/** Show the speed override */
		void showSpeedFactor();

		/** Show the extrusion override, and find the flow gauge's range */
		void showExtrusionFactor();

		/** Show the current move's speeds */
		void showSpeeds();

		/** Show the volumetric flow */
		void showFlow();

		int32_t m_maxSpeed = 1;			// Fastest axis' maximum speed, mm/s: the top of the speed gauges
		int32_t m_maxExtrusionRate = 1; // The current tool's fastest extruder's maximum: the top of the flow gauge

		List<AxisRow> m_axes;					 // One row per axis
		StringSubject<16> m_speedFactor;		 // e.g. "100%"
		StringSubject<16> m_extrusionFactor;	 // e.g. "100%"
		StringSubject<24> m_babystep;			 // e.g. "0 mm"
		StringSubject<32> m_speedText;			 // Current and requested speed, e.g. "0/0 mm/s"
		IntSubject m_speedTop;					 // Top speed, thousandths of the fastest axis' maximum
		IntSubject m_speedRequested;			 // Requested speed, thousandths of the fastest axis' maximum
		StringSubject<32> m_flowText;			 // Volumetric flow, e.g. "0.0 mm³/s"
		IntSubject m_flow;						 // Volumetric flow, thousandths of the fastest extruder's maximum
	};
} // namespace UI::Xml

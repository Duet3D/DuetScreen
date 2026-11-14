/*
 * JogSource.h
 *
 * Publishes how far and how fast a jog goes, so every view that jogs uses the same: the Move tab, and any jog widgets
 * placed on the dashboard. Each is a value selector (UI/Xml/View/ValueSelector.h) whose presets are settings; which is
 * chosen lasts until the screen restarts. The global actions `actions.selector.*` choose them.
 *
 * Each has presets in millimetres and in inches, as the screen shows lengths. A printer jogs in mm/s; a CNC machine,
 * whose feeds are written per minute, in mm/min with presets of its own. In inches both are in inches per minute.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/View/ValueSelector.h"
#include <optional>

namespace UI::Xml
{
	/** Publishes `source.jog.*` */
	class JogSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		JogSource();

		/** Declare `source.jog.distance.*` and `source.jog.feedrate.*` */
		void fields(FieldVisitor& f) override;

		/** Use the presets for the units in use and the machine, and show their titles */
		void update() override;

		/** Nothing to clear: the choices don't depend on the printer */
		void clear() override {}

		/** The chosen jog distance, in the displayed unit; nullopt while the data sources aren't running */
		static std::optional<float> distance();

		/** The chosen jog speed as a G-code F value, in the Duet's distance unit per minute; nullopt while the data
		 * sources aren't running */
		static std::optional<float> feedrateGcode();

	  protected:
		/** Follow the machine's mode, which decides the jog speeds */
		void onInit() override;

	  private:
		bool m_perMinute = false; // The jog speed is per minute (a CNC machine in millimetres), not mm/s
		ValueSelector m_distance; // `distance.*`
		ValueSelector m_feedrate; // `feedrate.*`
	};
} // namespace UI::Xml

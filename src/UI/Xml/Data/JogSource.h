/*
 * JogSource.h
 *
 * Publishes how far and how fast a jog goes, so every view that jogs uses the same: the Move tab, and any jog widgets
 * placed on the dashboard. Each is a value selector (UI/Xml/View/ValueSelector.h) whose presets are settings; which is
 * chosen lasts until the screen restarts. The global actions `actions.selector.*` choose them.
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

		/** Show the titles in the units in use */
		void update() override;

		/** Nothing to clear: the choices don't depend on the printer */
		void clear() override {}

		/** The chosen jog distance, in the displayed unit; nullopt while the data sources aren't running */
		static std::optional<float> distance();

		/** The chosen jog speed, in the displayed unit; nullopt while the data sources aren't running */
		static std::optional<float> feedrate();

	  private:
		ValueSelector m_distance; // `distance.*`
		ValueSelector m_feedrate; // `feedrate.*`
	};
} // namespace UI::Xml

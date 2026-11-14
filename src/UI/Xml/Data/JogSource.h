/*
 * JogSource.h
 *
 * Publishes the jog distances and speeds the user chooses from, and which are chosen, so every view that jogs uses the
 * same: the Move tab, and any jog widgets placed on the dashboard. The choices themselves are settings
 * (ID_MOVE_DISTANCES, ID_MOVE_FEEDRATES); which is chosen lasts until the screen restarts. The actions that change
 * them are in UI/Presenters/Control/Move/JogActions.h.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace UI::Xml
{
	/** One jog distance or speed */
	class JogChoiceRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> text; // E.g. "0.1"
		IntSubject selected;	// 1 for the one in use
	};

	/** Publishes `source.jog.*` */
	class JogSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		JogSource();

		/** Declare the jog names */
		void fields(FieldVisitor& f) override;

		/** Show the choices, with the chosen ones selected */
		void update() override;

		/** Nothing to clear: the choices don't depend on the printer */
		void clear() override {}

		/** The chosen jog distance, in mm; nullopt if there are none to choose from */
		static std::optional<float> distance();

		/** The chosen jog speed, in mm/min; nullopt if there are none to choose from */
		static std::optional<uint32_t> feedrate();

		/** Choose the distance at `index` */
		static void chooseDistance(size_t index);

		/** Choose the speed at `index` */
		static void chooseFeedrate(size_t index);

		/** Make the distance at `index` `value`, and keep it in the settings */
		static void setDistance(size_t index, float value);

		/** Make the speed at `index` `value`, and keep it in the settings */
		static void setFeedrate(size_t index, uint32_t value);

	  protected:
		/** Listen for the choice changing */
		void onInit() override;

	  private:
		StringSubject<48> m_distanceTitle;	  // Distance row title
		StringSubject<48> m_feedrateTitle;	  // Speed row title
		List<JogChoiceRow> m_distances;		  // Jog distances
		List<JogChoiceRow> m_feedrates;		  // Jog speeds
	};
} // namespace UI::Xml

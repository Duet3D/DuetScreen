/*
 * ExtruderSource.h
 *
 * Publishes the extruder controls every view shares: how far and how fast to extrude, each a value selector
 * (UI/Xml/View/ValueSelector.h) kept in the settings, and whether the current tool can extrude or retract now. The
 * global actions `actions.extruder.extrude` and `actions.extruder.retract` use them
 * (src/UI/Presenters/Control/Temperature/ExtruderActions.h); `actions.selector.*` change the selectors.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/ValueSelector.h"

namespace UI::Xml
{
	/** Publishes `source.extruder.*` */
	class ExtruderSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ExtruderSource();

		/** Declare `source.extruder.*` */
		void fields(FieldVisitor& f) override;

		/** Show what the current tool can do now, and the titles in the units in use */
		void update() override;

		/** Nothing can be extruded while disconnected */
		void clear() override;

		/** Extrude (`direction` 1) or retract (-1) the chosen distance at the chosen rate with the current tool */
		void extrude(int direction) const;

	  protected:
		/** Follow the tools, their heaters and the printer's state */
		void onInit() override;

	  private:
		ValueSelector m_distance;		// `distance.*`
		ValueSelector m_feedrate;		// `feedrate.*`
		IntSubject m_extrudeUsable;		// `extrude_usable`
		IntSubject m_retractUsable;		// `retract_usable`
	};
} // namespace UI::Xml

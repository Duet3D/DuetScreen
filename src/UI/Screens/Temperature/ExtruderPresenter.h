/*
 * ExtruderPresenter.h
 *
 * Behaviour for the extruder panel (assets/ui/components/extruder.xml): extrude or retract with the selected tool,
 * by a distance and at a speed chosen with two value selectors (src/UI/Xml/View/ValueSelector.h).
 *
 * Fields
 *   extrude_usable     1 if the selected tool can extrude: there is one, it is hot enough, and no job is running
 *   retract_usable     1 if it can retract, likewise
 *   distance.*         the feed distance selector: title, value, choices
 *   feedrate.*         the feed rate selector: title, value, choices
 *
 * Actions
 *   extruder.extrude   extrude by the chosen distance at the chosen speed
 *   extruder.retract   retract likewise
 *   selector.*         change the distance or speed (see ValueSelector.h)
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/ValueSelector.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class ExtruderPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		ExtruderPresenter();

		/** Declare the panel's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for the tools, their heaters and the printer's state */
		void onInit() override;

		/** Show what can be done now */
		void onActivate() override;

		/** Nothing can be done */
		void onDisconnect() override;

	  private:
		/** Work out whether the selected tool can extrude and retract */
		void update();

		Xml::IntSubject m_extrudeUsable;	// 1 if the tool can extrude
		Xml::IntSubject m_retractUsable;	// 1 if the tool can retract
		Xml::ValueSelector m_distance;		// Feed distance
		Xml::ValueSelector m_feedrate;		// Feed rate
	};
} // namespace UI

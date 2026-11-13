/*
 * ExtruderPresenter.h
 *
 * Behaviour for the extruder panel (assets/ui/components/control/temperature/extruder.xml): extrude or retract with the
 * selected tool, by a distance and at a speed chosen with two value selectors (src/UI/Xml/View/ValueSelector.h).
 *
 * Its names and actions, including those of its two ValueSelectors, are declared in fields() and its constructor,
 * and listed in the UI API reference (docs/XML_UI_API.md).
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

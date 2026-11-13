/*
 * BabyStepPresenter.h
 *
 * Behaviour for the babystepping panel (assets/ui/components/babystep.xml): nudge Z by a chosen step while printing,
 * or reset it.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <vector>

namespace UI
{
	/** One step size */
	class BabyStepRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<24> text; // E.g. "0.01 mm"
		Xml::IntSubject selected;	 // 1 for the step in use
	};

	class BabyStepPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		BabyStepPresenter();

		/** Declare the panel's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for the axes */
		void onInit() override;

		/** Show the remembered steps and the current offset */
		void onActivate() override;

		/** Disable the panel */
		void onDisconnect() override;

	  private:
		/** Show the current offset, and whether babystepping is possible */
		void showOffset();

		/** Show the steps, with the chosen one selected */
		void showSteps();

		std::vector<float> m_steps; // Step sizes, mm
		size_t m_selected = 0;		// Index of the step in use

		Xml::IntSubject m_usable;			  // 1 if babystepping is possible
		Xml::StringSubject<32> m_resetText;	  // Reset button text
		Xml::List<BabyStepRow> m_stepRows;	  // Step sizes
	};
} // namespace UI

/*
 * FactorsPresenter.h
 *
 * Behaviour for the shared speed and extrusion factor dialogs (assets/ui/components/modals/factors.xml), which any
 * view opens with the global actions `actions.factors.speed` and `actions.factors.extrusion` (FactorsActions.cpp).
 *
 * Its names and actions, including those of its FactorDialogs, are declared in fields() and its constructor, and
 * listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "FactorDialog.h"
#include "UI/Xml/Om/OmWatch.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <optional>
#include <vector>

namespace UI
{
	class FactorsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with both dialogs closed */
		FactorsPresenter();

		/** Declare the dialogs' fields */
		void fields(Xml::FieldVisitor& f) override;

		/** The shared dialogs' presenter: the view called `factors`'s; nullptr, with a warning, if there is none */
		static FactorsPresenter* shared();

		/** Open the speed factor dialog */
		void openSpeed();

		/** Open the extrusion factor dialog for the extruders of the tool numbered `tool`, or every extruder if none */
		void openExtrusion(std::optional<size_t> tool);

	  protected:
		/** Listen for the speed and extrusion factors */
		void onInit() override;

	  private:
		/** Show the speed factor */
		void showSpeed();

		/** Show the extrusion factors of the extruders the dialog is for */
		void showExtrusion();

		/** The extruders the extrusion dialog is for, by number: the tool's, or every extruder (the model lock held) */
		std::vector<size_t> extruders() const;

		std::optional<size_t> m_tool; // Number of the tool the extrusion dialog is for; none for every extruder
		FactorDialog m_speed;		  // `speed_dialog.*`
		FactorDialog m_extrusion;	  // `extrusion_dialog.*`
		Xml::OmWatch m_watch;		  // Shows the factors again as they change
	};
} // namespace UI

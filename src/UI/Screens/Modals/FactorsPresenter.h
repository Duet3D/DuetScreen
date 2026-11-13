/*
 * FactorsPresenter.h
 *
 * Behaviour for the shared speed and extrusion factor dialogs (assets/ui/components/modals/factors.xml), which any
 * view opens with the global actions `factors.speed` and `factors.extrusion` (registerActions()).
 *
 * Its names and actions, including those of its FactorDialogs, are declared in fields() and its constructor, and
 * listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "FactorDialog.h"
#include "ObjectModel/Tool.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <optional>

namespace UI
{
	class FactorsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with both dialogs closed */
		FactorsPresenter();

		/** Declare the dialogs' fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Register `factors.speed` and `factors.extrusion` as global actions, for every view */
		static void registerActions();

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

		/** The extruder in row `index` of the extrusion dialog, or nullptr */
		OM::Move::ExtruderAxisPtr extruderAt(size_t index) const;

		/** The extruders the extrusion dialog is for */
		size_t extruderCount() const;

		std::optional<size_t> m_tool; // Number of the tool the extrusion dialog is for; none for every extruder
		FactorDialog m_speed;		  // `speed_dialog.*`
		FactorDialog m_extrusion;	  // `extrusion_dialog.*`
	};
} // namespace UI

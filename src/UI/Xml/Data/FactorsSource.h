/*
 * FactorsSource.h
 *
 * Publishes the shared speed and extrusion factor dialogs (assets/ui/components/modals/factors.xml, hosted once by
 * the home screen): whether each is open, its title and its percentages. Any view opens them with the global actions
 * `actions.factors.speed` and `actions.factors.extrusion`, and their sliders set the factors with
 * `actions.factors.set` (src/UI/Presenters/Modals/FactorsActions.h). The dialogs close themselves
 * (src/UI/Xml/Dialogs/DsDialog.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/View/FactorDialog.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace UI::Xml
{
	/** Publishes `source.factors.*` */
	class FactorsSource : public DataSource
	{
	  public:
		/** Create the source, with both dialogs closed. Nothing is published until DataSources::start(). */
		FactorsSource();

		/** Declare `source.factors.speed.*` and `source.factors.extrusion.*` */
		void fields(FieldVisitor& f) override;

		/** Show the factors as they are now */
		void update() override;

		/** Close the dialogs and forget the factors */
		void clear() override;

		/** Open the speed factor dialog */
		void openSpeed();

		/** Open the extrusion factor dialog for the extruders of the tool numbered `tool`, or every extruder if none */
		void openExtrusion(std::optional<size_t> tool);

		/** Show every extruder in the extrusion dialog, or only those of the tool it was opened for */
		void showAllExtruders(bool all);

	  protected:
		/** Follow the speed and extrusion factors */
		void onInit() override;

	  private:
		/** Show the speed factor */
		void showSpeed();

		/** Show the extrusion factors of the extruders the dialog is for */
		void showExtrusion();

		/** The extruders the extrusion dialog shows, by number: the tool's, or every extruder (the model lock held) */
		std::vector<size_t> extruders() const;

		/** Every extruder, by number, if `every`, else those of the tool the dialog is for if any (the model lock
		 * held) */
		std::vector<size_t> extruders(bool every) const;

		/** The names of the tools that use extruder `extruder`, e.g. "Revo, Tool 2" (the model lock held) */
		static std::string toolsUsing(size_t extruder);

		std::optional<size_t> m_tool; // Number of the tool the extrusion dialog is for; none for every extruder
		FactorDialog m_speed;		  // `speed.*`
		FactorDialog m_extrusion;	  // `extrusion.*`
		IntSubject m_all;			  // `extrusion_all`: the extrusion dialog shows every extruder
		IntSubject m_allShown;		  // `extrusion_all_shown`: it was opened for a tool that doesn't use them all
	};
} // namespace UI::Xml

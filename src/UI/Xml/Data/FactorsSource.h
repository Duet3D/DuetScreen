/*
 * FactorsSource.h
 *
 * Publishes the shared speed and extrusion factor dialogs (assets/ui/components/modals/factors.xml, hosted once by
 * the home screen): whether each is open, its title and its percentages. Any view opens them with the global actions
 * `actions.factors.speed` and `actions.factors.extrusion`, and the dialogs change them with `actions.factors.set` and
 * `actions.factors.close` (src/UI/Presenters/Modals/FactorsActions.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/View/FactorDialog.h"
#include <cstddef>
#include <optional>
#include <string_view>
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

		/** The dialog called `id` ("speed" or "extrusion"), or nullptr */
		FactorDialog* dialog(std::string_view id);

	  protected:
		/** Follow the speed and extrusion factors */
		void onInit() override;

	  private:
		/** Show the speed factor */
		void showSpeed();

		/** Show the extrusion factors of the extruders the dialog is for */
		void showExtrusion();

		/** The extruders the extrusion dialog is for, by number: the tool's, or every extruder (the model lock held) */
		std::vector<size_t> extruders() const;

		std::optional<size_t> m_tool; // Number of the tool the extrusion dialog is for; none for every extruder
		FactorDialog m_speed;		  // `speed.*`
		FactorDialog m_extrusion;	  // `extrusion.*`
	};
} // namespace UI::Xml

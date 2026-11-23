/*
 * BabystepSource.h
 *
 * Publishes babystepping for every view that shows it (the Move tab, a dashboard widget, the shared dialog): the step
 * size, a value selector (UI/Xml/View/ValueSelector.h) whose presets are settings, whether Z can be babystepped, and
 * whether the shared dialog (assets/ui/components/modals/babystep_dialog.xml) is open; the offset is
 * source.move.babystep. The
 * global actions `actions.babystep.*` use them (src/UI/Presenters/Modals/BabystepActions.h); `actions.selector.*`
 * choose the step.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/ValueSelector.h"

namespace UI::Xml
{
	/** Publishes `source.babystep.*` */
	class BabystepSource : public DataSource
	{
	  public:
		/** Create the source, with the dialog closed. Nothing is published until DataSources::start(). */
		BabystepSource();

		/** Declare `source.babystep.*` */
		void fields(FieldVisitor& f) override;

		/** Show whether Z can be babystepped, and by how much it has been */
		void update() override;

		/** Nothing can be babystepped while disconnected */
		void clear() override;

		/** Move Z up (`direction` 1) or down (-1) by the chosen step */
		void step(int direction) const;

		/** Open the shared dialog; it closes itself (DsDialog.h) */
		void openDialog();

	  protected:
		/** Follow the axes */
		void onInit() override;

	  private:
		ValueSelector m_step;		   // `step.*`
		IntSubject m_usable;		   // `usable`
		IntSubject m_dialogOpen;	   // `dialog_open`
	};
} // namespace UI::Xml

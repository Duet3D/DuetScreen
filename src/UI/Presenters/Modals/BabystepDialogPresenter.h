/*
 * BabystepDialogPresenter.h
 *
 * Behaviour for the shared babystepping dialog (assets/ui/components/modals/babystep_dialog.xml), which any view opens
 * with the global action `actions.babystep.open` (BabystepActions.cpp). The babystepping inside it is babystep.xml, with its own
 * presenter (Common/BabyStepPresenter.h).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class BabystepDialogPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with the dialog closed */
		BabystepDialogPresenter();

		/** Declare the dialog's field */
		void fields(Xml::FieldVisitor& f) override;

		/** The shared dialog's presenter: the view called `babystep_dialog`'s; nullptr, with a warning, if there is none */
		static BabystepDialogPresenter* shared();

		/** Open the dialog */
		void open();

	  private:
		Xml::IntSubject m_open; // 1 while the dialog is open
	};
} // namespace UI

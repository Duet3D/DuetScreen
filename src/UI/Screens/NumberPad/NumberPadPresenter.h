/*
 * NumberPadPresenter.h
 *
 * Behaviour for the shared number pad (assets/ui/components/numberpad/numberpad.xml): a dialog asking for one number,
 * or for digits and dots, at a time. Code asks it with Xml::Services::askNumber() or askText(), never directly.
 *
 * Its names and actions, including those of its NumberEntry, are declared in fields() and its constructor, and listed
 * in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/NumberEntry.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class NumberPadPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with the dialog closed */
		NumberPadPresenter();

		/** Declare the dialog's fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Open the dialog asking for `request`, replacing any question it was showing */
		void ask(Xml::NumberRequest request);

		/** Open the dialog asking for `request`, replacing any question it was showing */
		void ask(Xml::TextRequest request);

	  private:
		/** Close the dialog, forgetting the question */
		void close();

		Xml::IntSubject m_open;	 // `open`: 1 while the dialog is open
		Xml::NumberEntry m_pad; // `pad.*`: what is asked for and typed
	};
} // namespace UI

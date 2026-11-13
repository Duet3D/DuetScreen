/*
 * ObjectCancelPresenter.h
 *
 * Behaviour for the Object Cancel tab (assets/ui/components/control/object_cancel/object_cancel.xml): the job's objects
 * drawn on the bed and listed, the one being printed, and cancelling (or resuming) objects.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>

namespace UI
{
	/** One object of the job */
	class JobObjectRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::IntSubject number;		  // Object number
		Xml::StringSubject<96> label;  // "[n]: name"
		Xml::IntSubject x1;			  // Bounds, mm
		Xml::IntSubject y1;			  // …
		Xml::IntSubject x2;			  // …
		Xml::IntSubject y2;			  // …
		Xml::IntSubject current;	  // 1 if being printed
		Xml::IntSubject cancelled;	  // 1 if cancelled
	};

	class ObjectCancelPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		ObjectCancelPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for the job's objects and the axes */
		void onInit() override;

		/** Show the objects as they are now */
		void onActivate() override;

	  private:
		/** Show every object, the one being printed, and the bed's span */
		void showObjects();

		/** Show the bed's span */
		void showSpan();

		/** Ask `title` and `text`, then do `action` if the answer is yes */
		void ask(std::string title, std::string text, std::function<void()> action);

		std::function<void()> m_confirmed; // What yes does

		Xml::StringSubject<96> m_currentObject;	 // Name of the object being printed
		Xml::IntSubject m_xMin;					 // Bed span, mm
		Xml::IntSubject m_xMax;					 // …
		Xml::IntSubject m_yMin;					 // …
		Xml::IntSubject m_yMax;					 // …
		Xml::List<JobObjectRow> m_objects;		 // The job's objects
		Xml::IntSubject m_confirmations;		 // 1 if actions are confirmed
		Xml::IntSubject m_confirm;				 // 1 while asking
		Xml::StringSubject<64> m_confirmTitle;	 // Question title
		Xml::StringSubject<160> m_confirmText;	 // Question
	};
} // namespace UI

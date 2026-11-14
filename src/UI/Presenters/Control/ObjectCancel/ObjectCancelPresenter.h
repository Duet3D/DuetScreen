/*
 * ObjectCancelPresenter.h
 *
 * Behaviour for the Objects tab (assets/ui/components/control/object_cancel/object_cancel.xml): the job's objects
 * drawn on the bed and listed, the one being printed, and cancelling (or resuming) the one chosen. Until the user
 * chooses one, the one being printed is chosen.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "ObjectModel/Tree/Node.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Om/OmWatch.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <string>

namespace UI
{
	/** One object of the job */
	class JobObjectRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::IntSubject number;		  // Object number
		Xml::StringSubject<96> name;  // Its name, as the slicer wrote it
		Xml::IntSubject x1;			  // Bounds, mm
		Xml::IntSubject y1;			  // …
		Xml::IntSubject x2;			  // …
		Xml::IntSubject y2;			  // …
		Xml::IntSubject current;	  // 1 if being printed
		Xml::IntSubject cancelled;	  // 1 if cancelled
		Xml::IntSubject chosen;		  // 1 if chosen to cancel or resume
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
		/** Show every object, the one being printed, the one chosen, and the bed's span */
		void showObjects();

		/** The object chosen: the one the user chose, or else the one being printed; -1 for none (the model lock
		 * held) */
		int32_t chosenObject() const;

		/** Cancel the chosen object (M486 P), or resume it (M486 U), asking first if confirmations are on */
		void cancelChosen(bool resume, lv_obj_t* asker);

		/** Show the bed's span: the X and Y axes' travel, shifted by the current tool's offsets, or without them
		 * the objects' bounds with a margin */
		void showSpan();

		/** The job's objects (job.build.objects; the model lock held) */
		static const OM::Node& objects();

		/** The name of the object being printed, or "Nothing" if it has none or none is (the model lock held) */
		static std::string currentObjectName();

		Xml::StringSubject<96> m_currentObject;	 // Name of the object being printed
		Xml::StringSubject<96> m_chosenName;	 // Name of the object chosen
		Xml::IntSubject m_chosenCancelled;		 // 1 if the object chosen is cancelled
		Xml::IntSubject m_chosenShown;			 // 1 if an object is chosen
		Xml::StringSubject<128> m_cancelText;	 // "Cancel <name>…"
		Xml::StringSubject<128> m_resumeText;	 // "Resume <name>…"
		Xml::IntSubject m_xMin;					 // Bed span, mm
		Xml::IntSubject m_xMax;					 // …
		Xml::IntSubject m_yMin;					 // …
		Xml::IntSubject m_yMax;					 // …
		Xml::List<JobObjectRow> m_objects;		 // The job's objects
		int32_t m_userChoice = -1;				 // The object the user chose, or -1 to follow the one being printed
		Xml::OmWatch m_watch;					 // Shows the objects again as the job, axes or tool change
	};
} // namespace UI

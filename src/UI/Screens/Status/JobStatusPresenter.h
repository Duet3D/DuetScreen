/*
 * JobStatusPresenter.h
 *
 * Behaviour for the job status panel (assets/ui/components/job_status.xml): the current (or last) job with its
 * progress and thumbnail, the tool position, speed, flow, babystepping and times, and pause, resume, print again and
 * cancel.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>

namespace UI
{
	/** One axis' position */
	class AxisPositionRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<24> text; // Letter and position on two lines
	};

	class JobStatusPresenter : public Xml::XmlPresenter
	{
	  public:
		/** A control's state, as published */
		enum class Control : int32_t
		{
			Hidden = 0,
			Disabled = 1,
			Usable = 2
		};

		/** Create the presenter */
		JobStatusPresenter();

		/** Declare the panel's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for job, state, move and extruder changes */
		void onInit() override;

		/** Show everything as it is now */
		void onActivate() override;

		/** Close the question: the job may have changed */
		void onDeactivate() override;

	  private:
		/** A job started (or none is running, if `file` is empty) */
		void jobFileChanged(const std::string& file);

		/** The last job's file changed */
		void lastJobFileChanged(const std::string& file);

		/** Show the job's progress and elapsed time */
		void showProgress();

		/** Show the time left */
		void showRemaining();

		/** Show the controls usable in `status` */
		void showControls(OM::PrinterStatus status);

		/** Show the axes' positions, the speed range and babystepping */
		void showAxes();

		/** Show the extrusion override and flow range */
		void showExtruders();

		/** Show the speed override */
		void showSpeedFactor();

		/** Show the current move's speeds */
		void showSpeeds();

		/** Show the volumetric flow */
		void showFlow();

		/** Show `file`'s thumbnail, asking the printer for it if it isn't here yet; nothing if `file` is empty */
		void showThumbnail(const std::string& file);

		/** A thumbnail arrived */
		void thumbnailReceived(const std::string& file);

		/** The job shown: the current one, else the last one */
		static const std::string& shownJob();

		int32_t m_maxSpeed = 1;			// Fastest axis' maximum speed, mm/s
		int32_t m_maxExtrusionRate = 1; // Fastest extruder's maximum speed

		Xml::StringSubject<256> m_file;			// Job shown
		Xml::IntSubject m_finished;				// 1 when showing the last job
		Xml::IntSubject m_progress;				// 0-100
		Xml::StringSubject<256> m_thumbnail;	// Thumbnail path, or empty
		Xml::List<AxisPositionRow> m_axes;		// Axis positions
		Xml::StringSubject<16> m_speedFactor;	// Speed override text
		Xml::StringSubject<32> m_speedText;		// Current and requested speed text
		Xml::IntSubject m_speedTop;				// Top speed, thousandths of the maximum
		Xml::IntSubject m_speedRequested;		// Requested speed, thousandths of the maximum
		Xml::StringSubject<16> m_flowFactor;	// Extrusion override text
		Xml::StringSubject<32> m_flowText;		// Volumetric flow text
		Xml::IntSubject m_flow;					// Volumetric flow, thousandths of the maximum
		Xml::StringSubject<24> m_babystep;		// Babystep text
		Xml::StringSubject<48> m_elapsed;		// Elapsed time text
		Xml::StringSubject<48> m_remaining;		// Time left text
		Xml::IntSubject m_pause;				// Pause control state
		Xml::IntSubject m_resume;				// Resume control state
		Xml::IntSubject m_printAgain;			// Print again control state
		Xml::IntSubject m_cancel;				// Cancel control state
		Xml::IntSubject m_confirmCancel;		// 1 while asking whether to cancel
		Xml::IntSubject m_babystepOpen;			// 1 while the babystepping dialog is open
	};
} // namespace UI

/*
 * JobSource.h
 *
 * Publishes the job: the current one's file, progress and times; the latest one (the current job, else the last) with
 * its thumbnail; and which of pause, resume, print again and cancel the printer's state allows.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Node.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Om/OmWatch.h"
#include <cstdint>
#include <string>

namespace UI::Xml
{
	class JobSource : public DataSource
	{
	  public:
		/** Whether a job control is offered, as `job.*_control` publishes it */
		enum class Control : int32_t
		{
			Hidden = 0,	  // Not shown
			Disabled = 1, // Shown but can't be used now
			Usable = 2	  // Shown and usable
		};

		/** Create the source. Nothing is published until DataSources::start(). */
		JobSource();

		/** Declare `job.*` */
		void fields(FieldVisitor& f) override;

		/** Read the current and last jobs */
		void update() override;

		/** Forget the jobs */
		void clear() override;

		/**
		 * The current job's progress, percent 0-100, measured as the user chose in the settings: through the file, or
		 * through the expected print time
		 */
		static int32_t progress();

		/** The current job's time left, in seconds: from its simulation if it was simulated, else the slicer's,
		 * filament's or file's estimate */
		static uint32_t remaining();

		/** `seconds` as hh:mm:ss */
		static std::string clock(uint32_t seconds);

	  protected:
		/** Follow the job, thumbnails arriving and the printer's state */
		void onInit() override;

	  private:
		/** The object model's job (the model lock held) */
		static const OM::Node& job();

		/** The job changed: a job started or ended, or its progress */
		void jobChanged();

		/** A job started (or none is running, if `file` is empty) */
		void fileChanged(const std::string& file);

		/** The last job's file changed */
		void lastFileChanged(const std::string& file);

		/** Show the progress and elapsed time */
		void showProgress();

		/** Show the time left */
		void showRemaining();

		/** Show which controls `status` allows */
		void showControls(OM::PrinterStatus status);

		/** Show `file`'s thumbnail, asking the printer for it if it isn't here yet; nothing if `file` is empty */
		void showThumbnail(const std::string& file);

		/** A thumbnail arrived: show it if it is the latest job's */
		void thumbnailReceived(const std::string& file);

		StringSubject<256> m_file;		 // Current job's file, or empty
		StringSubject<256> m_latest;	 // The current job's file, else the last job's
		IntSubject m_finished;			 // 1 when the latest job is the last one, finished
		StringSubject<256> m_thumbnail;	 // The latest job's thumbnail path, or empty
		IntSubject m_progress;			 // 0-100
		StringSubject<16> m_elapsed;	 // hh:mm:ss
		StringSubject<16> m_remaining;	 // hh:mm:ss
		StringSubject<48> m_elapsedText;	 // e.g. "Elapsed: 00:03:43", translated
		StringSubject<48> m_remainingText; // e.g. "Remaining: 00:45:03", translated
		IntSubject m_pause;				 // A Control
		IntSubject m_resume;			 // A Control
		IntSubject m_printAgain;		 // A Control
		IntSubject m_cancel;			 // A Control
		std::string m_seenFile;			 // The current job's file when last looked at
		std::string m_seenLast;			 // The last job's file when last looked at
		OmWatch m_watch;				 // Follows the job
	};
} // namespace UI::Xml

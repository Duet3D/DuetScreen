/*
 * JobSource.h
 *
 * Publishes the current job: its file, progress and times.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include <cstdint>
#include <string>

namespace UI::Xml
{
	class JobSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		JobSource();

		/** Declare `job.*` */
		void fields(FieldVisitor& f) override;

		/** Read the current job */
		void update() override;

		/** Forget the job */
		void clear() override;

		/**
		 * The current job's progress, percent 0-100, measured as the user chose in the settings: through the file, or
		 * through the expected print time
		 */
		static int32_t progress();

		/** `seconds` as hh:mm:ss */
		static std::string clock(uint32_t seconds);

	  protected:
		/** Listen for jobs starting and ending, and their progress */
		void onInit() override;

	  private:
		/** The current job's file changed */
		void fileChanged(const std::string& file);

		/** Show the progress and elapsed time */
		void showProgress();

		/** Show the time left */
		void showRemaining();

		StringSubject<256> m_file;	   // Current job's file, or empty
		IntSubject m_progress;		   // 0-100
		StringSubject<16> m_elapsed;   // hh:mm:ss
		StringSubject<16> m_remaining; // hh:mm:ss
	};
} // namespace UI::Xml

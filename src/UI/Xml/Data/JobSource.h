/*
 * JobSource.h
 *
 * Publishes the current job.
 *
 *   job.file   the current job's file, e.g. "0:/gcodes/part.gcode"; empty when there is none. E.g. the dashboard
 *              shows its status tab whenever a job starts: `<ds_tabs-tab select_on="job.file"/>`
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include <string>

namespace UI::Xml
{
	class JobSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		JobSource();

		/** Declare `job.file` */
		void fields(FieldVisitor& f) override;

		/** Read the current job */
		void update() override;

		/** Forget the job */
		void clear() override;

	  protected:
		/** Listen for jobs starting and ending */
		void onInit() override;

	  private:
		/** The current job's file changed */
		void fileChanged(const std::string& file);

		StringSubject<256> m_file; // Current job's file, or empty
	};
} // namespace UI::Xml

/*
 * JobProgressSource.h
 *
 * How the job's progress is measured, as the user chooses in the settings (ID_JOB_PROGRESS_SOURCE).
 */

#pragma once

namespace OM
{
	enum class JobProgressSource
	{
		DURATION = 0, // By the time printed, of the expected print time
		FILE = 1	  // By how far through the file the printer is
	};
} // namespace OM

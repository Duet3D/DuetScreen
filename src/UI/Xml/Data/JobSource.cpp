/*
 * JobSource.cpp
 */

#include "JobSource.h"
#include "ObjectModel/Job.h"
#include "UI/Core/Model.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI::Xml
{
	JobSource::JobSource()
		: DataSource("job")
	{
	}

	void JobSource::fields(FieldVisitor& f)
	{
		f.field("job.file", m_file, "The current job's file, e.g. \"0:/gcodes/part.gcode\"; empty when there is none");
		f.field("job.progress",
				m_progress,
				"The current job's progress, percent 0-100, through the file or the expected time as set in the "
				"settings");
		f.field("job.elapsed", m_elapsed, "How long the current job has been running, e.g. \"00:03:43\"");
		f.field("job.remaining", m_remaining, "The current job's expected time left, e.g. \"00:45:03\"");
	}

	void JobSource::onInit()
	{
		registerEventListener<EventType::JobFileName>(this, &JobSource::fileChanged);
		registerEventListener<EventType::JobDuration>([this]() { showProgress(); });
		registerEventListener<EventType::JobFileProgress>([this]() { showProgress(); });
		registerEventListener<EventType::JobTimeLeft>([this]() { showRemaining(); });
	}

	void JobSource::update()
	{
		{
			MODEL_LOCK();
			fileChanged(OM::GetJobName());
		}
		showProgress();
		showRemaining();
	}

	void JobSource::clear()
	{
		m_file.set("");
		m_progress.set(0);
		m_elapsed.set(clock(0));
		m_remaining.set(clock(0));
	}

	int32_t JobSource::progress()
	{
		MODEL_LOCK();
		if (StorageHelper::getData(ID_JOB_PROGRESS_SOURCE) == OM::JobProgressSource::FILE)
		{
			const uint32_t size = OM::GetFileSize();
			return size == 0 ? 0 : static_cast<int32_t>(std::min<uint32_t>(100 * OM::GetFilePosition() / size, 100));
		}
		const uint32_t elapsed = OM::GetPrintDuration();
		const uint32_t warmUp = OM::GetWarmUpDuration();
		const uint32_t total = std::max<uint32_t>(OM::GetPrintTime(), OM::GetSimulatedTime());
		return total == 0 ? 0
						  : static_cast<int32_t>(
								std::min<uint32_t>(100 * (elapsed > warmUp ? elapsed - warmUp : 0) / total, 100));
	}

	std::string JobSource::clock(uint32_t seconds)
	{
		return fmt::format("{:02d}:{:02d}:{:02d}", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
	}

	void JobSource::fileChanged(const std::string& file)
	{
		m_file.set(file);
	}

	void JobSource::showProgress()
	{
		m_progress.set(progress());
		m_elapsed.set(clock(OM::GetPrintDuration()));
	}

	void JobSource::showRemaining()
	{
		m_remaining.set(clock(OM::GetPrintRemaining(OM::RemainingTimeType::AUTO)));
	}
} // namespace UI::Xml

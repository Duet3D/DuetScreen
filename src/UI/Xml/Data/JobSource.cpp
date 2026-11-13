/*
 * JobSource.cpp
 */

#include "JobSource.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "ObjectModel/Job.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
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
		f.field("job.latest_file",
				m_latest,
				"The latest job's file: the current job's, or the last job's when it has finished");
		f.field("job.finished", m_finished, "1 when the latest job is the last one, finished, else 0");
		f.field("job.thumbnail", m_thumbnail, "The latest job's thumbnail path, or empty until it arrives");
		f.field("job.progress",
				m_progress,
				"The current job's progress, percent 0-100, through the file or the expected time as set in the "
				"settings");
		f.field("job.elapsed", m_elapsed, "How long the current job has been running, e.g. \"00:03:43\"");
		f.field("job.remaining", m_remaining, "The current job's expected time left, e.g. \"00:45:03\"");
		f.field(
			"job.elapsed_text", m_elapsedText, "`job.elapsed` in a sentence, translated, e.g. \"Elapsed: 00:03:43\"");
		f.field("job.remaining_text",
				m_remainingText,
				"`job.remaining` in a sentence, translated, e.g. \"Remaining: 00:45:03\"");
		f.field("job.pause_control", m_pause, "Pausing the job (job.pause): 0 hidden, 1 disabled, 2 usable");
		f.field("job.resume_control", m_resume, "Resuming the job (job.resume): 0 hidden, 1 disabled, 2 usable");
		f.field("job.print_again_control",
				m_printAgain,
				"Printing the last job again (job.print_again): 0 hidden, 1 disabled, 2 usable");
		f.field("job.cancel_control", m_cancel, "Cancelling the job: 0 hidden, 1 disabled, 2 usable");
	}

	void JobSource::onInit()
	{
		registerEventListener<EventType::JobFileName>(this, &JobSource::fileChanged);
		registerEventListener<EventType::JobLastFileName>(this, &JobSource::lastFileChanged);
		registerEventListener<EventType::ThumbnailData>(this, &JobSource::thumbnailReceived);
		registerEventListener<EventType::JobDuration>([this]() { showProgress(); });
		registerEventListener<EventType::JobFileProgress>([this]() { showProgress(); });
		registerEventListener<EventType::JobTimeLeft>([this]() { showRemaining(); });
		registerEventListener<EventType::Status>(this, &JobSource::showControls);
	}

	void JobSource::update()
	{
		ZoneScoped;
		std::string file;
		std::string last;
		{
			MODEL_LOCK();
			file = OM::GetJobName();
			last = OM::GetLastJobName();
		}
		// As the events would: the last job, if there is one, is the latest over the current one
		fileChanged(file);
		lastFileChanged(last);
		showProgress();
		showRemaining();
		showControls(OM::GetStatus());
	}

	void JobSource::clear()
	{
		m_file.set("");
		m_latest.set("");
		m_finished.set(0);
		m_thumbnail.set("");
		m_progress.set(0);
		m_elapsed.set(clock(0));
		m_remaining.set(clock(0));
		m_elapsedText.set(_("status.elapsed_time", clock(0)));
		m_remainingText.set(_("status.remaining_time", clock(0)));
		showControls(OM::PrinterStatus::disconnected);
		m_printAgain.set(static_cast<int32_t>(Control::Hidden));
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
		m_latest.set(file);
		m_finished.set(0);
		showThumbnail(file);
	}

	void JobSource::lastFileChanged(const std::string& file)
	{
		m_printAgain.set(static_cast<int32_t>(file.empty() ? Control::Hidden : Control::Usable));
		if (file.empty())
		{
			return;
		}
		m_latest.set(file);
		m_finished.set(1);
		showThumbnail(file);
	}

	void JobSource::showProgress()
	{
		m_progress.set(progress());
		m_elapsed.set(clock(OM::GetPrintDuration()));
		m_elapsedText.set(_("status.elapsed_time", m_elapsed.get()));
	}

	void JobSource::showRemaining()
	{
		m_remaining.set(clock(OM::GetPrintRemaining(OM::RemainingTimeType::AUTO)));
		m_remainingText.set(_("status.remaining_time", m_remaining.get()));
	}

	void JobSource::showControls(OM::PrinterStatus status)
	{
		const auto set = [](IntSubject& subject, Control control) { subject.set(static_cast<int32_t>(control)); };
		switch (status)
		{
		case OM::PrinterStatus::processing:
		case OM::PrinterStatus::simulating:
			set(m_resume, Control::Hidden);
			set(m_pause, Control::Usable);
			set(m_cancel, Control::Disabled);
			break;
		case OM::PrinterStatus::paused:
			set(m_resume, Control::Usable);
			set(m_pause, Control::Hidden);
			set(m_cancel, Control::Usable);
			break;
		case OM::PrinterStatus::pausing:
			set(m_resume, Control::Disabled);
			set(m_pause, Control::Hidden);
			set(m_cancel, Control::Disabled);
			break;
		case OM::PrinterStatus::resuming:
			set(m_resume, Control::Hidden);
			set(m_pause, Control::Disabled);
			set(m_cancel, Control::Disabled);
			break;
		default:
			set(m_resume, Control::Hidden);
			set(m_pause, Control::Hidden);
			set(m_cancel, Control::Disabled);
			break;
		}
	}

	void JobSource::showThumbnail(const std::string& file)
	{
		if (file.empty())
		{
			return;
		}
		if (IsThumbnailCached(file))
		{
			thumbnailReceived(file);
			return;
		}
		m_thumbnail.set("");
		LOG_DBG("Requesting thumbnail for '{:s}'", file);
		FILEINFO_CACHE->QueueThumbnailRequest(file, true);
	}

	void JobSource::thumbnailReceived(const std::string& file)
	{
		if (file.empty() || file != m_latest.get())
		{
			return; // Not the latest job's
		}
		m_thumbnail.set(GetThumbnailPath(file).string());
	}
} // namespace UI::Xml

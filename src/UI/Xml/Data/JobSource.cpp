/*
 * JobSource.cpp
 */

#include "JobSource.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "ObjectModel/JobProgressSource.h"
#include "ObjectModel/Tree/Tree.h"
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
		m_watch = OmWatch({"job"}, [this] { jobChanged(); });
		registerEventListener<EventType::ThumbnailData>(this, &JobSource::thumbnailReceived);
		registerEventListener<EventType::Status>(this, &JobSource::showControls);
	}

	void JobSource::update()
	{
		ZoneScoped;
		std::string file;
		std::string last;
		{
			MODEL_LOCK();
			file = std::string(job()["file"]["fileName"].text());
			last = std::string(job()["lastFileName"].text());
		}
		// As if both had just changed: the last job, if there is one, is the latest over the current one
		m_seenFile = file;
		m_seenLast = last;
		fileChanged(file);
		lastFileChanged(last);
		showProgress();
		showRemaining();
		showControls(OM::GetStatus());
	}

	void JobSource::jobChanged()
	{
		std::string file;
		std::string last;
		{
			MODEL_LOCK();
			file = std::string(job()["file"]["fileName"].text());
			last = std::string(job()["lastFileName"].text());
		}
		// A job starting or ending changes one name; the latest job is the one that changed last
		if (file != m_seenFile)
		{
			m_seenFile = file;
			fileChanged(file);
		}
		if (last != m_seenLast)
		{
			m_seenLast = last;
			lastFileChanged(last);
		}
		showProgress();
		showRemaining();
	}

	void JobSource::clear()
	{
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

	const OM::Node& JobSource::job()
	{
		return OM::Tree::get().root()["job"];
	}

	int32_t JobSource::progress()
	{
		MODEL_LOCK();
		const OM::Node& node = job();
		if (StorageHelper::getData(ID_JOB_PROGRESS_SOURCE) == OM::JobProgressSource::FILE)
		{
			const int64_t size = node["file"]["size"].integer(0);
			return size <= 0 ? 0
							 : static_cast<int32_t>(
								   std::clamp<int64_t>(100 * node["filePosition"].integer(0) / size, 0, 100));
		}
		const int64_t elapsed = node["duration"].integer(0);
		const int64_t warmUp = node["warmUpDuration"].integer(0);
		const int64_t total = std::max(node["file"]["printTime"].integer(0), node["file"]["simulatedTime"].integer(0));
		return total <= 0 ? 0
						  : static_cast<int32_t>(
								std::clamp<int64_t>(100 * std::max<int64_t>(elapsed - warmUp, 0) / total, 0, 100));
	}

	uint32_t JobSource::remaining()
	{
		MODEL_LOCK();
		const OM::Node& node = job();
		// A simulated job's time is the most exact; then the slicer's estimate, the filament's, the file's
		if (const int64_t simulated = node["file"]["simulatedTime"].integer(0); simulated > 0)
		{
			const int64_t printed = node["duration"].integer(0) - node["warmUpDuration"].integer(0);
			return static_cast<uint32_t>(std::max<int64_t>(simulated - printed, 0));
		}
		const OM::Node& timesLeft = node["timesLeft"];
		for (const char* estimate : {"slicer", "filament"})
		{
			if (const int64_t left = timesLeft[estimate].integer(0); left > 0)
			{
				return static_cast<uint32_t>(left);
			}
		}
		return static_cast<uint32_t>(std::max<int64_t>(timesLeft["file"].integer(0), 0));
	}

	std::string JobSource::clock(uint32_t seconds)
	{
		return fmt::format("{:02d}:{:02d}:{:02d}", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
	}

	void JobSource::fileChanged(const std::string& file)
	{
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
		uint32_t duration = 0;
		{
			MODEL_LOCK();
			duration = static_cast<uint32_t>(std::max<int64_t>(job()["duration"].integer(0), 0));
		}
		m_elapsed.set(clock(duration));
		m_elapsedText.set(_("status.elapsed_time", m_elapsed.get()));
	}

	void JobSource::showRemaining()
	{
		m_remaining.set(clock(remaining()));
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

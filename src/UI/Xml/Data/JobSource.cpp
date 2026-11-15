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
#include <ctime>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** The time of day `seconds` from now, e.g. "14:32"; from noon in tests, so pictures don't change */
		std::string timeOfDayIn(uint32_t seconds)
		{
#if USE_FIXED_TEST_STRINGS
			constexpr uint32_t NOON = 12 * 3600;
			const uint32_t at = (NOON + seconds) % (24 * 3600);
			return fmt::format("{:02d}:{:02d}", at / 3600, (at % 3600) / 60);
#else
			const std::time_t at = std::time(nullptr) + static_cast<std::time_t>(seconds);
			std::tm local{};
			localtime_r(&at, &local);
			return fmt::format("{:02d}:{:02d}", local.tm_hour, local.tm_min);
#endif
		}
	} // namespace

	void JobSource::showLayer()
	{
		int64_t layer = 0;
		int64_t layers = 0;
		{
			MODEL_LOCK();
			layer = job()["layer"].integer(0);
			layers = job()["file"]["numLayers"].integer(0);
		}
		m_layer.set(layer > 0 && layers > 0 ? _("job.layer_of", layer, layers) : std::string());
	}

	JobSource::JobSource()
		: DataSource("job")
	{
		m_page.set(READY_PAGE);
	}

	void JobSource::fields(FieldVisitor& f)
	{
		f.field("latest_file",
				m_latest,
				"The latest job's file: the current job's, or the last job's when it has finished");
		f.field("finished",
				m_finished,
				"1 once the latest job has ended (finished, cancelled, stopped or simulated) and no other runs, else "
				"0. Whether a job runs is source.machine.job_active.");
		f.field("page",
				m_page,
				"The job widget's page (dashboard_job.xml): \"status\" from when a job starts until the user goes "
				"back, else \"ready\". Its <ds_pages bind_page> follows and sets it.");
		f.field("name", m_name, "The latest job's file, without its folder, e.g. \"benchy.gcode\"");
		f.field("state",
				m_state,
				"What the job is doing, in the machine's words: \"Printing\" (\"Running\" on a CNC or laser machine), "
				"\"Paused\", \"Finished\"; empty for no job");
		f.field("layer", m_layer, "The layer being printed, e.g. \"51 of 120\"; empty if the job has no layers");
		f.field("finish_at",
				m_finishAt,
				"The time of day the job should finish, e.g. \"14:32\"; empty while it isn't running or can't be told");
		f.field("last_detail",
				m_lastDetail,
				"How the last job ended and how long it took, e.g. \"Finished at 13:02 · took 1 h 13 min\" (the time "
				"of day only if the screen saw it end); empty if there was no last job");
		f.field("thumbnail", m_thumbnail, "The latest job's thumbnail path, or empty until it arrives");
		f.field("progress",
				m_progress,
				"The current job's progress, percent 0-100, through the file or the expected time as set in the "
				"settings");
		f.field("elapsed", m_elapsed, "How long the current job has been running, e.g. \"00:03:43\"");
		f.field("remaining", m_remaining, "The current job's expected time left, e.g. \"00:45:03\"");
		f.field("elapsed_text",
				m_elapsedText,
				"`source.job.elapsed` in a sentence, translated, e.g. \"Elapsed: 00:03:43\"");
		f.field("remaining_text",
				m_remainingText,
				"`source.job.remaining` in a sentence, translated, e.g. \"Remaining: 00:45:03\"");
		f.field("pause_control", m_pause, "Pausing the job (actions.job.pause): 0 hidden, 1 disabled, 2 usable");
		f.field("resume_control", m_resume, "Resuming the job (actions.job.resume): 0 hidden, 1 disabled, 2 usable");
		f.field("print_again_control",
				m_printAgain,
				"Printing the last job again (actions.job.print_again): 0 hidden, 1 disabled, 2 usable");
		f.field("cancel_control", m_cancel, "Cancelling the job: 0 hidden, 1 disabled, 2 usable");
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
		showLayer();
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
		// A job starting changes the file (which stays once it has ended); a job ending, the last file. The latest job
		// is the one that changed last
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
		showLayer();
		// How the last job ended may change after the printer's state does
		showControls(OM::GetStatus());
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
		m_name.set("");
		m_state.set("");
		m_layer.set("");
		m_finishAt.set("");
		m_lastDetail.set("");
		m_endedAt.clear();
		m_seenLast.clear();
		m_wasActive = false;
		m_page.set(READY_PAGE);
		showControls(OM::PrinterStatus::disconnected);
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

	std::string JobSource::duration(uint32_t seconds)
	{
		const uint32_t minutes = (seconds + 30) / 60;
		return minutes < 60 ? _("job.duration_minutes", minutes) : _("job.duration_hours", minutes / 60, minutes % 60);
	}

	const char* JobSource::endedKey()
	{
		MODEL_LOCK();
		const OM::Node& node = job();
		if (node["lastFileSimulated"].boolean(false))
		{
			return "job.ended_simulated";
		}
		if (node["lastFileCancelled"].boolean(false))
		{
			return "job.ended_cancelled";
		}
		if (node["lastFileAborted"].boolean(false))
		{
			return "job.ended_stopped";
		}
		return "job.ended_finished";
	}

	void JobSource::showLastDetail()
	{
		std::string file;
		uint32_t took = 0;
		{
			MODEL_LOCK();
			const OM::Node& node = job();
			file = std::string(node["lastFileName"].text());
			took = static_cast<uint32_t>(std::max<int64_t>(node["lastDuration"].integer(0), 0));
		}
		if (file.empty())
		{
			m_lastDetail.set("");
			return;
		}
		const char* ended = endedKey();
		const std::string how = m_endedAt.empty() ? _(ended) : _("job.ended_at", _(ended), m_endedAt);
		m_lastDetail.set(took > 0 ? _("job.ended_took", how, duration(took)) : how);
	}

	void JobSource::fileChanged(const std::string& file)
	{
		m_latest.set(file);
		m_name.set(file.substr(file.find_last_of('/') + 1));
		showThumbnail(file);
	}

	void JobSource::lastFileChanged(const std::string& file)
	{
		if (file.empty())
		{
			return;
		}
		m_latest.set(file);
		m_name.set(file.substr(file.find_last_of('/') + 1));
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
		const uint32_t left = remaining();
		m_remaining.set(clock(left));
		m_remainingText.set(_("status.remaining_time", m_remaining.get()));
		// The time of day it should finish, while it runs and the time left is known
		if (!OM::IsPrintingStatus(OM::GetStatus()) || left == 0)
		{
			m_finishAt.set("");
			return;
		}
		m_finishAt.set(timeOfDayIn(left));
	}

	void JobSource::showControls(OM::PrinterStatus status)
	{
		// A job starting shows its status; one ending notes when, while the screen watched
		const bool active = OM::IsPrintingStatus(status);
		if (active && !m_wasActive)
		{
			m_page.set(STATUS_PAGE);
			m_endedAt.clear();
		}
		else if (!active && m_wasActive)
		{
			m_endedAt = timeOfDayIn(0);
		}
		m_wasActive = active;

		bool hasLast = false;
		{
			MODEL_LOCK();
			hasLast = !job()["lastFileName"].text().empty();
		}
		// Ended once it has been cancelled too: the printer cancels it before it says how it ended
		const bool cancelling = status == OM::PrinterStatus::cancelling;
		const bool ended = !active && !cancelling && hasLast;
		m_finished.set(ended ? 1 : 0);

		const auto set = [](IntSubject& subject, Control control) { subject.set(static_cast<int32_t>(control)); };
		// Printing the last job again, once it has ended
		set(m_printAgain, ended ? Control::Usable : Control::Hidden);
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
			set(m_cancel, Control::Hidden);
			break;
		}

		// What the job is doing, in the machine's words: a CNC or laser machine runs a job rather than printing it
		bool printer = true;
		{
			MODEL_LOCK();
			const std::string_view mode = OM::Tree::get().root()["state"]["machineMode"].text();
			printer = mode.empty() || mode == "FFF";
		}
		switch (status)
		{
		case OM::PrinterStatus::processing:
		case OM::PrinterStatus::resuming:
			m_state.set(_(printer ? "job.printing" : "job.running"));
			break;
		case OM::PrinterStatus::simulating:
			m_state.set(_("job.simulating"));
			break;
		case OM::PrinterStatus::paused:
		case OM::PrinterStatus::pausing:
			m_state.set(_("job.paused"));
			break;
		case OM::PrinterStatus::cancelling:
			m_state.set(_("job.cancelling"));
			break;
		default:
			// How the last job ended
			m_state.set(m_finished.get() != 0 ? _(endedKey()) : std::string());
			break;
		}
		showLastDetail();
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

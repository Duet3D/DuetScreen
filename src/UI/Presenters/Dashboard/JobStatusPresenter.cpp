/*
 * JobStatusPresenter.cpp
 */

#include "JobStatusPresenter.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/JobSource.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	namespace
	{
		/** `value` in thousandths of `max` */
		int32_t permille(float value, int32_t max)
		{
			return std::clamp(
				static_cast<int32_t>(std::lround(1000.0f * value / static_cast<float>(std::max(max, 1)))), 0, 1000);
		}
	} // namespace

	JobStatusPresenter::JobStatusPresenter()
		: XmlPresenter("dashboard_job_status")
	{
		action("job.pause", "Pause the job", [](const Xml::ActionArgs&) { OM::FileSystem::PausePrint(); });
		action("job.resume", "Resume the paused job", [](const Xml::ActionArgs&) { OM::FileSystem::ResumePrint(); });
		action("job.print_again",
			   "Start the last job again",
			   [](const Xml::ActionArgs&) { OM::FileSystem::PrintAgain(); });
		action("job.ask_cancel",
			   "Ask whether to cancel the job (sets `confirm_cancel`)",
			   [this](const Xml::ActionArgs&) { m_confirmCancel.set(1); });
		action("job.cancel",
			   "Cancel the job, which also turns the heaters off, and close the question",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirmCancel.set(0);
				   OM::FileSystem::StopPrint(); // Also turns the heaters off
			   });
		action("job.close_dialog",
			   "Close the question without cancelling the job",
			   [this](const Xml::ActionArgs&) { m_confirmCancel.set(0); });
		action("job.babystep",
			   "Open the babystepping dialog (sets `babystep_open`)",
			   [this](const Xml::ActionArgs&) { m_babystepOpen.set(1); });
		action("job.close_babystep",
			   "Close the babystepping dialog",
			   [this](const Xml::ActionArgs&) { m_babystepOpen.set(0); });
	}

	void JobStatusPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("file", m_file, "The current job's file, else the last job's");
		f.field("finished", m_finished, "1 when showing the last job (none is running), else 0");
		f.field("thumbnail", m_thumbnail, "The job's thumbnail path, or empty until it arrives");
		f.field("speed_text", m_speedText, "Current and requested speed, e.g. \"0/0 mm/s\"");
		f.field("speed_top",
				m_speedTop,
				"The current move's top speed, in thousandths of the fastest axis' maximum (0-1000)");
		f.field("speed_requested",
				m_speedRequested,
				"Requested speed, in thousandths of the fastest axis' maximum (0-1000)");
		f.field("flow_text", m_flowText, "Volumetric flow, e.g. \"0.0 mm³/s\"");
		f.field("flow", m_flow, "Volumetric flow, in thousandths of the fastest extruder's maximum (0-1000)");
		f.field("babystep_open", m_babystepOpen, "1 while the babystepping dialog is open");
		f.field("elapsed", m_elapsed, "Elapsed time, e.g. \"Elapsed: 00:03:43\"");
		f.field("remaining", m_remaining, "Time left, e.g. \"Remaining: 00:45:03\"");
		f.field("pause", m_pause, "Pause: 0 hidden, 1 disabled, 2 usable");
		f.field("resume", m_resume, "Resume: 0 hidden, 1 disabled, 2 usable");
		f.field("print_again", m_printAgain, "Print again: 0 hidden, 1 disabled, 2 usable");
		f.field("cancel", m_cancel, "Cancel: 0 hidden, 1 disabled, 2 usable");
		f.field("confirm_cancel", m_confirmCancel, "1 while asking whether to cancel the job");
	}

	void JobStatusPresenter::onInit()
	{
		registerEventListener<EventType::JobFileName>(this, &JobStatusPresenter::jobFileChanged);
		registerEventListener<EventType::JobLastFileName>(this, &JobStatusPresenter::lastJobFileChanged);
		registerEventListener<EventType::ThumbnailData>(this, &JobStatusPresenter::thumbnailReceived);
		registerEventListener<EventType::JobDuration>([this]() { showProgress(); });
		registerEventListener<EventType::JobFileProgress>([this]() { showProgress(); });
		registerEventListener<EventType::JobTimeLeft>([this]() { showRemaining(); });
		registerEventListener<EventType::Status>(this, &JobStatusPresenter::showControls);
		registerEventListener<EventType::AxesData>([this]() { showAxes(); });
		registerEventListener<EventType::ExtruderData>([this]() { showExtruders(); });
		registerEventListener<EventType::CurrentMoveRequestedSpeed>([this]() { showSpeeds(); });
		registerEventListener<EventType::CurrentMoveTopSpeed>([this]() { showSpeeds(); });
		registerEventListener<EventType::CurrentMoveExtrusionSpeed>([this]() { showFlow(); });
	}

	void JobStatusPresenter::onActivate()
	{
		ZoneScoped;
		// As the events would: the last job, if there is one, is shown over the current one
		jobFileChanged(OM::GetJobName());
		lastJobFileChanged(OM::GetLastJobName());
		showProgress();
		showRemaining();
		showControls(OM::GetStatus());
		showAxes();
		showExtruders();
		showSpeeds();
		showFlow();

		showThumbnail(shownJob());
	}

	void JobStatusPresenter::onDeactivate()
	{
		m_confirmCancel.set(0);
		m_babystepOpen.set(0);
	}

	void JobStatusPresenter::jobFileChanged(const std::string& file)
	{
		m_file.set(file);
		m_finished.set(0);
		showThumbnail(file);
	}

	void JobStatusPresenter::lastJobFileChanged(const std::string& file)
	{
		m_printAgain.set(static_cast<int32_t>(file.empty() ? Control::Hidden : Control::Usable));
		if (file.empty())
		{
			return;
		}
		m_file.set(file);
		m_finished.set(1);
		showThumbnail(file);
	}

	void JobStatusPresenter::showProgress()
	{
		m_elapsed.set(_("status.elapsed_time", Xml::JobSource::clock(OM::GetPrintDuration())));
	}

	void JobStatusPresenter::showRemaining()
	{
		m_remaining.set(
			_("status.remaining_time", Xml::JobSource::clock(OM::GetPrintRemaining(OM::RemainingTimeType::AUTO))));
	}

	void JobStatusPresenter::showControls(OM::PrinterStatus status)
	{
		auto set = [](Xml::IntSubject& subject, Control control) { subject.set(static_cast<int32_t>(control)); };
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

	void JobStatusPresenter::showAxes()
	{
		ZoneScoped;
		{
			MODEL_LOCK();
			int32_t maxSpeed = 1;
			for (size_t slot = 0; slot < OM::Move::GetAxisCount(); slot++)
			{
				if (auto axis = OM::Move::GetAxisBySlot(slot))
				{
					maxSpeed = std::max(maxSpeed, axis->maxSpeed);
				}
			}
			m_maxSpeed = maxSpeed;
		}
		showSpeeds();
	}

	void JobStatusPresenter::showExtruders()
	{
		ZoneScoped;
		{
			MODEL_LOCK();
			// The current tool's extruders; with no tool (or none with extruders) the range stays as it was
			int32_t maxRate = 0;
			if (auto tool = OM::GetCurrentTool())
			{
				tool->IterateExtruders([&](OM::Move::ExtruderAxisPtr extruder, size_t)
									   { maxRate = std::max(maxRate, extruder->maxSpeed); });
			}
			if (maxRate > 0)
			{
				m_maxExtrusionRate = maxRate;
			}
		}
		showFlow();
	}

	void JobStatusPresenter::showSpeeds()
	{
		const float top = OM::Move::GetCurrentMoveTopSpeed();
		const float requested = OM::Move::GetCurrentMoveRequestedSpeed();
		m_speedTop.set(permille(std::round(top), m_maxSpeed));
		m_speedRequested.set(permille(std::round(requested), m_maxSpeed));
		m_speedText.set(_("status.speed_label", top, requested));
	}

	void JobStatusPresenter::showFlow()
	{
		const float volumetric = OM::Move::GetVolumetricFlow();
		m_flow.set(permille(std::round(volumetric), m_maxExtrusionRate));
		m_flowText.set(_("status.flow_rate", volumetric));
	}

	void JobStatusPresenter::showThumbnail(const std::string& file)
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

	void JobStatusPresenter::thumbnailReceived(const std::string& file)
	{
		if (file.empty() || (file != OM::GetJobName() && file != OM::GetLastJobName()))
		{
			return; // Not this job's
		}
		m_thumbnail.set(GetThumbnailPath(file).string());
	}

	const std::string& JobStatusPresenter::shownJob()
	{
		const std::string& current = OM::GetJobName();
		return current.empty() ? OM::GetLastJobName() : current;
	}
} // namespace UI

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
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Control/ExtrusionFactor/ExtrusionFactor.h"
#include "UI/Widgets/Control/SpeedFactor/SpeedFactor.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cmath>

namespace UI
{
	namespace
	{
		/** `seconds` as hh:mm:ss */
		std::string clock(uint32_t seconds)
		{
			return fmt::format("{:02d}:{:02d}:{:02d}", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
		}

		/** `value` in thousandths of `max` */
		int32_t permille(float value, int32_t max)
		{
			return std::clamp(
				static_cast<int32_t>(std::lround(1000.0f * value / static_cast<float>(std::max(max, 1)))), 0, 1000);
		}
	} // namespace

	void AxisPositionRow::fields(Xml::FieldVisitor& f)
	{
		f.field("text", text, "Letter and position on two lines");
	}

	JobStatusPresenter::JobStatusPresenter()
		: XmlPresenter("job_status")
	{
		action("job.pause", [](const Xml::ActionArgs&) { OM::FileSystem::PausePrint(); });
		action("job.resume", [](const Xml::ActionArgs&) { OM::FileSystem::ResumePrint(); });
		action("job.print_again", [](const Xml::ActionArgs&) { OM::FileSystem::PrintAgain(); });
		action("job.ask_cancel", [this](const Xml::ActionArgs&) { m_confirmCancel.set(1); });
		action("job.cancel",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirmCancel.set(0);
				   OM::FileSystem::StopPrint(); // Also turns the heaters off
			   });
		action("job.close_dialog", [this](const Xml::ActionArgs&) { m_confirmCancel.set(0); });
		action("job.speed_factor",
			   [](const Xml::ActionArgs&)
			   {
				   if (auto* modal = findView<ModalSpeedFactor>("speed_factor"))
				   {
					   modal->open();
				   }
			   });
		action("job.flow_factor",
			   [](const Xml::ActionArgs&)
			   {
				   if (auto* modal = findView<ModalExtrusionFactor>("job_extrusion_factor"))
				   {
					   modal->open();
				   }
			   });
		action("job.babystep", [this](const Xml::ActionArgs&) { m_babystepOpen.set(1); });
		action("job.close_babystep", [this](const Xml::ActionArgs&) { m_babystepOpen.set(0); });
	}

	void JobStatusPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("file", m_file, "The job shown");
		f.field("finished", m_finished, "1 when showing the last job");
		f.field("progress", m_progress, "0-100");
		f.field("thumbnail", m_thumbnail, "Thumbnail path, or empty");
		f.list("axes", m_axes, "One row per axis");
		f.field("speed_factor", m_speedFactor, "Speed override");
		f.field("speed_text", m_speedText, "Current and requested speed");
		f.field("speed_top", m_speedTop, "Top speed, thousandths of the fastest axis' maximum");
		f.field("speed_requested", m_speedRequested, "Requested speed, thousandths of the fastest axis' maximum");
		f.field("flow_factor", m_flowFactor, "Extrusion override");
		f.field("flow_text", m_flowText, "Volumetric flow");
		f.field("flow", m_flow, "Volumetric flow, thousandths of the fastest extruder's maximum");
		f.field("babystep", m_babystep, "Z babystep");
		f.field("babystep_open", m_babystepOpen, "1 while the babystepping dialog is open");
		f.field("elapsed", m_elapsed, "Elapsed time");
		f.field("remaining", m_remaining, "Time left");
		f.field("pause", m_pause, "Pause: 0 hidden, 1 disabled, 2 usable");
		f.field("resume", m_resume, "Resume: 0 hidden, 1 disabled, 2 usable");
		f.field("print_again", m_printAgain, "Print again: 0 hidden, 1 disabled, 2 usable");
		f.field("cancel", m_cancel, "Cancel: 0 hidden, 1 disabled, 2 usable");
		f.field("confirm_cancel", m_confirmCancel, "1 while asking whether to cancel");
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
		registerEventListener<EventType::SpeedFactor>([this]() { showSpeedFactor(); });
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
		showSpeedFactor();
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
		ZoneScoped;
		uint32_t progress = 0;
		{
			MODEL_LOCK();
			if (StorageHelper::getData(ID_JOB_PROGRESS_SOURCE) == OM::JobProgressSource::FILE)
			{
				const uint32_t size = OM::GetFileSize();
				progress = size == 0 ? 0 : std::min<uint32_t>(100 * OM::GetFilePosition() / size, 100);
			}
			else
			{
				const uint32_t elapsed = OM::GetPrintDuration();
				const uint32_t warmUp = OM::GetWarmUpDuration();
				const uint32_t total = std::max<uint32_t>(OM::GetPrintTime(), OM::GetSimulatedTime());
				progress =
					total == 0 ? 0 : std::min<uint32_t>(100 * (elapsed > warmUp ? elapsed - warmUp : 0) / total, 100);
			}
		}
		m_progress.set(static_cast<int32_t>(progress));
		m_elapsed.set(_("status.elapsed_time", clock(OM::GetPrintDuration())));
	}

	void JobStatusPresenter::showRemaining()
	{
		m_remaining.set(_("status.remaining_time", clock(OM::GetPrintRemaining(OM::RemainingTimeType::AUTO))));
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
		MODEL_LOCK();
		int32_t maxSpeed = 1;
		m_axes.sync(OM::Move::GetAxisCount(),
					[&](AxisPositionRow& row, size_t slot)
					{
						if (auto axis = OM::Move::GetAxisBySlot(slot))
						{
							row.text.set(fmt::format("{}\n{:.2f}", axis->letter[0], axis->userPosition));
							maxSpeed = std::max(maxSpeed, axis->maxSpeed);
						}
					});
		m_maxSpeed = maxSpeed;

		auto z = OM::Move::GetAxisByLetter('Z');
		m_babystep.set(_("status.babystep_value", z ? z->babystep : 0.0f));
		showSpeeds();
	}

	void JobStatusPresenter::showExtruders()
	{
		ZoneScoped;
		uint32_t factors = 0;
		uint32_t count = 0;
		int32_t maxRate = 1;
		auto add = [&](OM::Move::ExtruderAxisPtr extruder)
		{
			factors += static_cast<uint32_t>(std::lround(100 * extruder->factor));
			count++;
			maxRate = std::max(maxRate, extruder->maxSpeed);
		};

		if (auto tool = OM::GetCurrentTool())
		{
			tool->IterateExtruders([&](OM::Move::ExtruderAxisPtr extruder, size_t) { add(extruder); });
			if (count > 0)
			{
				m_maxExtrusionRate = maxRate;
			}
		}
		else
		{
			// No tool: the average of every extruder
			OM::Move::IterateExtruderAxesWhile(
				[&](OM::Move::ExtruderAxisPtr extruder, size_t)
				{
					add(extruder);
					return true;
				});
		}
		m_flowFactor.set(_("status.flow_multiplier", count > 0 ? factors / count : 100u));
		showFlow();
	}

	void JobStatusPresenter::showSpeedFactor()
	{
		m_speedFactor.set(
			_("status.speed_multiplier", static_cast<uint32_t>(std::lround(100 * OM::Move::GetSpeedFactor()))));
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

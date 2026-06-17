#include "StatusPresenter.h"
#include "Comm/FileInfo.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/Fan.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "StatusView.h"
#include "UI/Core/Navigation.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"

namespace UI
{
	void StatusPresenter::pausePrint()
	{
		ZoneScoped;
		OM::FileSystem::PausePrint(); // Pause print
	}

	void StatusPresenter::resumePrint()
	{
		ZoneScoped;
		OM::FileSystem::ResumePrint(); // Resume print
	}

	void StatusPresenter::printAgain()
	{
		ZoneScoped;
		OM::FileSystem::PrintAgain(); // Print again
	}

	void StatusPresenter::cancelPrint()
	{
		ZoneScoped;
		OM::FileSystem::StopPrint(); // Stop print and turn off heaters
	}

	void StatusPresenter::onActivate()
	{
		ZoneScoped;
		newJobFileName(OM::GetJobName());
		newJobLastFileName(OM::GetLastJobName());
		newJobDuration();
		newStatus(OM::GetStatus());

		std::string filename = OM::GetJobName();
		if (filename.empty())
		{
			LOG_DBG("No current job name");
			filename = OM::GetLastJobName();
		}
		if (filename.empty())
		{
			LOG_DBG("No job name");
			return;
		}

		setOrRequestThumbnail(filename);
	}

	void StatusPresenter::newJobFileName(const std::string& filename)
	{
		ZoneScoped;
		getView()->setFilename(filename, true);
		setOrRequestThumbnail(filename);
	}

	void StatusPresenter::newThumbnailData(const std::string& filename)
	{
		ZoneScoped;
		if (!filename.empty() && filename != OM::GetJobName() && filename != OM::GetLastJobName())
		{
			// Thumbnail is not for the current job
			return;
		}

		getView()->setThumbnail(GetThumbnailPath(filename).c_str());
	}

	void StatusPresenter::newJobLastFileName(const std::string& filename)
	{
		ZoneScoped;
		getView()->setPrintAgain(filename.empty() ? StatusView::HIDDEN : StatusView::ENABLED);
		if (filename.empty())
		{
			return;
		}
		getView()->setFilename(filename, false);
		setOrRequestThumbnail(filename);
	}

	void StatusPresenter::newJobDuration()
	{
		ZoneScoped;
		uint32_t progress = 0;
		{
			MODEL_LOCK();
			if (StorageHelper::getData(ID_JOB_PROGRESS_SOURCE) == OM::JobProgressSource::FILE)
			{
				uint32_t fileSize = OM::GetFileSize();
				uint32_t filePosition = OM::GetFilePosition();
				progress = fileSize == 0 ? 0 : std::min<uint32_t>((100 * filePosition) / fileSize, 100);
			}
			else
			{
				uint32_t elapsed = OM::GetPrintDuration();
				uint32_t warmupTime = OM::GetWarmUpDuration();
				uint32_t totalDuration = std::max<uint32_t>(OM::GetPrintTime(), OM::GetSimulatedTime());
				progress =
					totalDuration == 0
						? 0
						: std::min<uint32_t>((100 * std::max<uint32_t>(0, elapsed - warmupTime)) / totalDuration, 100);
			}
		}

		m_view->updateProgress(progress);
	}

	void StatusPresenter::newStatus(OM::PrinterStatus status)
	{
		ZoneScoped;
		UI_LOCK();
		switch (status)
		{
		case OM::PrinterStatus::processing:
		case OM::PrinterStatus::simulating:
			m_view->setResume(StatusView::HIDDEN);
			m_view->setPause(StatusView::ENABLED);
			m_view->setCancel(StatusView::DISABLED);
			break;
		case OM::PrinterStatus::paused:
			m_view->setResume(StatusView::ENABLED);
			m_view->setPause(StatusView::HIDDEN);
			m_view->setCancel(StatusView::ENABLED);
			break;
		case OM::PrinterStatus::pausing:
			m_view->setResume(StatusView::DISABLED);
			m_view->setPause(StatusView::HIDDEN);
			m_view->setCancel(StatusView::DISABLED);
			break;
		case OM::PrinterStatus::resuming:
			m_view->setResume(StatusView::HIDDEN);
			m_view->setPause(StatusView::DISABLED);
			m_view->setCancel(StatusView::DISABLED);
			break;
		case OM::PrinterStatus::cancelling:
			m_view->setResume(StatusView::HIDDEN);
			m_view->setPause(StatusView::HIDDEN);
			m_view->setCancel(StatusView::DISABLED);
			break;
		default:
			m_view->setResume(StatusView::HIDDEN);
			m_view->setPause(StatusView::HIDDEN);
			m_view->setCancel(StatusView::DISABLED);
			break;
		}
	}

	void StatusPresenter::setOrRequestThumbnail(const std::string& filename)
	{
		ZoneScoped;
		if (filename.empty())
		{
			return;
		}

		if (IsThumbnailCached(filename))
		{
			newThumbnailData(filename);
			return;
		}

		m_view->setThumbnail(nullptr);
		LOG_DBG("Requesting thumbnail for '{:s}'", filename);
		FILEINFO_CACHE->QueueThumbnailRequest(filename, true);
	}
} // namespace UI

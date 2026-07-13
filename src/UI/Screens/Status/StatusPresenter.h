#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Presenter.h"

namespace UI
{
	class StatusView;

	class StatusPresenter : public Presenter<StatusView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(StatusPresenter, StatusView)

		// Actions
		void pausePrint();
		void resumePrint();
		void printAgain();
		void cancelPrint();

		// Subscriptions
		void newJobFileName(const std::string& filename);
		void newJobLastFileName(const std::string& filename);
		void newThumbnailData(const std::string& filename);
		void newJobDuration();
		void newStatus(const OM::PrinterStatus status);

	  private:
		void onActivate() override;

		void onInit() override
		{
			registerEventListener<EventType::JobFileName>(this, &StatusPresenter::newJobFileName);
			registerEventListener<EventType::JobLastFileName>(this, &StatusPresenter::newJobLastFileName);
			registerEventListener<EventType::ThumbnailData>(this, &StatusPresenter::newThumbnailData);
			registerEventListener<EventType::JobDuration>(this, &StatusPresenter::newJobDuration);
			registerEventListener<EventType::JobFileProgress>(this, &StatusPresenter::newJobDuration);
			registerEventListener<EventType::Status>(this, &StatusPresenter::newStatus);
		}

		void setOrRequestThumbnail(const std::string& filename);

		lv_timer_t* m_updateTimer;
	};
} // namespace UI

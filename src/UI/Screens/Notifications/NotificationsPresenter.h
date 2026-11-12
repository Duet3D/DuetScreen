/*
 * NotificationsPresenter.h
 *
 * Behaviour for NotificationsView: queues responses from the Duet as message boxes, shows M291 alerts and sends the
 * user's answer, and offers firmware updates found on USB or GitHub.
 */

#pragma once

#include "ObjectModel/Axis.h"
#include "UI/Core/Presenter.h"
#include <vector>

namespace UI
{
	class NotificationsView;

	class NotificationsPresenter : public Presenter<NotificationsView>
	{
	  public:
		PRESENTER_CONSTRUCTOR(NotificationsPresenter, NotificationsView)

		/** Forget queued responses and the alert's axes */
		void clear();

		/** Install the update found on USB */
		void update();

		/** Install the latest release from GitHub */
		void updateFromGithub();

		/** An update file was found on USB: offer it */
		void newUpdateAvailable(const std::string& file);

		/** A newer release is on GitHub: offer it */
		void newGithubUpdateAvailable(const std::string& latestVersion);

		/** Report how an update went */
		void handleUpdateResult(const UpgradeHelper::UpgradeInfo& info);

		/** Update the alert's jog axes */
		void newAxesData();

		/** Queue a response from the Duet, unless the console is showing or it is below the notification level */
		void newResponse(const ResponseType type, const std::string& response);

		/** Show, update or close the M291 alert */
		void newAlertData(const OM::Alert& alert);

	  protected:
		/** Listen for responses, alerts, updates and USB removal */
		void onInit() override;

		/** Refresh the alert's jog axes */
		void onActivate() override;

		/** Nothing to do */
		void onDeactivate() override {}

		/** Forget everything from the printer */
		void onDisconnect() override { clear(); }

		std::vector<char> m_alertAxes; // Letters of the axes the alert lets the user jog
		std::string m_updateFile;	   // Update file offered from USB
	};
} // namespace UI

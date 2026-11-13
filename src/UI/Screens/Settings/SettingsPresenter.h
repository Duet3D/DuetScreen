#pragma once

#include "UI/Core/Presenter.h"

namespace UI
{
	class DeveloperSettings;
	class NetworkSettingsView;





	class DeveloperSettingsPresenter : public Presenter<DeveloperSettings>
	{
	  public:
		PRESENTER_CONSTRUCTOR(DeveloperSettingsPresenter, DeveloperSettings)

		// Actions
		void startHardwareTest();
		void refreshCacheInfo();

		// Trigger GitHub latest release download + upgrade
		void upgradeFromGithubLatest();

	  private:
		void onActivate() override;
	};
} // namespace UI

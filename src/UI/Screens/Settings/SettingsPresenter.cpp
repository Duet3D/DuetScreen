#include "SettingsPresenter.h"
#include "Comm/Usb.h"
#include "SettingsView.h"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include <filesystem>
#include <fstream>

namespace UI
{
	void DeveloperSettingsPresenter::startHardwareTest()
	{
		ZoneScoped;
		auto& hardwareTest = getView()->getHardwareTest();
		hardwareTest.show(true);

		/* Skip the serial number entry screen since the end user won't know this */
		hardwareTest.getPresenter()->m_serialNumber = "unknown";
		hardwareTest.getPresenter()->createLogFile();
		hardwareTest.getPresenter()->nextTest();
	}

	void DeveloperSettingsPresenter::refreshCacheInfo()
	{
		ZoneScoped;
		const auto si = std::filesystem::space("/");

		/* Calculate size of NVS folder */
		auto calcSize = [](const std::filesystem::path& path)
		{
			struct Stats
			{
				std::uintmax_t size = 0;
				size_t fileCount = 0;
			} stats;

			if (!std::filesystem::exists(path))
				return stats;

			for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
			{
				if (entry.is_regular_file())
				{
					stats.size += entry.file_size();
					stats.fileCount++;
				}
			}
			return stats;
		};

		const auto nvsStats = calcSize(NVS_FOLDER);

		getView()->setStorageInfo(si.capacity, si.capacity - si.available);
		getView()->setDuetScreenCacheSize(nvsStats.size, nvsStats.fileCount);
	}

	void DeveloperSettingsPresenter::onActivate()
	{
		ZoneScoped;
		refreshCacheInfo();
	}

	void DeveloperSettingsPresenter::upgradeFromGithubLatest()
	{
		ZoneScoped;
		LOG_INFO("User requested upgrade from GitHub latest release");
		if (!UpgradeHelper::upgradeFromGithubLatest())
		{
			LOG_ERROR("Upgrade from GitHub latest failed to start");
			Model::get().post<EventType::Response>(ResponseType::ERROR, _("settings.upgrade_from_github_failed"));
		}
	}
} // namespace UI

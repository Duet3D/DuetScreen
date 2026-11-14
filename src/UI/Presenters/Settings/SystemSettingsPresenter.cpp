/*
 * SystemSettingsPresenter.cpp
 */

#include "SystemSettingsPresenter.h"
#include "BuildDate.h"
#include "Comm/FileInfo.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Directories.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include "utils/UpgradeHelper.h"
#include "version.h"
#include <filesystem>

namespace UI
{
	SystemSettingsPresenter::SystemSettingsPresenter()
		: SettingsPresenter("settings_system")
	{
#if USE_FIXED_TEST_STRINGS
		m_firmware.set(_("settings.build_time", "vX.Y.Z", "YYYY-MM-DD", "HH:mm::ss"));
#else
		m_firmware.set(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));
#endif
		m_buildroot.set(UpgradeHelper::getBuildrootVersion());

		action("settings.clear_cache",
			   "Delete the cached settings and file information, show the storage used again, and say so",
			   [this](const Xml::ActionArgs&)
			   {
				   if (std::filesystem::exists(NVS_FOLDER))
				   {
					   std::filesystem::remove_all(NVS_FOLDER);
				   }
				   FILEINFO_CACHE->ClearCache();
				   showStorage();
				   Model::get().post<EventType::Response>(ResponseType::SUCCESS, _("settings.cache_cleared"));
			   });
		action("settings.upgrade_from_github",
			   "Upgrade to the latest release on GitHub, reporting an error if it fails to start",
			   [](const Xml::ActionArgs&)
			   {
				   LOG_INFO("User requested upgrade from GitHub latest release");
				   if (!UpgradeHelper::upgradeFromGithubLatest())
				   {
					   LOG_ERROR("Upgrade from GitHub latest failed to start");
					   Model::get().post<EventType::Response>(ResponseType::ERROR,
															  _("settings.upgrade_from_github_failed"));
				   }
			   });
		action("settings.send_config",
			   "Back the screen's settings up to the Duet, as duetscreen.json in its system folder, and say whether "
			   "it worked",
			   [](const Xml::ActionArgs&)
			   {
				   const bool saved =
					   Comm::DUET.UploadFile(fmt::format("{:s}/duetscreen.json", OM::Directories::GetSystemDirectory()),
											 StorageHelper::dump());
				   Model::get().post<EventType::Response>(saved ? ResponseType::SUCCESS : ResponseType::ERROR,
														  saved ? _("settings.backup_saved")
																: _("settings.backup_failed"));
			   });
		action("settings.restart", "Restart the DuetScreen program", [](const Xml::ActionArgs&) { Restart(); });
		action("settings.reboot", "Reboot the screen", [](const Xml::ActionArgs&) { Reboot(); });
		action("settings.erase_and_restart",
			   "Ask whether to erase every setting, the WiFi networks and the layouts made on the screen, then restart",
			   [](const Xml::ActionArgs& args)
			   {
				   Xml::Prompts::askQuestion({.title = _("settings.erase_confirm.title"),
											  .text = _("settings.erase_confirm.text"),
											  .yes = _("settings.erase_confirm.yes"),
											  .no = _("common.cancel"),
											  .onYes = [] { EraseAndRestart(); },
											  .asker = args.target});
			   });
	}

	void SystemSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("firmware", m_firmware, "DuetScreen's version and build time");
		f.field("buildroot", m_buildroot, "Buildroot's version");
		f.field("storage_info", m_storageInfo, "Storage used, e.g. \"256.00 MB / 16.00 GB (1%)\"");
		f.field("cache_size", m_cacheSize, "Size of the cache, e.g. \"10.00 KB (100 files)\"");
	}

	void SystemSettingsPresenter::onActivate()
	{
		showStorage();
	}

	void SystemSettingsPresenter::showStorage()
	{
		ZoneScoped;
		std::uintmax_t total = 0;
		std::uintmax_t used = 0;
		std::uintmax_t cacheSize = 0;
		size_t cacheFiles = 0;
#if USE_FIXED_TEST_STRINGS
		total = 16uz * 1024uz * 1024uz * 1024uz; // 16 GB
		used = 256uz * 1024uz * 1024uz;			 // 256 MB
		cacheSize = 10uz * 1024uz;				 // 10 KB
		cacheFiles = 100u;
#else
		const auto space = std::filesystem::space("/");
		total = space.capacity;
		used = space.capacity - space.available;
		if (std::filesystem::exists(NVS_FOLDER))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(NVS_FOLDER))
			{
				if (entry.is_regular_file())
				{
					cacheSize += entry.file_size();
					cacheFiles++;
				}
			}
		}
#endif
		m_storageInfo.set(_("settings.storage_info_format",
							Units::formatBytes(used),
							Units::formatBytes(total),
							total == 0 ? 0 : used * 100 / total));
		m_cacheSize.set(_("settings.duetscreen_cache_size_format", Units::formatBytes(cacheSize), cacheFiles));
	}
} // namespace UI

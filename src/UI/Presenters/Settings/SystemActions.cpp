/*
 * SystemActions.cpp
 */

#include "SystemActions.h"
#include "Comm/Connection.h"
#include "Comm/FileInfo.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Directories.h"
#include "UI/Core/Model.h"
#include "UI/Presenters/HardwareTest/HardwareTestPresenter.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/SystemSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/SystemHelper.h"
#include "utils/UpgradeHelper.h"
#include <filesystem>
#include <fmt/format.h>
#include <memory>

namespace UI::SystemActions
{
	namespace
	{
		/** Show the hardware test over the screen, without a serial number (the user won't know it); it goes on exit */
		void startHardwareTest()
		{
			ZoneScoped;
			auto presenter = std::make_shared<HardwareTestPresenter>();
			auto* view = new Xml::XmlView("hardware_test", lv_screen_active(), "hardware_test", presenter);
			presenter->setOnExit(
				[view]()
				{
					// Not while its own action is running
					lv_async_call([](void* v) { delete static_cast<Xml::XmlView*>(v); }, view);
				});
			view->show(true);
			presenter->startWithoutSerialNumber();
		}

		/** `actions.system.clear_cache` */
		void clearCache(const Xml::ActionArgs&)
		{
			if (std::filesystem::exists(NVS_FOLDER))
			{
				std::filesystem::remove_all(NVS_FOLDER);
			}
			FILEINFO_CACHE->ClearCache();
			if (auto* system = Xml::DataSources::find<Xml::SystemSource>())
			{
				system->update();
			}
			Model::get().post<EventType::Response>(ResponseType::SUCCESS, _("settings.cache_cleared"));
		}

		/** `actions.system.upgrade_from_github` */
		void upgradeFromGithub(const Xml::ActionArgs&)
		{
			LOG_INFO("User requested upgrade from GitHub latest release");
			if (!UpgradeHelper::upgradeFromGithubLatest())
			{
				LOG_ERROR("Upgrade from GitHub latest failed to start");
				Model::get().post<EventType::Response>(ResponseType::ERROR, _("settings.upgrade_from_github_failed"));
			}
		}

		/** `actions.system.send_config` */
		void sendConfig(const Xml::ActionArgs&)
		{
			Comm::Connection::get().writeFile(
				fmt::format("{:s}/duetscreen.json", OM::Directories::GetSystemDirectory()),
				StorageHelper::dump(),
				[](std::expected<void, Comm::ReplyError> saved)
				{
					Model::get().post<EventType::Response>(saved ? ResponseType::SUCCESS : ResponseType::ERROR,
														   saved ? _("settings.backup_saved")
																 : _("settings.backup_failed"));
				});
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("system.restart",
								"Restart the DuetScreen program",
								"tr.actions.system_restart",
								[](const Xml::ActionArgs&) { Restart(); });
		Xml::Actions::addGlobal(
			"system.reboot", "Reboot the screen", "tr.actions.system_reboot", [](const Xml::ActionArgs&) { Reboot(); });
		Xml::Actions::addGlobal("system.erase_and_restart",
								"Ask whether to erase every setting, the WiFi networks and the layouts made on the "
								"screen, then restart",
								[](const Xml::ActionArgs& args)
								{
									Xml::Prompts::askQuestion({.title = _("settings.erase_confirm.title"),
															   .text = _("settings.erase_confirm.text"),
															   .yes = _("settings.erase_confirm.yes"),
															   .no = _("common.cancel"),
															   .onYes = [] { EraseAndRestart(); },
															   .asker = args.target});
								});
		Xml::Actions::addGlobal("system.clear_cache",
								"Delete the cached settings and file information, show the storage used again, and say "
								"so",
								clearCache);
		Xml::Actions::addGlobal("system.upgrade_from_github",
								"Upgrade to the latest release on GitHub, reporting an error if it fails to start",
								upgradeFromGithub);
		Xml::Actions::addGlobal("system.send_config",
								"Back the screen's settings up to the Duet, as duetscreen.json in its system folder, "
								"and say whether it worked",
								sendConfig);
		Xml::Actions::addGlobal(
			"system.hardware_test", "Open the hardware test", [](const Xml::ActionArgs&) { startHardwareTest(); });
		Xml::Actions::addGlobal("system.buildroot_setup",
								"Restart the system's setup service (developer mode builds)",
								[](const Xml::ActionArgs&)
								{
#if DEVELOPER_MODE
									SystemHelper::restartService(SystemHelper::Services::SETUP);
#else
									LOG_WARN("actions.system.buildroot_setup: only in developer mode builds");
#endif
								});
	}
} // namespace UI::SystemActions

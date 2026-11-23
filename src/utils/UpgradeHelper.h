/*
 * UpgradeHelper.h
 *
 *  Created on: 2025-03-04
 *      Author: Andy Everitt
 */

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace UpgradeHelper
{
	enum class UpgradeResult
	{
		Success = 0,
		BuildrootVersionError,
		BuildrootVersionWarning,
	};

	struct UpgradeInfo
	{
		UpgradeResult result;
		std::string currentVersion;
		std::string currentBuildrootVersion;
		std::string updateBuildrootVersion;
	};

	/** What checking an update archive found: whether the screen's update service would install it */
	struct UpdateCheck
	{
		/** The answer */
		enum class Verdict
		{
			Installable,  // It can be installed
			PatchDiffers, // It can be installed, though made for another patch of the screen's system (Buildroot)
			WrongSystem,  // It was made for another system (Buildroot major.minor): the SD card must be flashed instead
			NotAnUpdate,  // Not a screen update archive, or it can't be read
		};

		Verdict verdict = Verdict::NotAnUpdate; // The answer
		std::string updateBuildrootVersion;		// The system it was made for, if it says
		std::string installedBuildrootVersion;	// The screen's

		/** Whether it can be installed */
		bool installable() const { return verdict == Verdict::Installable || verdict == Verdict::PatchDiffers; }
	};

	/** Whether the file `name` could be an update archive, so is worth checking: "*.tar.gz" */
	bool looksLikeUpdate(std::string_view name);

	/**
	 * Check the update archive at `path` as the screen's update service does before installing it: it holds the
	 * update's scripts, and the system it was made for (its `buildroot_version`, if it says) has the same major and
	 * minor version as `installed`, the screen's (none skips that, as the service does). It is decompressed all
	 * through, so call it away from the UI's thread.
	 */
	UpdateCheck checkUpdate(const std::filesystem::path& path, std::string_view installed);

	/** checkUpdate() against this screen's system; the simulation has none, so only the archive is checked */
	UpdateCheck checkUpdate(const std::filesystem::path& path);

	/**
	 * Install the update archive at `path`, on a USB drive, from a thread of its own: it is copied to the boot
	 * partition and the update service started, which restarts the screen once it is installed (and the screen then
	 * says how it went: EventType::UpdateResult). An empty file, "upgraded", is left beside it, and it isn't offered
	 * again when its drive is plugged in.
	 * @param failed Told, on that thread, why it couldn't be started
	 */
	void installFromUsb(std::string path, std::function<void(std::string why)> failed);

	/** Offer an update (EventType::UpdateAvailable) whenever a USB drive with an installable DuetScreen.tar.gz at its
	 * top is plugged in, unless it was installed already */
	void offerUpdatesOnUsbDrives();

	bool upgradeFromDuet();
	bool upgradeFromGithubLatest();
	[[nodiscard]] std::optional<std::string> checkForUpdate();

	// Start monitoring for upgrade failure/warning files
	void startMonitoringUpgradeStatus();
	std::string_view getBuildrootVersion();
} // namespace UpgradeHelper

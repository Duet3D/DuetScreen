/*
 * test_upgrade_helper.cpp
 *
 * Checking an update archive before installing it (src/utils/UpgradeHelper.h), as the screen's update service does,
 * and listing a folder on a USB drive (OM::FileSystem::ListLocalFolder).
 */

#include "ObjectModel/Files.h"
#include "test_utils/TestSuite.h"
#include "utils/UpgradeHelper.h"
#include <fstream>
#include <gtest/gtest.h>
#include <string>

using UpgradeHelper::UpdateCheck;
using Verdict = UpgradeHelper::UpdateCheck::Verdict;

class TestUpgradeHelper : public TestSuite
{
  protected:
	/** An update archive as scripts/create_upgrade.py makes one, for system `buildroot` (none if empty) */
	std::filesystem::path update(const std::string& buildroot)
	{
		std::vector<std::pair<std::string, std::string>> files = {
			{"pre-update", "#!/bin/sh\n"},
			{"post-update", "#!/bin/sh\nsync\n"},
			{"etc/assets/ui/components/" + std::string(120, 'x') + ".xml", "<component/>"},
			{"usr/bin/DuetScreen", std::string(3000, 'b')},
		};
		if (!buildroot.empty())
		{
			files.emplace_back("buildroot_version", buildroot + "\n");
		}
		const std::filesystem::path path = folder() / "DuetScreen.tar.gz";
		writeArchive(path, files);
		return path;
	}

	/** A folder of the test's own */
	static std::filesystem::path folder()
	{
		const std::filesystem::path path = cacheFolder() / "upgrade";
		std::filesystem::create_directories(path);
		return path;
	}
};

TEST_F(TestUpgradeHelper, AnUpdateForTheSameSystemIsInstallable)
{
	const UpdateCheck check = UpgradeHelper::checkUpdate(update("2025.02.1"), "2025.02.1");
	EXPECT_EQ(check.verdict, Verdict::Installable);
	EXPECT_TRUE(check.installable());
	EXPECT_EQ(check.updateBuildrootVersion, "2025.02.1") << "its line ending left out";
	EXPECT_EQ(check.installedBuildrootVersion, "2025.02.1");
}

TEST_F(TestUpgradeHelper, AnUpdateForAnotherPatchIsInstallableWithAWarning)
{
	const UpdateCheck check = UpgradeHelper::checkUpdate(update("2025.02.2"), "2025.02.1");
	EXPECT_EQ(check.verdict, Verdict::PatchDiffers);
	EXPECT_TRUE(check.installable());
}

TEST_F(TestUpgradeHelper, AnUpdateForAnotherSystemIsNot)
{
	EXPECT_EQ(UpgradeHelper::checkUpdate(update("2025.05.1"), "2025.02.1").verdict, Verdict::WrongSystem);
	EXPECT_EQ(UpgradeHelper::checkUpdate(update("2026.02"), "2025.02.1").verdict, Verdict::WrongSystem);
	EXPECT_FALSE(UpgradeHelper::checkUpdate(update("2025.05.1"), "2025.02.1").installable());
}

TEST_F(TestUpgradeHelper, WithoutAVersionOnEitherSideTheSystemIsntChecked)
{
	// As the update service: an assets-only update (create_upgrade.py --skip-binary), or a screen with no version file
	EXPECT_EQ(UpgradeHelper::checkUpdate(update(""), "2025.02.1").verdict, Verdict::Installable);
	EXPECT_EQ(UpgradeHelper::checkUpdate(update("2025.05.1"), "").verdict, Verdict::Installable);
}

TEST_F(TestUpgradeHelper, AnythingElseIsNotAnUpdate)
{
	// An archive without the update's scripts
	const std::filesystem::path other = folder() / "photos.tar.gz";
	writeArchive(other, {{"holiday.jpg", "jpeg"}});
	EXPECT_EQ(UpgradeHelper::checkUpdate(other, "2025.02.1").verdict, Verdict::NotAnUpdate);

	// Not an archive at all
	const std::filesystem::path text = folder() / "notes.tar.gz";
	std::ofstream(text) << "not an archive";
	EXPECT_EQ(UpgradeHelper::checkUpdate(text, "2025.02.1").verdict, Verdict::NotAnUpdate);

	// Cut short, as a copy that didn't finish
	const std::filesystem::path whole = update("2025.02.1");
	const std::filesystem::path cut = folder() / "cut.tar.gz";
	std::filesystem::copy_file(whole, cut);
	std::filesystem::resize_file(cut, std::filesystem::file_size(whole) / 2);
	EXPECT_EQ(UpgradeHelper::checkUpdate(cut, "2025.02.1").verdict, Verdict::NotAnUpdate);

	EXPECT_EQ(UpgradeHelper::checkUpdate(folder() / "missing.tar.gz", "2025.02.1").verdict, Verdict::NotAnUpdate);
}

TEST_F(TestUpgradeHelper, OnlyTarGzFilesLookLikeUpdates)
{
	EXPECT_TRUE(UpgradeHelper::looksLikeUpdate("DuetScreen.tar.gz"));
	EXPECT_TRUE(UpgradeHelper::looksLikeUpdate("DuetScreen-v1.2.tar.gz"));
	EXPECT_FALSE(UpgradeHelper::looksLikeUpdate(".tar.gz"));
	EXPECT_FALSE(UpgradeHelper::looksLikeUpdate("benchy.gcode"));
	EXPECT_FALSE(UpgradeHelper::looksLikeUpdate("DuetScreen.tar"));
}

TEST_F(TestUpgradeHelper, AFolderOnADriveIsListedWithSizesAndDates)
{
	const std::filesystem::path drive = plugInUsbDrive();
	std::ofstream(drive / "benchy.gcode") << std::string(1500, 'g');
	std::ofstream(drive / ".hidden") << "x";
	std::ofstream(drive / "._benchy.gcode") << "macOS";
	std::filesystem::create_directories(drive / "parts");
	std::filesystem::create_directories(drive / "System Volume Information");

	auto items = OM::FileSystem::ListLocalFolder(drive.string());
	ASSERT_TRUE(items) << items.error();
	OM::FileSystem::SortFilesBy(*items, OM::FileSystem::SortBy::NAME, false);
	ASSERT_EQ(items->size(), 2u) << "hidden files and System Volume Information left out";
	const OM::FileSystem::ItemPtr& file =
		(*items)[0]->GetType() == OM::FileSystem::FileSystemItemType::file ? (*items)[0] : (*items)[1];
	EXPECT_EQ(file->GetName(), "benchy.gcode");
	EXPECT_EQ(file->GetPath(), (drive / "benchy.gcode").string());
	EXPECT_EQ(file->GetSize(), 1500u);
	EXPECT_EQ(file->GetDate().size(), 19u)
		<< "as the printer gives dates, e.g. 2025-03-18T18:21:58: " << file->GetDate();

	EXPECT_FALSE(OM::FileSystem::ListLocalFolder((drive / "missing").string()));
}

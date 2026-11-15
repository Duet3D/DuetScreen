/*
 * test_icons.cpp
 *
 * The icon sets (assets/icons/<set>/, made by scripts/generate_icons.py): every icon the UI names is in every set, so
 * choosing a set never leaves a button without its icon.
 */

#include "UI/Styles/Styles.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include <set>
#include <sstream>
#include <string>

namespace
{
	/** The text of `path` */
	std::string readFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		std::stringstream text;
		text << file.rdbuf();
		return text.str();
	}

	/** Every "<name>.png" quoted in `text` that `pattern` finds (its first group is the file name) */
	void collect(const std::string& text, const std::regex& pattern, std::set<std::string>& names)
	{
		for (auto it = std::sregex_iterator(text.begin(), text.end(), pattern); it != std::sregex_iterator(); ++it)
		{
			names.insert((*it)[1].str());
		}
	}

	/** The icons in the folders that aren't icon sets, named with `folder=` (e.g. the hardware test's target) */
	std::set<std::string> fixedIcons()
	{
		std::set<std::string> names;
		for (const char* folder : {"hardware_test", "examples"})
		{
			for (const auto& entry : std::filesystem::directory_iterator(ASSETS_FOLDER "icons/" + std::string(folder)))
			{
				names.insert(entry.path().filename().string());
			}
		}
		return names;
	}

	/** Every icon the UI names: in its components and widget files, and the icons C++ widgets set themselves */
	std::set<std::string> namedIcons()
	{
		std::set<std::string> names;
		const std::regex quoted("\"([a-z0-9_]+\\.png)\"");
		for (const auto& entry : std::filesystem::recursive_directory_iterator(ASSETS_FOLDER "ui"))
		{
			const auto extension = entry.path().extension();
			if (entry.is_regular_file() && (extension == ".xml" || extension == ".json"))
			{
				collect(readFile(entry.path()), quoted, names);
			}
		}
		const std::regex setIcon("setIcon\\(\"([a-z0-9_]+\\.png)\"\\)");
		for (const auto& entry : std::filesystem::recursive_directory_iterator("src/UI"))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".cpp")
			{
				collect(readFile(entry.path()), setIcon, names);
			}
		}
		// Named at runtime: a WiFi network's signal (src/UI/Presenters/Settings/WifiPresenter.cpp)
		for (int bars = 1; bars <= 4; bars++)
		{
			names.insert("wifi_signal_" + std::to_string(bars) + ".png");
		}
		for (const auto& fixed : fixedIcons())
		{
			names.erase(fixed);
		}
		return names;
	}
} // namespace

TEST(TestIcons, EveryIconTheUiNamesIsInEverySet)
{
	const auto names = namedIcons();
	ASSERT_GT(names.size(), 40u) << "the icons named in the UI weren't found";
	const auto sets = UI::Themes::getIconSets();
	ASSERT_FALSE(sets.empty());
	for (const auto& set : sets)
	{
		for (const auto& name : names)
		{
			EXPECT_TRUE(std::filesystem::exists(UI::Themes::getFixedIconPath(set, name)))
				<< "icon set " << set << " has no " << name;
		}
	}
}

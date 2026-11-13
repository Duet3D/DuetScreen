/*
 * LayoutLibrary.h
 *
 * Layouts the user saved on the screen, to load again later: each a file in the `saved/` folder of the user bundle
 * folder (/boot/ui/saved/ on the screen, var/ui/saved/ in the simulator), holding the layout of every slot:
 *
 *     {
 *       "format": 1,
 *       "name": "Printing",
 *       "layouts": [ {"format": 1, "slot": "dashboard", "kind": "free", …}, {"format": 1, "slot": "sidebar", …} ]
 *     }
 *
 * The file's name is the layout's name, so a name is limited to letters, digits, spaces, `-` and `_`. Files are written
 * safely (see Layouts::writeSafely()). Like everything the user customises, they are only files in the user bundle
 * folder, so copying that folder copies them too.
 */

#pragma once

#include "Layout.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml::LayoutLibrary
{
	/** The current file format */
	inline constexpr int FORMAT = 1;

	/** The longest name, in bytes */
	inline constexpr size_t MAX_NAME = 32;

	/** Keep saved layouts in `dir`; empty for none (saving fails, nothing is listed) */
	void setFolder(std::filesystem::path dir);

	/** Whether layouts can be saved: there is a folder for them */
	bool canSave();

	/** Whether `name` can name a saved layout: 1 to MAX_NAME letters, digits, spaces, `-` and `_`, not all spaces */
	bool isValidName(std::string_view name);

	/** The names of the saved layouts, in name order */
	std::vector<std::string> names();

	/**
	 * Save `layouts` as `name`, replacing any saved layout of that name
	 * @param error Set to what went wrong if false is returned
	 */
	bool save(std::string_view name, const std::vector<Layout>& layouts, std::string& error);

	/**
	 * The layouts saved as `name`, as they were saved (not repaired: that is for when they are shown)
	 * @param error Set to what went wrong if nullopt is returned
	 */
	std::optional<std::vector<Layout>> load(std::string_view name, std::string& error);

	/** Delete the layout saved as `name`. @return whether there was one to delete */
	bool remove(std::string_view name);
} // namespace UI::Xml::LayoutLibrary

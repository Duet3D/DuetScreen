/*
 * Directories.h
 *
 *  Created on: 2025-04-09
 *      Author: Andy Everitt
 *
 * The printer's folders, as its object model gives them (`directories`, e.g. "0:/gcodes/"): where jobs, macros,
 * filaments and system files are. Read from the object model (Tree.h) each time; empty until the printer sends them.
 */

#pragma once

#include <string>

namespace OM::Directories
{
	/** A folder of the printer's */
	enum class DirectoryType
	{
		FILAMENTS,
		FIRMWARE,
		GCODES,
		MACROS,
		MENU,
		SYSTEM,
		WEB
	};

	/** The folder of kind `type`, e.g. "0:/gcodes/"; empty if the printer hasn't sent it */
	std::string GetDirectory(DirectoryType type);

	/** The jobs' folder */
	std::string GetGcodesDirectory();

	/** The system folder */
	std::string GetSystemDirectory();
} // namespace OM::Directories

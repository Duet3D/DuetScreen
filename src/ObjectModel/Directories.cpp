/*
 * Directories.cpp
 *
 *  Created on: 2025-04-09
 *      Author: Andy Everitt
 */

#include "Directories.h"
#include "Debug.h"
#include "LockWrapper.h"
#include "Tree/Tree.h"
#include <mutex>

namespace OM::Directories
{
	std::string GetDirectory(DirectoryType type)
	{
		const char* field = nullptr;
		switch (type)
		{
		case DirectoryType::FILAMENTS:
			field = "filaments";
			break;
		case DirectoryType::FIRMWARE:
			field = "firmware";
			break;
		case DirectoryType::GCODES:
			field = "gCodes";
			break;
		case DirectoryType::MACROS:
			field = "macros";
			break;
		case DirectoryType::MENU:
			field = "menu";
			break;
		case DirectoryType::SYSTEM:
			field = "system";
			break;
		case DirectoryType::WEB:
			field = "web";
			break;
		}
		if (field == nullptr)
		{
			LOG_ERROR("Invalid directory type");
			return "";
		}
		std::lock_guard lock(mutexModel);
		return std::string(Tree::get().root()["directories"][field].text());
	}

	std::string GetGcodesDirectory()
	{
		return GetDirectory(DirectoryType::GCODES);
	}

	std::string GetSystemDirectory()
	{
		return GetDirectory(DirectoryType::SYSTEM);
	}
} // namespace OM::Directories

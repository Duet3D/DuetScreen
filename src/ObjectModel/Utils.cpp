/*
 * Utils.cpp
 *
 *  Created on: 7 Sep 2020
 *      Author: manuel
 */
#include "Utils.h"
#include "Debug.h"
#include "DebugCommands.h"
#include "Files.h"
#include "Heightmap.h"
#include "LockWrapper.h"
#include "PrinterStatus.h"
#include "Tree/Tree.h"
#include <mutex>

namespace OM
{
	void RemoveAll()
	{
		ClearHeightmapCache();
		FileSystem::ClearFileSystem();
		SetPrinterUniqueId("");
		Tree::get().clear();
	}

	static Debug::DebugCommand s_logOM("dbg_log_OM",
									   []()
									   {
										   std::lock_guard lock(mutexModel);
										   LOG_INFO("ObjectModel: {:s}", Tree::get().root().toJson());
									   });
} // namespace OM

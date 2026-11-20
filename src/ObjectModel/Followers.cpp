/*
 * Followers.cpp
 */

#include "Followers.h"
#include "Configuration.h"
#include "Debug.h"
#include "LockWrapper.h"
#include "PrinterStatus.h"
#include "Tree/Tree.h"
#include "tracy/Tracy.hpp"
#include "utils/TimeHelper.h"
#include <mutex>
#include <string>

namespace OM::Followers
{
	namespace
	{
		/** The text at `path` in the object model, read under the model lock */
		std::string textAt(std::string_view path)
		{
			ZoneScoped;
			std::lock_guard lock(mutexModel);
			return std::string(Tree::get().root().at(path).text());
		}

		/** The printer's status changed */
		void statusChanged()
		{
			ZoneScoped;
			const std::string status = textAt("state.status");
			if (!status.empty())
			{
				SetStatus(status.c_str());
			}
		}

		/** The printer's main board changed: it may be another printer */
		void boardChanged()
		{
			ZoneScoped;
			SetPrinterUniqueId(textAt("boards[0].uniqueId").c_str());
		}

		/** The printer's clock changed: set the screen's from it, now and then */
		void timeChanged()
		{
			ZoneScoped;
			static std::chrono::milliseconds s_lastSet = std::chrono::milliseconds(0);
			const std::string time = textAt("state.time");
			const auto now = TimeHelper::getRunningTime();
			if (time.empty() || (s_lastSet.count() != 0 && now - s_lastSet < TIME_SYNC_INTERVAL))
			{
				return;
			}
			LOG_DBG("Setting system time to {:s}", time);
			TimeHelper::setDateTime(time.c_str());
			s_lastSet = now;
		}

		/** The input channel the screen uses changed */
		void inputChanged()
		{
			ZoneScoped;
			std::lock_guard lock(mutexModel);
			const int64_t channel = Tree::get().root().at("state.thisInput").integer(-1);
			if (channel >= 0)
			{
				SetChannelIndex(static_cast<uint32_t>(channel));
			}
		}
	} // namespace

	void refresh()
	{
		ZoneScoped;
		statusChanged();
		boardChanged();
		timeChanged();
		inputChanged();
	}

	void start()
	{
		ZoneScoped;
		static bool s_started = false;
		if (s_started)
		{
			return;
		}
		s_started = true;
		auto& tree = Tree::get();
		tree.watch({"state.status"}, statusChanged);
		tree.watch({"boards"}, boardChanged);
		tree.watch({"state.time"}, timeChanged);
		tree.watch({"state.thisInput"}, inputChanged);
		// The printer's folders: file paths are made from them (Directories.h), whatever the UI shows
		tree.want({"directories"});
	}
} // namespace OM::Followers

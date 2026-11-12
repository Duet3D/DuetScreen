/*
 * HotReload.cpp
 */

#include "HotReload.h"
#include "Debug.h"
#include "XmlUi.h"
#include "lvgl/lvgl.h"
#include <map>

namespace UI::Xml::HotReload
{
	static lv_timer_t* s_timer = nullptr;											   // Polling timer, or nullptr
	static std::map<std::filesystem::path, std::filesystem::file_time_type> s_watched; // Files and their times

	/** Timer callback: reload if a watched file changed */
	static void check(lv_timer_t*)
	{
		for (const auto& [path, stamp] : s_watched)
		{
			std::error_code ec;
			if (std::filesystem::last_write_time(path, ec) != stamp)
			{
				LOG_INFO("UI bundle file changed: {:s}", path.string());
				reload(); // Calls watch() with the new set of files
				return;
			}
		}
	}

	void setPeriod(uint32_t periodMs)
	{
		if (s_timer)
		{
			lv_timer_delete(s_timer);
			s_timer = nullptr;
		}
		if (periodMs > 0)
		{
			s_timer = lv_timer_create(check, periodMs, nullptr);
		}
	}

	void watch(const std::vector<std::filesystem::path>& files)
	{
		s_watched.clear();
		for (const auto& file : files)
		{
			std::error_code ec;
			s_watched[file] = std::filesystem::last_write_time(file, ec);
		}
	}
} // namespace UI::Xml::HotReload

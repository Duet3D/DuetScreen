/*
 * SystemSource.cpp
 */

#include "SystemSource.h"
#include "BuildDate.h"
#include "Configuration.h"
#include "Debug.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
#include "utils/UpgradeHelper.h"
#include "version.h"
#include <filesystem>

namespace UI::Xml
{
	SystemSource::SystemSource()
		: DataSource("system")
	{
#if USE_FIXED_TEST_STRINGS
		m_firmware.set(_("settings.build_time", "vX.Y.Z", "YYYY-MM-DD", "HH:mm::ss"));
#else
		m_firmware.set(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));
#endif
		m_buildroot.set(UpgradeHelper::getBuildrootVersion());
		m_debugBordersShown.set(DEBUG_BORDERS ? 1 : 0);
		m_developerMode.set(DEVELOPER_MODE ? 1 : 0);
		m_systemMonitorShown.set(LV_USE_SYSMON ? 1 : 0);
		m_buildToolsShown.set(DEBUG_BORDERS || DEVELOPER_MODE || LV_USE_SYSMON ? 1 : 0);
		m_simulatorShown.set(DUET_SIMULATOR ? 1 : 0);
	}

	void SystemSource::fields(FieldVisitor& f)
	{
		f.field("firmware", m_firmware, "DuetScreen's version and build time");
		f.field("buildroot", m_buildroot, "Buildroot's version");
		f.field("storage_info", m_storageInfo, "Storage used, e.g. \"256.00 MB / 16.00 GB (1%)\"");
		f.field("cache_size", m_cacheSize, "Size of the cache, e.g. \"10.00 KB (100 files)\"");
		f.field("debug_borders_shown", m_debugBordersShown, "1 if the build has debug borders");
		f.field("developer_mode", m_developerMode, "1 in developer mode builds (SSH, ADB, setup)");
		f.field("system_monitor_shown", m_systemMonitorShown, "1 if the build has the system monitor");
		f.field("build_tools_shown",
				m_buildToolsShown,
				"1 if the build has any of debug borders, developer mode or the system monitor");
		f.field("simulator_shown",
				m_simulatorShown,
				"1 if the build has the simulated Duet (a connection method, and its page in the settings)");
	}

	void SystemSource::update()
	{
		ZoneScoped;
		std::uintmax_t total = 0;
		std::uintmax_t used = 0;
		std::uintmax_t cacheSize = 0;
		size_t cacheFiles = 0;
#if USE_FIXED_TEST_STRINGS
		total = 16uz * 1024uz * 1024uz * 1024uz; // 16 GB
		used = 256uz * 1024uz * 1024uz;			 // 256 MB
		cacheSize = 10uz * 1024uz;				 // 10 KB
		cacheFiles = 100u;
#else
		const auto space = std::filesystem::space("/");
		total = space.capacity;
		used = space.capacity - space.available;
		if (std::filesystem::exists(NVS_FOLDER))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(NVS_FOLDER))
			{
				if (entry.is_regular_file())
				{
					cacheSize += entry.file_size();
					cacheFiles++;
				}
			}
		}
#endif
		m_storageInfo.set(_("settings.storage_info_format",
							Units::formatBytes(used),
							Units::formatBytes(total),
							total == 0 ? 0 : used * 100 / total));
		m_cacheSize.set(_("settings.duetscreen_cache_size_format", Units::formatBytes(cacheSize), cacheFiles));
	}
} // namespace UI::Xml

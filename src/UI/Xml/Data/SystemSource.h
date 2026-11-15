/*
 * SystemSource.h
 *
 * Publishes facts about the screen itself: the software's versions, the storage used and the cache's size, and which
 * build tools this build has (so the settings for them are shown only where they do something). The global actions
 * `actions.system.*` act on the screen (src/UI/Presenters/Settings/SystemActions.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** Publishes `source.system.*` */
	class SystemSource : public DataSource
	{
	  public:
		/** Create the source, with the versions and build tools. Nothing is published until DataSources::start(). */
		SystemSource();

		/** Declare `source.system.*` */
		void fields(FieldVisitor& f) override;

		/** Show the storage used and the cache's size as they are now */
		void update() override;

		/** Nothing here depends on the printer */
		void clear() override {}

	  private:
		StringSubject<96> m_firmware;	  // `firmware`
		StringSubject<64> m_buildroot;	  // `buildroot`
		StringSubject<64> m_storageInfo;  // `storage_info`
		StringSubject<64> m_cacheSize;	  // `cache_size`
		IntSubject m_debugBordersShown;	  // `debug_borders_shown`
		IntSubject m_developerMode;		  // `developer_mode`
		IntSubject m_systemMonitorShown;  // `system_monitor_shown`
		IntSubject m_buildToolsShown;	  // `build_tools_shown`
		IntSubject m_simulatorShown;	  // `simulator_shown`
	};
} // namespace UI::Xml

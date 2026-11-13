/*
 * DeveloperSettingsPresenter.h
 *
 * Behaviour for the Developer settings tab (assets/ui/screens/settings_developer.xml): logging and diagnostic
 * options, restarting, the hardware test, the cache, upgrades, and the storage used. Each setting is read when the tab
 * is shown and applied as soon as it is changed.
 *
 * Fields
 *   debug_levels              the log levels, one per line; debug_level is the one in use
 *   advanced_settings         1 if advanced settings are shown
 *   second_usb_channel        1 if the second USB channel is used
 *   system_logging            1 if system messages are logged to the console
 *   debug_borders_shown, developer_mode, system_monitor_shown
 *                             1 if the build has debug borders, developer mode (SSH, ADB, setup) or the system monitor
 *   debug_borders, ssh, adb, system_monitor   1 if on
 *   storage_info              e.g. "256.00 MB / 16.00 GB (1%)"
 *   cache_size                e.g. "10.00 KB (100 files)"
 *
 * Actions (the value from the widget: an index, or 1/0 for a toggle)
 *   settings.debug_level, settings.advanced_settings, settings.second_usb_channel, settings.system_logging,
 *   settings.system_monitor
 *   settings.restart, settings.erase_and_restart, settings.reboot, settings.hardware_test, settings.clear_cache,
 *   settings.upgrade_from_github, settings.send_config
 *   settings.debug_borders                                (builds with debug borders)
 *   settings.ssh, settings.adb, settings.buildroot_setup   (developer mode)
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <memory>

namespace UI
{
	class HardwareTest;

	class DeveloperSettingsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		DeveloperSettingsPresenter();

		/** Stop the hardware test if it is showing */
		~DeveloperSettingsPresenter() override;

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the settings and storage as they are now */
		void onActivate() override;

	  private:
		/** Register an action that applies the 1/0 a toggle gives with `apply`, and shows it in `shown` */
		void toggle(std::string name, Xml::IntSubject& shown, std::function<void(bool on)> apply);

		/** Show the storage used and the size of the cache */
		void showStorage();

		/** Start the hardware test, without asking for a serial number */
		void startHardwareTest();

		/** The hardware test: a C++ view of its own (src/UI/Widgets/HardwareTest) until it is written in XML */
		std::unique_ptr<HardwareTest> m_hardwareTest;

		Xml::StringSubject<256> m_debugLevels;		  // Log levels offered
		Xml::IntSubject m_debugLevel;				  // Index of the one in use
		Xml::IntSubject m_advancedSettings;			  // 1 if advanced settings are shown
		Xml::IntSubject m_secondUsbChannel;			  // 1 if the second USB channel is used
		Xml::IntSubject m_systemLogging;			  // 1 if system messages are logged
		Xml::IntSubject m_debugBordersShown;		  // 1 if the build has debug borders
		Xml::IntSubject m_debugBorders;				  // 1 if they are on
		Xml::IntSubject m_developerMode;			  // 1 in developer mode
		Xml::IntSubject m_ssh;						  // 1 if SSH is on
		Xml::IntSubject m_adb;						  // 1 if ADB is on
		Xml::IntSubject m_systemMonitorShown;		  // 1 if the build has the system monitor
		Xml::IntSubject m_systemMonitor;			  // 1 if it is on
		Xml::StringSubject<96> m_storageInfo;		  // Storage used
		Xml::StringSubject<96> m_cacheSize;			  // Cache size
	};
} // namespace UI

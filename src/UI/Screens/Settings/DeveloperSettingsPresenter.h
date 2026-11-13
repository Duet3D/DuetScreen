/*
 * DeveloperSettingsPresenter.h
 *
 * Behaviour for the Developer settings tab (assets/ui/components/settings/settings_developer.xml): logging and
 * diagnostic options, restarting, the hardware test, the cache, upgrades, and the storage used. Each setting is read
 * when the tab is shown and applied as soon as it is changed.
 *
 * There is one action per setting, given the value from the widget (an index, or 1/0 for a toggle), and one per
 * command. Every build has them all, so the UI API is the same in every build: settings.debug_borders only does
 * something in builds with debug borders (`debug_borders_shown`), and settings.ssh, settings.adb and
 * settings.buildroot_setup only in developer mode builds (`developer_mode`).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <memory>

namespace UI
{
	namespace Xml
	{
		class XmlView;
	}

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
		/** Register an action, described by `description`, that applies the 1/0 a toggle gives with `apply`, and shows
		 * it in `shown` */
		void toggle(std::string name,
					std::string description,
					Xml::IntSubject& shown,
					std::function<void(bool on)> apply);

		/** Show the storage used and the size of the cache */
		void showStorage();

		/** Start the hardware test, without asking for a serial number */
		void startHardwareTest();

		/** The hardware test (hardware_test.xml) over the whole screen, once it has been opened */
		std::unique_ptr<Xml::XmlView> m_hardwareTest;

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

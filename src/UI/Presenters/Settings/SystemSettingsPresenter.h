/*
 * SystemSettingsPresenter.h
 *
 * Behaviour for the System settings page (assets/ui/components/settings/settings_system.xml): the versions, the
 * storage used and the cache; updating DuetScreen and backing its settings up to the Duet; restarting and rebooting
 * the screen; and erasing every setting. Clearing the cache and backing up say when they are done; erasing asks
 * first, as it can't be undone. Restarting and rebooting don't: they only restart the screen, in seconds.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "SettingsPresenter.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI
{
	class SystemSettingsPresenter : public SettingsPresenter
	{
	  public:
		/** Create the presenter */
		SystemSettingsPresenter();

		/** Declare the page's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the storage as it is now */
		void onActivate() override;

	  private:
		/** Show the storage used and the size of the cache */
		void showStorage();

		Xml::StringSubject<96> m_firmware;	  // DuetScreen's version and build time
		Xml::StringSubject<64> m_buildroot;	  // Buildroot's version
		Xml::StringSubject<96> m_storageInfo; // Storage used
		Xml::StringSubject<96> m_cacheSize;	  // Cache size
	};
} // namespace UI

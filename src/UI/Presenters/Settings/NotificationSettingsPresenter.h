/*
 * NotificationSettingsPresenter.h
 *
 * Behaviour for the Notifications settings page (assets/ui/components/settings/settings_notifications.xml): which of
 * the Duet's messages show, and how long they stay. Each setting is read when the page is shown and stored as soon as
 * it is changed.
 *
 * There is one action per setting, given the value from the widget (an index, a number of seconds, or 1/0 for a
 * switch).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "SettingsPresenter.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI
{
	class NotificationSettingsPresenter : public SettingsPresenter
	{
	  public:
		/** Create the presenter */
		NotificationSettingsPresenter();

		/** Declare the page's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the settings as they are now */
		void onActivate() override;

	  private:
		Xml::IntSubject m_connectedMessage;			  // 1 if a message shows when the Duet connects
		Xml::StringSubject<128> m_levels;			  // Levels offered
		Xml::IntSubject m_level;					  // Index of the lowest level shown
		Xml::IntSubject m_timeout;					  // ms before a message closes itself
		Xml::IntSubject m_errorsStay;				  // 1 if errors stay until closed
	};
} // namespace UI

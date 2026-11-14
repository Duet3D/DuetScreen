/*
 * NotificationSettingsPresenter.cpp
 */

#include "NotificationSettingsPresenter.h"
#include "Storage.h"
#include "Subscribers/ResponseSubscribers.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"

namespace UI
{
	namespace
	{
		/** ms in a second: the timeout is stored in ms and set in seconds */
		constexpr float MS_PER_SECOND = 1000.0f;
	} // namespace

	NotificationSettingsPresenter::NotificationSettingsPresenter()
		: SettingsPresenter("settings_notifications")
	{
		std::string levels;
		for (const auto& level : RESPONSE_TYPE_STRINGS)
		{
			levels += levels.empty() ? "" : "\n";
			levels += _(level);
		}
		m_levels.set(levels);

		toggle("settings.connected_message",
			   "Show a message when the Duet connects if the edited value is 1, else not",
			   m_connectedMessage,
			   [](bool on) { StorageHelper::setData(ID_DISPLAY_CONNECTED_MESSAGE, on); });
		setting("settings.notification_level",
				"Show messages from the level at the edited index of `notification_levels` up",
				[this](float value)
				{
					m_level.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_NOTIFICATION_LEVEL, ResponseType(static_cast<int>(value)));
				});
		setting("settings.notification_timeout",
				"Close messages after the edited value, in seconds",
				[this](float seconds)
				{
					const auto ms = static_cast<int32_t>(seconds * MS_PER_SECOND);
					m_timeout.set(ms);
					StorageHelper::setData(ID_NOTIFICATION_TIMEOUT, std::chrono::milliseconds(ms));
				});
		toggle("settings.error_notifications_stay",
			   "Keep errors open until closed if the edited value is 1, else close them as other messages",
			   m_errorsStay,
			   [](bool on) { StorageHelper::setData(ID_NOTIFICATION_AUTO_CLOSE_ERROR, !on); });
	}

	void NotificationSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("connected_message", m_connectedMessage, "1 if a message shows when the Duet connects");
		f.field("notification_levels", m_levels, "Message levels, one per line");
		f.field("notification_level", m_level, "Index of the lowest level shown, in `notification_levels`");
		f.field("notification_timeout", m_timeout, "ms before a message closes itself");
		f.field("error_notifications_stay", m_errorsStay, "1 if errors stay until closed");
	}

	void NotificationSettingsPresenter::onActivate()
	{
		m_connectedMessage.set(StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE) ? 1 : 0);
		m_level.set(static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_LEVEL)));
		m_timeout.set(static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_TIMEOUT).count()));
		m_errorsStay.set(StorageHelper::getData(ID_NOTIFICATION_AUTO_CLOSE_ERROR) ? 0 : 1);
	}
} // namespace UI

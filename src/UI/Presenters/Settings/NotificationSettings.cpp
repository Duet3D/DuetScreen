/*
 * NotificationSettings.cpp
 */

#include "NotificationSettings.h"
#include "Storage.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <chrono>
#include <string>
#include <vector>

namespace UI::NotificationSettings
{
	void registerAll()
	{
		Xml::Settings::add(
			{.name = "connected_message",
			 .description = "1 if a message shows when the Duet connects",
			 .read = [] { return StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE) ? 1 : 0; },
			 .write = [](int32_t on) { StorageHelper::setData(ID_DISPLAY_CONNECTED_MESSAGE, on != 0); }});
		Xml::Settings::add(
			{.name = "notification_level",
			 .description = "Index of the lowest level of message shown",
			 .read = [] { return static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_LEVEL)); },
			 .write = [](int32_t level) { StorageHelper::setData(ID_NOTIFICATION_LEVEL, ResponseType(level)); },
			 .options =
				 []
			 {
				 std::vector<std::string> names;
				 for (const auto& level : RESPONSE_TYPE_STRINGS)
				 {
					 names.push_back(_(level));
				 }
				 return Xml::Settings::lines(names);
			 }});
		Xml::Settings::add(
			{.name = "notification_timeout",
			 .description = "ms before a message closes itself",
			 .read = [] { return static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_TIMEOUT).count()); },
			 .write = [](int32_t ms)
			 { StorageHelper::setData(ID_NOTIFICATION_TIMEOUT, std::chrono::milliseconds(ms)); },
			 .scale = 1000});
		Xml::Settings::add(
			{.name = "error_notifications_stay",
			 .description = "1 if errors stay until closed",
			 .read = [] { return StorageHelper::getData(ID_NOTIFICATION_AUTO_CLOSE_ERROR) ? 0 : 1; },
			 .write = [](int32_t on) { StorageHelper::setData(ID_NOTIFICATION_AUTO_CLOSE_ERROR, on == 0); }});
	}
} // namespace UI::NotificationSettings

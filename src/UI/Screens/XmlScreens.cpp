/*
 * XmlScreens.cpp
 */

#include "XmlScreens.h"
#include "Console/ConsoleView.h"
#include "Control/ControlView.h"
#include "Fan/FanPresenter.h"
#include "File/FilesScreen.h"
#include "Notifications/NotificationsView.h"
#include "Settings/SettingsView.h"
#include "StatusBar/StatusBarPresenter.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Widgets/Control/ExtrusionFactor/ExtrusionFactor.h"
#include "UI/Widgets/Dashboard/Dashboard.h"
#include "UI/Widgets/SideBar/SideBar.h"
#include "UI/Xml/View/NativeViews.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	/** The on-screen keyboard: along the bottom of the screen, hidden when cancelled */
	static LvObj* createKeyboard(const std::string& name, lv_obj_t* parent)
	{
		LvObjRef parentRef(parent);
		auto* keyboard = new LvKeyboard(name, parentRef);
		keyboard->hide();
		keyboard->setFlag(LV_OBJ_FLAG_FLOATING, true);
		keyboard->setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
		keyboard->setSize(LV_PCT(100), LV_PCT(50));
		keyboard->addEventCallback(
			[keyboard](lv_event_t*)
			{
				keyboard->hide();
				if (auto textarea = keyboard->getTextarea())
				{
					textarea->sendEvent(LV_EVENT_DEFOCUSED);
				}
			},
			LV_EVENT_CANCEL);
		return keyboard;
	}

	void registerXmlScreens()
	{
		/* Presenters */
		Xml::Presenters::add<FanPresenter>("fans");
		Xml::Presenters::add<StatusBarPresenter>("status_bar");

		/* C++ views, until they are written in XML */
		Xml::NativeViews::add<LvContainer>("container");
		Xml::NativeViews::add<SideBar>("side_bar");
		Xml::NativeViews::add<Dashboard>("dashboard");
		Xml::NativeViews::add<ConsoleView>("console");
		Xml::NativeViews::add<ControlView>("control");
		Xml::NativeViews::add<FilesScreen>("files");
		Xml::NativeViews::add<SettingsView>("settings");
		Xml::NativeViews::add<NotificationsView>("notifications");
		Xml::NativeViews::add<ModalExtrusionFactor>("extrusion_factor");
		Xml::NativeViews::add<ModalNumberPad>("numberpad");
		Xml::NativeViews::add("keyboard", createKeyboard);
	}
} // namespace UI

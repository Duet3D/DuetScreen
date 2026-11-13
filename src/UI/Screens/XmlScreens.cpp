/*
 * XmlScreens.cpp
 */

#include "XmlScreens.h"
#include "Console/ConsolePresenter.h"
#include "Dashboard/ToolListPresenter.h"
#include "Fan/FanPresenter.h"
#include "File/FileBrowserPresenter.h"
#include "Heightmap/HeightmapView.h"
#include "Move/MoveView.h"
#include "Notifications/NotificationsPresenter.h"
#include "ObjectCancel/ObjectCancelView.h"
#include "Settings/SettingsView.h"
#include "Status/JobStatusPresenter.h"
#include "StatusBar/StatusBarPresenter.h"
#include "Temperature/TemperatureView.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Widgets/BabyStep/BabyStep.h"
#include "UI/Widgets/Control/ExtrusionFactor/ExtrusionFactor.h"
#include "UI/Widgets/Control/SpeedFactor/SpeedFactor.h"
#include "UI/Widgets/EStop/EStop.h"
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

	/** A dialog of type M, 80% wide and as tall as it needs */
	template <typename M>
	static LvObj* createFactorDialog(const std::string& name, lv_obj_t* parent)
	{
		LvObjRef parentRef(parent);
		auto* dialog = new M(name, parentRef);
		dialog->setSize(LV_PCT(80), LV_SIZE_CONTENT);
		return dialog;
	}

	/** The babystepping dialog, typing values on the shared number pad */
	static LvObj* createBabyStep(const std::string& name, lv_obj_t* parent)
	{
		LvObjRef parentRef(parent);
		auto* dialog = new Modal<BabyStep>(name, parentRef);
		dialog->setSize(LV_PCT(40), LV_PCT(70));
		dialog->setNumberPad(findView<ModalNumberPad>("numberpad"));
		return dialog;
	}

	/** Make a file browser presenter with `settings` available as `name` */
	static void addFileBrowser(std::string name, FileBrowserPresenter::Settings settings)
	{
		Xml::Presenters::add(name,
							 [name, settings]() { return std::make_shared<FileBrowserPresenter>(name, settings); });
	}

	void registerXmlScreens()
	{
		/* Presenters */
		Xml::Presenters::add<FanPresenter>("fans");
		Xml::Presenters::add<StatusBarPresenter>("status_bar");
		Xml::Presenters::add<ToolListPresenter>("tool_list");
		Xml::Presenters::add<JobStatusPresenter>("job_status");
		Xml::Presenters::add<NotificationsPresenter>("notifications");
		Xml::Presenters::add<ConsolePresenter>("console");

		/* The file browsers (file_browser.xml): one per placement, each remembering its own order and display */
		using Browser = FileBrowserPresenter;
		using SortBy = Browser::SortBy;
		using Mode = Browser::DisplayMode;
		addFileBrowser("dashboard_jobs",
					   {.folder = Browser::BaseFolder::GCODES,
						.sortBy = {"ui:dashboard:file:jobs:sort_by", SortBy::DATE},
						.sortDescending = {"ui:dashboard:file:jobs:sort_descending", true},
						.displayMode = {"ui:dashboard:file:jobs:display_mode", Mode::List}});
		addFileBrowser("dashboard_macros",
					   {.folder = Browser::BaseFolder::MACROS,
						.sortBy = {"ui:dashboard:file:macros:sort_by", SortBy::NAME},
						.sortDescending = {"ui:dashboard:file:macros:sort_descending", false},
						.displayMode = {"ui:dashboard:file:macros:display_mode", Mode::List}});
		addFileBrowser("files_macros",
					   {.folder = Browser::BaseFolder::MACROS,
						.sortBy = {"ui:home:file:macros:sort_by", SortBy::NAME},
						.sortDescending = {"ui:home:file:macros:sort_descending", false},
						.displayMode = {"ui:home:file:macros:display_mode", Mode::List}});
		addFileBrowser("files_jobs",
					   {.folder = Browser::BaseFolder::GCODES,
						.sortBy = {"ui:home:file:jobs:sort_by", SortBy::DATE},
						.sortDescending = {"ui:home:file:jobs:sort_descending", true},
						.displayMode = {"ui:home:file:jobs:display_mode", Mode::Tile}});

		/* C++ views, until they are written in XML */
		Xml::NativeViews::add<LvContainer>("container");
		Xml::NativeViews::add<EStop>("estop");
		Xml::NativeViews::add<MoveView>("move");
		Xml::NativeViews::add<TemperatureView>("temperature");
		Xml::NativeViews::add<HeightmapView>("heightmap");
		Xml::NativeViews::add<ObjectCancelView>("object_cancel");
		Xml::NativeViews::add<SettingsView>("settings");
		Xml::NativeViews::add<ModalExtrusionFactor>("extrusion_factor");
		Xml::NativeViews::add<ModalNumberPad>("numberpad");
		Xml::NativeViews::add("speed_factor", createFactorDialog<ModalSpeedFactor>);
		Xml::NativeViews::add("job_extrusion_factor", createFactorDialog<ModalExtrusionFactor>);
		Xml::NativeViews::add("babystep", createBabyStep);
		Xml::NativeViews::add("keyboard", createKeyboard);
	}
} // namespace UI

/*
 * XmlUi.cpp
 */

#include "XmlUi.h"
#include "Debug.h"
#include "GlobalActions.h"
#include "HotReload.h"
#include "ReloadState.h"
#include "Services.h"
#include "Subscribers/ResponseSubscribers.h"
#include "Translations.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/Binding/Globals.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Theme/ThemeStyles.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "UI/Xml/Widgets/Widgets.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include <algorithm>

#ifndef ASSETS_FOLDER
#  error "ASSETS_FOLDER must be defined (UI/Styles/Styles.h)"
#endif

namespace UI::Xml
{
	namespace
	{
		bool s_initialized = false;	   // Between init() and deinit()
		Paths s_paths;				   // Where bundles are loaded from
		Bundles::Result s_loaded;	   // What the last load produced
		std::vector<XmlView*> s_views; // Live views, rebuilt on reload

		/** Load every bundle, and report problems on screen */
		void loadBundles()
		{
			ZoneScoped;
			s_loaded = Bundles::loadAll(s_paths);
			HotReload::watch(s_loaded.files);

			// Themes can only be checked once all are loaded: a theme's parent may come from another bundle
			std::vector<std::pair<std::filesystem::path, std::string>> themeProblems;
			Themes::setThemeFiles(std::move(s_loaded.themes), &themeProblems);
			for (auto& [file, message] : themeProblems)
			{
				s_loaded.issues.push_back({file, std::move(message)});
			}

			for (const auto& issue : s_loaded.issues)
			{
				Model::get().post<EventType::Response>(
					ResponseType::WARNING,
					fmt::format("UI: {:s}: {:s}", issue.file.filename().string(), issue.message));
			}
		}

		/** After a language change: new translations for XML, and re-read data whose text is translated */
		void onLanguageChanged()
		{
			UI_LOCK();
			Translations::sync();
			DataSources::updateAll();
		}
	} // namespace

	Paths defaultPaths()
	{
#if T113
		return Paths{.builtin = ASSETS_FOLDER "ui", .user = "/boot/ui"};
#else
		return Paths{.builtin = ASSETS_FOLDER "ui", .user = "var/ui"};
#endif
	}

	void init(const Paths& paths)
	{
		ZoneScoped;
		UI_LOCK();
		if (s_initialized)
		{
			LOG_WARN("XML UI already initialised");
			return;
		}

		s_paths = paths;
		lv_xml_init();
		ThemeStyles::registerAll();
		Translations::sync();
		Actions::init();
		Widgets::registerAll();
		GlobalActions::registerAll();
		DataSources::start();
		i18n::setLanguageChangedCallback(onLanguageChanged);

		s_initialized = true;
		loadBundles();
	}

	void deinit()
	{
		ZoneScoped;
		UI_LOCK();
		if (!s_initialized)
		{
			return;
		}
		if (!s_views.empty())
		{
			LOG_ERROR("XML UI stopped with {:d} live view(s)", s_views.size());
		}

		HotReload::setPeriod(0);
		HotReload::watch({});
		i18n::setLanguageChangedCallback(nullptr);
		DataSources::stop();
		Themes::clearThemes();
		Globals::clear();
		Actions::deinit();
		lv_xml_deinit();
		Translations::reset();
		Services::setGcodeSender(nullptr);

		s_loaded = {};
		s_initialized = false;
	}

	bool isInitialized()
	{
		return s_initialized;
	}

	namespace
	{
		/** Whether `view`'s widgets are inside another XmlView's */
		bool isInsideAnotherView(const XmlView& view)
		{
			for (lv_obj_t* obj = lv_obj_get_parent(view.getRootPtr()); obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				if (dynamic_cast<XmlView*>(LvObj::fromPtr(obj)) != nullptr)
				{
					return true;
				}
			}
			return false;
		}
	} // namespace

	void reload()
	{
		ZoneScoped;
		UI_LOCK();
		if (!s_initialized)
		{
			return;
		}
		// Only the outermost views: taking one down deletes the views inside it (its <ds_view>s), and building it
		// again builds them afresh
		std::vector<XmlView*> outermost;
		for (XmlView* view : s_views)
		{
			if (view->getRootPtr() != nullptr && !isInsideAnotherView(*view))
			{
				outermost.push_back(view);
			}
		}

		// The rebuilt views start on the home screen with their first tabs: remember where the user was, to go back
		std::vector<lv_obj_t*> roots;
		for (XmlView* view : outermost)
		{
			roots.push_back(view->getRootPtr());
		}
		const ReloadState where = ReloadState::capture(roots);

		// Every widget goes before the definitions are replaced: a component's styles are freed with its old
		// definition, and loading the bundles restyles and refreshes everything still on screen
		for (XmlView* view : outermost)
		{
			view->takeDown();
		}
		loadBundles();
		for (XmlView* view : outermost)
		{
			view->buildAgain();
		}
		where.restore();
	}

	const std::vector<LoadIssue>& getIssues()
	{
		return s_loaded.issues;
	}

	const ComponentInfo* findComponent(std::string_view name)
	{
		auto it = s_loaded.components.find(name);
		return it == s_loaded.components.end() ? nullptr : &it->second;
	}

	const std::map<std::string, ComponentInfo, std::less<>>& getComponents()
	{
		return s_loaded.components;
	}

	void addView(XmlView* view)
	{
		s_views.push_back(view);
	}

	void removeView(XmlView* view)
	{
		std::erase(s_views, view);
	}
} // namespace UI::Xml

/*
 * XmlUi.cpp
 */

#include "XmlUi.h"
#include "Debug.h"
#include "GlobalActions.h"
#include "HotReload.h"
#include "Services.h"
#include "Subscribers/ResponseSubscribers.h"
#include "Translations.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Styles.h"
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

	void reload()
	{
		ZoneScoped;
		UI_LOCK();
		if (!s_initialized)
		{
			return;
		}
		loadBundles();
		const auto views = s_views; // A rebuild may create or destroy views
		for (XmlView* view : views)
		{
			if (std::find(s_views.begin(), s_views.end(), view) != s_views.end())
			{
				view->rebuild();
			}
		}
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

	void addView(XmlView* view)
	{
		s_views.push_back(view);
	}

	void removeView(XmlView* view)
	{
		std::erase(s_views, view);
	}
} // namespace UI::Xml

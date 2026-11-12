/*
 * XmlUi.cpp
 */

#include "XmlUi.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "OmPublisher.h"
#include "Presenters/Presenters.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Styles.h"
#include "XmlActions.h"
#include "XmlListModel.h"
#include "XmlPresenter.h"
#include "XmlView.h"
#include "XmlWidgets.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include "src/libs/expat/expat.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

#ifndef ASSETS_FOLDER
#  error "ASSETS_FOLDER must be defined (UI/Styles/Styles.h)"
#endif

namespace UI::Xml
{
	namespace
	{
		bool s_initialized = false;
		Paths s_paths;
		std::vector<LoadIssue> s_issues;
		std::map<std::string, ComponentInfo, std::less<>> s_components;
		std::vector<XmlView*> s_views;
		std::function<void(std::string_view)> s_gcodeSender;
		std::function<ModalNumberPad*()> s_numberPadProvider;
		lv_timer_t* s_hotReloadTimer = nullptr;
		std::map<std::filesystem::path, std::filesystem::file_time_type> s_watched;
		std::set<std::string> s_translationPacks;

		/* ------------------------------------------------------------ theme */

		void registerThemeStyles()
		{
			ZoneScoped;
			// The global style objects keep their addresses; a theme switch rewrites them in place and reports the
			// change, so XML widgets follow theme switches like C++ ones.
			auto& l = const_cast<Themes::LvglStyles&>(Themes::getLvglStyles());
			auto& c = const_cast<Themes::ComponentStyles&>(Themes::getComponentStyles());
			auto reg = [](const char* name, Themes::Style& style)
			{ lv_xml_register_style_external(nullptr, name, static_cast<lv_style_t*>(style)); };

#define DS_STYLE(group, name) reg(#name, group.name);
			DS_STYLE(l, base)
			DS_STYLE(l, bg_dark)
			DS_STYLE(l, bg)
			DS_STYLE(l, bg_light)
			DS_STYLE(l, bg_color_primary)
			DS_STYLE(l, bg_color_primary_muted)
			DS_STYLE(l, bg_color_secondary)
			DS_STYLE(l, bg_color_secondary_muted)
			DS_STYLE(l, bg_color_success)
			DS_STYLE(l, bg_color_warning)
			DS_STYLE(l, bg_color_error)
			DS_STYLE(l, bg_color_header)
			DS_STYLE(l, bg_color_list_item)
			DS_STYLE(l, bg_modal) DS_STYLE(l, text) DS_STYLE(l, text_muted) DS_STYLE(l, text_emphasis) DS_STYLE(
				l, text_header) DS_STYLE(l, text_warning) DS_STYLE(l, border) DS_STYLE(l, border_muted)
				DS_STYLE(l, border_highlight) DS_STYLE(l, border_color_primary) DS_STYLE(
					l, border_color_secondary) DS_STYLE(l, border_color_card) DS_STYLE(l, screen) DS_STYLE(l, scrollbar)
					DS_STYLE(l, scrollbar_scrolled) DS_STYLE(l, card) DS_STYLE(l, tab_card) DS_STYLE(l, modal) DS_STYLE(
						l, btn) DS_STYLE(l, btn_checked) DS_STYLE(l, icon) DS_STYLE(l, icon_recolor)
						DS_STYLE(l, pressed) DS_STYLE(l, disabled) DS_STYLE(l, pad_base) DS_STYLE(l, pad_zero) DS_STYLE(
							l, pad_tiny) DS_STYLE(l, pad_small) DS_STYLE(l, pad_normal) DS_STYLE(l, pad_large)
							DS_STYLE(l, pad_gap) DS_STYLE(l, line_space_large) DS_STYLE(l, text_align_center) DS_STYLE(
								l,
								outline_primary) DS_STYLE(l,
														  outline_secondary) DS_STYLE(l, circle)
								DS_STYLE(l, no_radius) DS_STYLE(l, no_border) DS_STYLE(l, clip_corner) DS_STYLE(
									l, rotary_scroll) DS_STYLE(l, grow) DS_STYLE(l, transition_delayed)
									DS_STYLE(l, transition_normal) DS_STYLE(l, anim) DS_STYLE(l, anim_fast) DS_STYLE(
										l, actionBtn) DS_STYLE(l, input) DS_STYLE(l, long_press)
										DS_STYLE(l, draggable) DS_STYLE(l, dragging) DS_STYLE(
											l, drag_complete) DS_STYLE(l, knob)
#if LV_USE_BAR
											DS_STYLE(l, bar) DS_STYLE(l, bar_indic)
#endif
#if LV_USE_SLIDER
												DS_STYLE(l,
														 slider) DS_STYLE(l,
																		  slider_indic) DS_STYLE(l, slider_knob)
#endif
#if LV_USE_TEXTAREA
													DS_STYLE(l, text_area)
#endif
#if LV_USE_LIST
														DS_STYLE(l, list_bg) DS_STYLE(l, list_btn) DS_STYLE(
															l, list_item_grow)
#endif
															DS_STYLE(c, sidebar) DS_STYLE(c, sidebar_btn) DS_STYLE(
																c, estop) DS_STYLE(c, file) DS_STYLE(c, folder)
																DS_STYLE(c, unhomed) DS_STYLE(
																	c,
																	temperature_bar) DS_STYLE(c, temperature_bar_indic)
																	DS_STYLE(c, tool_list) DS_STYLE(
																		c, tool_list_tool) DS_STYLE(c, tool_list_heater)
																		DS_STYLE(c, tool_selected) DS_STYLE(
																			c, tab_button) DS_STYLE(c, slider_buttons)
																			DS_STYLE(c, list_header) DS_STYLE(
																				c, list_title) DS_STYLE(c, graph_legend)
																				DS_STYLE(c, bar_label_bg)
																					DS_STYLE(c, bar_label)
#undef DS_STYLE

																						auto& fonts =
																							const_cast<Themes::Fonts&>(
																								Themes::getFonts());
			lv_xml_register_font(nullptr, "font_header", &fonts.header);
			lv_xml_register_font(nullptr, "font_normal", &fonts.normal);
			lv_xml_register_font(nullptr, "font_emphasis", &fonts.emphasis);
			lv_xml_register_font(nullptr, "font_subdued", &fonts.subdued);
		}

		/* ------------------------------------------------------------ translations */

		/**
		 * XML `translation_tag`s go through LVGL's translation module. Feed it DuetScreen's table for the current
		 * language (one pack per language, built the first time it is selected) and tell LVGL to refresh.
		 */
		void syncTranslations()
		{
			ZoneScoped;
			std::string language(i18n::getCurrentLanguage());
			if (language.empty())
			{
				language = "default";
			}

			if (!s_translationPacks.contains(language))
			{
				lv_translation_pack_t* pack = lv_translation_add_dynamic();
				lv_translation_add_language(pack, language.c_str());
				i18n::forEachTranslation(
					[pack](std::string_view key, std::string_view value)
					{
						const std::string k(key);
						const std::string v(value);
						if (lv_translation_tag_dsc_t* tag = lv_translation_add_tag(pack, k.c_str()))
						{
							lv_translation_set_tag_translation(pack, tag, 0, v.c_str());
						}
					});
				s_translationPacks.insert(language);
			}

			lv_translation_set_language(language.c_str());
		}

		/* ------------------------------------------------------------ actions */

		void registerGlobalActions()
		{
			/* <action name="gcode" arg="M220 S{value}"/>: `{value}` is the edited value, `{index}` the list item */
			Actions::registerGlobal("gcode",
									[](const ActionArgs& args)
									{
										std::string gcode(args.arg);
										auto replace = [&gcode](std::string_view token, const std::string& with)
										{
											for (size_t pos; (pos = gcode.find(token)) != std::string::npos;)
											{
												gcode.replace(pos, token.size(), with);
											}
										};
										if (auto value = args.value(); value && args.event)
										{
											replace("{value}", fmt::format("{:g}", *value));
										}
										if (args.item)
										{
											replace("{index}", std::to_string(*args.item));
										}
										sendGcode(gcode);
									});
		}

		/* ------------------------------------------------------------ bundles */

		struct Preparsed
		{
			std::string presenter;
			bool sawRoot = false;
		};

		/**
		 * Check `text` is well-formed and read the root element's `presenter` attribute, before the engine sees it.
		 * @return an error message, or empty
		 */
		std::string preparse(const std::string& text, Preparsed& out)
		{
			XML_Parser parser = XML_ParserCreate(nullptr);
			XML_SetUserData(parser, &out);
			XML_SetElementHandler(
				parser,
				[](void* user, const XML_Char* name, const XML_Char** attrs)
				{
					auto* p = static_cast<Preparsed*>(user);
					if (p->sawRoot)
					{
						return;
					}
					p->sawRoot = true;
					if (std::string_view(name) != "component")
					{
						return;
					}
					for (size_t i = 0; attrs[i]; i += 2)
					{
						if (std::string_view(attrs[i]) == "presenter")
						{
							p->presenter = attrs[i + 1];
						}
					}
				},
				nullptr);

			std::string error;
			if (XML_Parse(parser, text.data(), static_cast<int>(text.size()), XML_TRUE) == XML_STATUS_ERROR)
			{
				error = fmt::format(
					"line {:d}: {:s}", XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
			}
			XML_ParserFree(parser);
			return error;
		}

		std::optional<std::string> readFile(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file)
			{
				return std::nullopt;
			}
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}

		void addIssue(const std::filesystem::path& file, std::string message)
		{
			LOG_ERROR("UI bundle: {:s}: {:s}", file.string(), message);
			s_issues.push_back({file, std::move(message)});
		}

		void watch(const std::filesystem::path& path)
		{
			std::error_code ec;
			s_watched[path] = std::filesystem::last_write_time(path, ec);
		}

		/** Expand a manifest entry: a file, or a file name with a single `*` wildcard */
		std::vector<std::filesystem::path> expand(const std::filesystem::path& root, const std::string& pattern)
		{
			std::vector<std::filesystem::path> files;
			const std::filesystem::path full = root / pattern;
			const std::string filename = full.filename().string();
			const size_t star = filename.find('*');
			if (star == std::string::npos)
			{
				files.push_back(full);
				return files;
			}

			const std::string prefix = filename.substr(0, star);
			const std::string suffix = filename.substr(star + 1);
			std::error_code ec;
			for (const auto& entry : std::filesystem::directory_iterator(full.parent_path(), ec))
			{
				const std::string name = entry.path().filename().string();
				if (entry.is_regular_file() && name.size() >= prefix.size() + suffix.size() &&
					name.starts_with(prefix) && name.ends_with(suffix))
				{
					files.push_back(entry.path());
				}
			}
			std::sort(files.begin(), files.end());
			return files;
		}

		void loadComponent(const std::filesystem::path& file, const std::string& bundle)
		{
			ZoneScoped;
			watch(file);
			const std::string name = file.stem().string();

			auto text = readFile(file);
			if (!text)
			{
				addIssue(file, "can't be read");
				return;
			}

			Preparsed pre;
			if (std::string error = preparse(*text, pre); !error.empty())
			{
				addIssue(file, error);
				return;
			}
			if (!pre.presenter.empty() && !Presenters::exists(pre.presenter))
			{
				addIssue(file, fmt::format("unknown presenter '{:s}'", pre.presenter));
				return;
			}

			// The engine keeps the earlier definition if this one fails
			if (lv_xml_register_component_from_data(name.c_str(), text->c_str()) != LV_RESULT_OK)
			{
				addIssue(file, "isn't a valid component");
				return;
			}

			s_components[name] = ComponentInfo{name, pre.presenter, file, bundle};
		}

		void loadBundle(const std::filesystem::path& dir)
		{
			ZoneScoped;
			const std::filesystem::path manifestPath = dir / "manifest.json";
			watch(manifestPath);

			auto text = readFile(manifestPath);
			if (!text)
			{
				addIssue(manifestPath, "missing");
				return;
			}

			const nlohmann::json manifest = nlohmann::json::parse(*text, nullptr, false, true);
			if (manifest.is_discarded() || !manifest.is_object())
			{
				addIssue(manifestPath, "isn't valid JSON");
				return;
			}

			const std::string id = manifest.value("id", dir.filename().string());
			const std::string api = manifest.value("ui_api", "");
			int major = -1;
			std::sscanf(api.c_str(), "%d", &major);
			if (major != UI_API_VERSION_MAJOR)
			{
				addIssue(manifestPath,
						 fmt::format("needs UI API {:s}, this firmware provides {:d}.{:d}",
									 api.empty() ? "?" : api,
									 UI_API_VERSION_MAJOR,
									 UI_API_VERSION_MINOR));
				return;
			}

			LOG_INFO("Loading UI bundle '{:s}' from {:s}", id, dir.string());
			for (const auto& pattern : manifest.value("components", std::vector<std::string>{}))
			{
				for (const auto& file : expand(dir, pattern))
				{
					loadComponent(file, id);
				}
			}
		}

		void loadAllBundles()
		{
			ZoneScoped;
			s_issues.clear();
			s_watched.clear();

			loadBundle(s_paths.builtin);

			std::error_code ec;
			if (!s_paths.user.empty() && std::filesystem::is_directory(s_paths.user, ec))
			{
				std::vector<std::filesystem::path> bundles;
				for (const auto& entry : std::filesystem::directory_iterator(s_paths.user, ec))
				{
					if (entry.is_directory() && std::filesystem::exists(entry.path() / "manifest.json"))
					{
						bundles.push_back(entry.path());
					}
				}
				std::sort(bundles.begin(), bundles.end());
				for (const auto& bundle : bundles)
				{
					loadBundle(bundle);
				}
			}

			for (const auto& issue : s_issues)
			{
				Model::get().post<EventType::Response>(
					ResponseType::WARNING,
					fmt::format("UI: {:s}: {:s}", issue.file.filename().string(), issue.message));
			}
		}

		void hotReloadTick(lv_timer_t*)
		{
			for (const auto& [path, stamp] : s_watched)
			{
				std::error_code ec;
				if (std::filesystem::last_write_time(path, ec) != stamp)
				{
					LOG_INFO("UI bundle file changed: {:s}", path.string());
					reload();
					return;
				}
			}
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
		registerThemeStyles();
		syncTranslations();
		Actions::init();
		registerWidgets();
		registerGlobalActions();
		registerBuiltinPresenters();
		OmPublisher::create();

		i18n::setLanguageChangedCallback(
			[]
			{
				UI_LOCK();
				syncTranslations();
				if (OmPublisher* publisher = OmPublisher::get())
				{
					publisher->refresh();
				}
			});

		s_initialized = true;
		loadAllBundles();
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
			LOG_ERROR("XML UI deinitialised with {:d} live view(s)", s_views.size());
		}

		setHotReload(0);
		i18n::setLanguageChangedCallback(nullptr);
		OmPublisher::destroy();
		Presenters::clear();
		Actions::deinit();
		lv_xml_deinit();

		s_components.clear();
		s_issues.clear();
		s_watched.clear();
		s_gcodeSender = nullptr;
		s_translationPacks.clear(); // lv_deinit() drops the packs
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
		loadAllBundles();
		const auto views = s_views;
		for (XmlView* view : views)
		{
			view->rebuild();
		}
	}

	const std::vector<LoadIssue>& getIssues()
	{
		return s_issues;
	}

	const ComponentInfo* findComponent(std::string_view name)
	{
		auto it = s_components.find(name);
		return it == s_components.end() ? nullptr : &it->second;
	}

	void setHotReload(uint32_t periodMs)
	{
		if (s_hotReloadTimer)
		{
			lv_timer_delete(s_hotReloadTimer);
			s_hotReloadTimer = nullptr;
		}
		if (periodMs > 0)
		{
			s_hotReloadTimer = lv_timer_create(hotReloadTick, periodMs, nullptr);
		}
	}

	void setGcodeSender(std::function<void(std::string_view)> sender)
	{
		s_gcodeSender = std::move(sender);
	}

	void sendGcode(std::string_view gcode)
	{
		// Lines are newline-terminated on the wire; XML authors shouldn't have to write "\n"
		std::string line(gcode);
		if (line.empty() || line.back() != '\n')
		{
			line += '\n';
		}

		LOG_DBG("XML UI sending '{:s}'", gcode);
		if (s_gcodeSender)
		{
			s_gcodeSender(line);
			return;
		}
		Comm::DUET.SendGcode(line);
	}

	void setNumberPadProvider(std::function<ModalNumberPad*()> provider)
	{
		s_numberPadProvider = std::move(provider);
	}

	ModalNumberPad* getNumberPad()
	{
		return s_numberPadProvider ? s_numberPadProvider() : nullptr;
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

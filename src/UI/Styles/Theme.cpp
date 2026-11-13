/*
 * Theme.cpp
 */

#include "Theme.h"
#include "Debug.h"
#include "Font.h"
#include "Storage.h"
#include "Styles.h"
#include "UI/Components/LVGL/LvObj.h"
#include "helix_xml.h"
#include "src/xml/lv_xml_base_types.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <charconv>
#include <memory>
#include <set>

namespace UI::Themes
{
	namespace
	{
		/** A palette, font or const definition merged down a theme's chain */
		struct Merged
		{
			std::map<std::string, std::string> palette;				// `<palette>` attributes
			std::vector<ColorAdjustment> colors;					// Adjustments, root ancestor's first
			std::map<std::string, FontSpec> fonts;					// Fonts by name
			std::map<std::string, std::string, std::less<>> consts; // Const values by name
		};

		/** The fonts of the active theme; FontManager frees a font when its last holder lets go */
		struct ActiveFonts
		{
			FontManager::Font header;	// Headings
			FontManager::Font normal;	// Body text
			FontManager::Font emphasis; // Emphasised text
			FontManager::Font subdued;	// Secondary text
		};

		/** Frees a style record, used by unique_ptr */
		struct RecordDeleter
		{
			/** Free what the record owns, then the record */
			void operator()(lv_xml_style_t* record) const
			{
				lv_xml_style_deinit(record);
				delete record;
			}
		};
		using Record = std::unique_ptr<lv_xml_style_t, RecordDeleter>;

		std::map<std::string, ThemeFile, std::less<>> s_files; // Loaded theme files by name
		std::vector<std::unique_ptr<Theme>> s_themes;		   // One per non-abstract file
		std::map<std::string_view, Theme*> s_index;			   // s_themes by name
		std::string s_current;								   // Name of the active theme; empty for none
		std::vector<Record> s_records; // Properties set by the active theme's styles; the styles point into them
		FontConfigSet s_fontConfig;	   // The active theme's fonts
		ActiveFonts s_activeFonts;	   // The fonts created from s_fontConfig

		/** Storage keys for a theme's palette choice */
		struct PaletteKeys
		{
			StorageKeyRunTime<uint16_t> primaryHue;	  // Primary hue
			StorageKeyRunTime<uint16_t> secondaryHue; // Secondary hue
			StorageKeyRunTime<float> chroma;		  // Chroma
			StorageKeyRunTime<bool> dark;			  // Dark mode
		};

		/** The storage keys of theme `name`'s palette choice, defaulting to `defaults` */
		PaletteKeys paletteKeys(std::string_view name, const PaletteInputs& defaults)
		{
			return PaletteKeys{
				.primaryHue = {fmt::format("themes:{:s}:primary_hue", name), defaults.primaryHue},
				.secondaryHue = {fmt::format("themes:{:s}:secondary_hue", name), defaults.secondaryHue},
				.chroma = {fmt::format("themes:{:s}:chroma", name), defaults.chroma},
				.dark = {fmt::format("themes:{:s}:dark_mode", name), defaults.dark},
			};
		}

		/**
		 * The files from the root ancestor to theme `name`
		 * @return the chain, or empty (with `error` set) if a theme is missing or extends itself
		 */
		std::vector<const ThemeFile*> chainOf(std::string_view name, std::string* error = nullptr)
		{
			std::vector<const ThemeFile*> chain;
			std::set<std::string, std::less<>> seen;
			for (std::string current(name); !current.empty();)
			{
				auto it = s_files.find(current);
				if (it == s_files.end())
				{
					if (error)
					{
						*error = fmt::format("theme '{:s}' not found", current);
					}
					return {};
				}
				if (!seen.insert(current).second)
				{
					if (error)
					{
						*error = fmt::format("theme '{:s}' extends itself", current);
					}
					return {};
				}
				chain.push_back(&it->second);
				current = it->second.extends;
			}
			std::reverse(chain.begin(), chain.end());
			return chain;
		}

		/** Merge palette, colours, fonts and consts down `chain` */
		Merged merge(const std::vector<const ThemeFile*>& chain)
		{
			Merged merged;
			for (const ThemeFile* file : chain)
			{
				for (const auto& [key, value] : file->palette)
				{
					merged.palette[key] = value;
				}
				merged.colors.insert(merged.colors.end(), file->colors.begin(), file->colors.end());
				for (const auto& [name, font] : file->fonts)
				{
					FontSpec& into = merged.fonts[name];
					if (font.size)
					{
						into.size = font.size;
					}
					if (font.weight)
					{
						into.weight = font.weight;
					}
					if (font.style)
					{
						into.style = font.style;
					}
				}
				for (const auto& [name, value] : file->consts)
				{
					merged.consts[name] = value;
				}
			}
			return merged;
		}

		/** `text` as a float, if all of it is one */
		std::optional<float> toFloat(std::string_view text)
		{
			float value = 0;
			auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
			if (text.empty() || ec != std::errc() || ptr != text.data() + text.size())
			{
				return std::nullopt;
			}
			return value;
		}

		/** `text` with surrounding spaces removed */
		std::string_view trim(std::string_view text)
		{
			while (!text.empty() && text.front() == ' ')
			{
				text.remove_prefix(1);
			}
			while (!text.empty() && text.back() == ' ')
			{
				text.remove_suffix(1);
			}
			return text;
		}

		/** The palette inputs in `merged`, with `problems` noting values that aren't numbers */
		PaletteInputs paletteInputs(const Merged& merged, std::vector<std::string>* problems = nullptr)
		{
			PaletteInputs inputs;
			auto number = [&](const char* key, float fallback)
			{
				auto it = merged.palette.find(key);
				if (it == merged.palette.end())
				{
					return fallback;
				}
				auto value = toFloat(it->second);
				if (!value && problems)
				{
					problems->push_back(fmt::format("palette {:s}=\"{:s}\" isn't a number", key, it->second));
				}
				return value.value_or(fallback);
			};
			inputs.primaryHue = static_cast<uint16_t>(number("primary_hue", 0));
			inputs.secondaryHue = static_cast<uint16_t>(number("secondary_hue", 0));
			inputs.chroma = number("chroma", 0);
			auto dark = merged.palette.find("dark");
			inputs.dark = dark != merged.palette.end() && dark->second == "true";
			return inputs;
		}

		/** Turns theme values into LVGL XML style values (see Theme.h, "Values") */
		class Resolver
		{
		  public:
			/** Resolve against `colors` and `consts` */
			Resolver(ThemeColors colors, const std::map<std::string, std::string, std::less<>>& consts)
				: m_colors(colors)
				, m_consts(consts)
			{
			}

			/** The value of `raw`, or nullopt with `error` set */
			std::optional<std::string> resolve(std::string_view raw, std::string& error, int depth = 0) const
			{
				raw = trim(raw);
				if (depth > 8)
				{
					error = fmt::format("'{:s}' refers to itself", raw);
					return std::nullopt;
				}

				if (raw.starts_with("-#"))
				{
					// A negated colour or const: only numbers can be negated
					auto value = resolve(raw.substr(1), error, depth + 1);
					if (value && toFloat(*value))
					{
						return value->starts_with('-') ? value->substr(1) : "-" + *value;
					}
					if (value)
					{
						error = fmt::format("'{:s}' negates something that isn't a number", raw);
					}
					return std::nullopt;
				}

				if (raw.starts_with('#'))
				{
					const std::string_view name = raw.substr(1);
					if (const Color* color = findColor(name))
					{
						return hex(*color);
					}
					auto it = m_consts.find(name);
					if (it == m_consts.end())
					{
						error = fmt::format("unknown colour or const '{:s}'", raw);
						return std::nullopt;
					}
					return resolve(it->second, error, depth + 1);
				}

				if (raw.ends_with("dpx"))
				{
					const bool negative = raw.starts_with('-');
					const std::string_view digits = raw.substr(negative ? 1 : 0, raw.size() - 3 - (negative ? 1 : 0));
					int32_t n = 0;
					auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
					if (digits.empty() || ec != std::errc() || ptr != digits.data() + digits.size())
					{
						error = fmt::format("'{:s}' isn't a whole number of dpx", raw);
						return std::nullopt;
					}
					const int32_t px = LV_DPX_CALC(lv_display_get_dpi(nullptr), n);
					return std::to_string(negative ? -px : px);
				}

				if (raw.ends_with(')'))
				{
					return function(raw, error);
				}

				return std::string(raw);
			}

		  private:
			/** `lighten(#c, x)`, `darken(#c, x)` or `rgb_darken(#c, n)` */
			std::optional<std::string> function(std::string_view raw, std::string& error) const
			{
				const size_t open = raw.find('(');
				const size_t comma = raw.find(',');
				if (open == std::string_view::npos || comma == std::string_view::npos || comma < open)
				{
					error = fmt::format("'{:s}' should look like name(#colour, amount)", raw);
					return std::nullopt;
				}
				const std::string_view name = trim(raw.substr(0, open));
				const std::string_view colorRef = trim(raw.substr(open + 1, comma - open - 1));
				const std::string_view amountText = trim(raw.substr(comma + 1, raw.size() - comma - 2));

				const Color* color = colorRef.starts_with('#') ? findColor(colorRef.substr(1)) : nullptr;
				const auto amount = toFloat(amountText);
				if (color == nullptr || !amount)
				{
					error = fmt::format("'{:s}' needs a palette colour and a number", raw);
					return std::nullopt;
				}

				Color result = *color;
				if (name == "lighten")
				{
					result.setL(std::min(color->getL() + *amount, 1.0f));
					return hex(result);
				}
				if (name == "darken")
				{
					result.setL(std::max(color->getL() - *amount, 0.0f));
					return hex(result);
				}
				if (name == "rgb_darken")
				{
					return hex(lv_color_darken(static_cast<lv_color_t>(*color), static_cast<lv_opa_t>(*amount)));
				}
				error = fmt::format("unknown function '{:s}'", name);
				return std::nullopt;
			}

			/** The palette colour called `name`, or nullptr */
			const Color* findColor(std::string_view name) const
			{
				const Color* found = nullptr;
				auto& colors = const_cast<ThemeColors&>(m_colors);
				forEachColor(colors,
							 [&](std::string_view n, Color& c)
							 {
								 if (n == name)
								 {
									 found = &c;
								 }
							 });
				return found;
			}

			/** `color` as 0xRRGGBB */
			static std::string hex(lv_color_t color)
			{
				return fmt::format("0x{:06x}", lv_color_to_u32(color) & 0xFFFFFF);
			}

			ThemeColors m_colors;											 // Palette colours
			const std::map<std::string, std::string, std::less<>>& m_consts; // Consts
		};

		/** The global styles by name */
		std::map<std::string, Style*, std::less<>> stylesByName()
		{
			std::map<std::string, Style*, std::less<>> styles;
			forEachStyle([&](std::string_view name, Style& style) { styles.emplace(name, &style); });
			return styles;
		}

		/**
		 * Apply `edit` to the styles
		 * @param records Receives the records the edit's properties are kept in
		 * @param problems Receives a message per problem
		 */
		void applyEdit(const StyleEdit& edit,
					   const std::map<std::string, Style*, std::less<>>& styles,
					   const Resolver& resolver,
					   std::vector<Record>& records,
					   std::vector<std::string>& problems)
		{
			auto lookup = [&](const std::string& name) -> lv_style_t*
			{
				auto it = styles.find(name);
				if (it == styles.end())
				{
					problems.push_back(fmt::format("line {:d}: unknown style '{:s}'", edit.line, name));
					return nullptr;
				}
				return *it->second;
			};

			lv_style_t* target = lookup(edit.name);
			if (target == nullptr)
			{
				return;
			}

			if (!edit.copy.empty())
			{
				if (lv_style_t* source = lookup(edit.copy))
				{
					lv_style_copy(target, source);
				}
			}
			for (const auto& name : edit.merge)
			{
				if (lv_style_t* source = lookup(name))
				{
					lv_style_merge(target, source);
				}
			}

			// The properties go into a record of their own, which owns anything they point to (a transition)
			std::vector<std::string> values;
			values.reserve(edit.props.size());
			std::vector<const char*> attrs;
			for (const auto& [prop, raw] : edit.props)
			{
				std::string error;
				auto value = resolver.resolve(raw, error);
				if (!value)
				{
					problems.push_back(fmt::format("line {:d}: {:s}: {:s}", edit.line, prop, error));
					continue;
				}
				if (*value == "remove")
				{
					lv_style_remove_prop(target, lv_xml_style_prop_to_enum(prop.c_str()));
					continue;
				}
				values.push_back(std::move(*value));
				attrs.push_back(prop.c_str());
				attrs.push_back(values.back().c_str());
			}
			if (attrs.empty())
			{
				return;
			}
			attrs.push_back(nullptr);
			attrs.push_back(nullptr);

			Record record(new lv_xml_style_t);
			lv_xml_style_init(record.get(), edit.name.c_str());
			lv_xml_style_set_props(record.get(), nullptr, attrs.data());
			lv_style_merge(target, &record->style);
			records.push_back(std::move(record));
		}

		/** FreeType style flags for a font */
		uint32_t fontStyleFlags(const FontSpec& font)
		{
			uint32_t flags = LV_FREETYPE_FONT_STYLE_NORMAL;
			const std::string style = font.style.value_or("normal");
			if (style.find("bold") != std::string::npos)
			{
				flags |= LV_FREETYPE_FONT_STYLE_BOLD;
			}
			if (style.find("italic") != std::string::npos)
			{
				flags |= LV_FREETYPE_FONT_STYLE_ITALIC;
			}
			if (font.weight.value_or(0) > 0)
			{
				flags |= LV_FREETYPE_FONT_STYLE_WEIGHT(*font.weight);
			}
			return flags;
		}

		/** The four fonts in `merged` */
		FontConfigSet fontConfig(const Merged& merged)
		{
			auto get = [&](const char* name)
			{
				FontConfig config;
				auto it = merged.fonts.find(name);
				if (it != merged.fonts.end())
				{
					config.size = it->second.size.value_or(config.size);
					config.style = fontStyleFlags(it->second);
				}
				return config;
			};
			return FontConfigSet{.header = get("header"),
								 .normal = get("normal"),
								 .emphasis = get("emphasis"),
								 .subdued = get("subdued")};
		}

		/** Make palette colours and consts XML consts, so components can use `#primary` */
		void publishConsts(const ThemeColors& colors, const Merged& merged, const Resolver& resolver)
		{
			auto& c = const_cast<ThemeColors&>(colors);
			forEachColor(c,
						 [](std::string_view name, Color& color)
						 {
							 const std::string n(name);
							 const std::string value =
								 fmt::format("0x{:06x}", lv_color_to_u32(static_cast<lv_color_t>(color)) & 0xFFFFFF);
							 lv_xml_set_const(nullptr, n.c_str(), value.c_str());
						 });
			for (const auto& [name, raw] : merged.consts)
			{
				std::string error;
				if (auto value = resolver.resolve(raw, error))
				{
					lv_xml_set_const(nullptr, name.c_str(), value->c_str());
				}
			}
		}

		/** Build the styles of theme `name`. @return the problems found */
		std::vector<std::string> build(std::string_view name, const PaletteInputs& palette, bool apply)
		{
			std::vector<std::string> problems;
			std::string error;
			const auto chain = chainOf(name, &error);
			if (chain.empty())
			{
				problems.push_back(error);
				return problems;
			}

			const Merged merged = merge(chain);
			const ThemeColors colors = generatePalette(palette, merged.colors, &problems);
			const Resolver resolver(colors, merged.consts);
			for (const auto& [constName, raw] : merged.consts)
			{
				if (!resolver.resolve(raw, error))
				{
					problems.push_back(fmt::format("const {:s}: {:s}", constName, error));
				}
			}

			const auto styles = stylesByName();
			std::vector<Record> records;
			if (apply)
			{
				for (auto& [styleName, style] : styles)
				{
					lv_style_reset(*style);
				}
			}
			for (const ThemeFile* file : chain)
			{
				for (const auto& edit : file->styles)
				{
					if (apply)
					{
						applyEdit(edit, styles, resolver, records, problems);
						continue;
					}
					// Checking only: resolve values and names without touching the styles
					for (const std::string* n : {&edit.name, &edit.copy})
					{
						if (!n->empty() && !styles.contains(*n))
						{
							problems.push_back(fmt::format("line {:d}: unknown style '{:s}'", edit.line, *n));
						}
					}
					for (const auto& m : edit.merge)
					{
						if (!styles.contains(m))
						{
							problems.push_back(fmt::format("line {:d}: unknown style '{:s}'", edit.line, m));
						}
					}
					for (const auto& [prop, raw] : edit.props)
					{
						if (!resolver.resolve(raw, error))
						{
							problems.push_back(fmt::format("line {:d}: {:s}: {:s}", edit.line, prop, error));
						}
					}
				}
			}

			if (apply)
			{
				// The styles now point into the new records; the old ones can go
				s_records = std::move(records);
				s_fontConfig = fontConfig(merged);
				publishConsts(colors, merged, resolver);
			}
			return problems;
		}

		/** The merged definition of theme `name` (empty if it can't be resolved) */
		Merged mergedOf(std::string_view name)
		{
			return merge(chainOf(name));
		}
	} // namespace

	Theme::Theme(std::string name)
		: m_name(std::move(name))
	{
	}

	bool Theme::isCustomisable() const
	{
		const Merged merged = mergedOf(m_name);
		auto it = merged.palette.find("customisable");
		return it != merged.palette.end() && it->second == "true";
	}

	PaletteInputs Theme::getDefaultPalette() const
	{
		return paletteInputs(mergedOf(m_name));
	}

	PaletteInputs Theme::getPalette() const
	{
		const PaletteInputs defaults = getDefaultPalette();
		if (!isCustomisable())
		{
			return defaults;
		}
		const PaletteKeys keys = paletteKeys(m_name, defaults);
		return PaletteInputs{
			.primaryHue = StorageHelper::getData(keys.primaryHue),
			.secondaryHue = StorageHelper::getData(keys.secondaryHue),
			.chroma = StorageHelper::getData(keys.chroma),
			.dark = StorageHelper::getData(keys.dark),
		};
	}

	void Theme::setPalette(const PaletteInputs& palette)
	{
		if (!isCustomisable())
		{
			LOG_WARN("Theme '{:s}' has a fixed palette", m_name);
			return;
		}
		const PaletteKeys keys = paletteKeys(m_name, getDefaultPalette());
		StorageHelper::setData(keys.primaryHue, palette.primaryHue);
		StorageHelper::setData(keys.secondaryHue, palette.secondaryHue);
		StorageHelper::setData(keys.chroma, palette.chroma);
		StorageHelper::setData(keys.dark, palette.dark);
	}

	void Theme::resetPalette()
	{
		setPalette(getDefaultPalette());
	}

	ThemeColors Theme::getColors() const
	{
		return generatePalette(getPalette(), mergedOf(m_name).colors);
	}

	FontConfigSet Theme::getFonts() const
	{
		return fontConfig(mergedOf(m_name));
	}

	void Theme::setThemeActive()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Applying theme: {:s}", m_name);

		lv_enable_style_refresh(false);
		for (const auto& problem : build(m_name, getPalette(), true))
		{
			LOG_WARN("Theme '{:s}': {:s}", m_name, problem);
		}
		s_current = m_name;
		setTypeface(FontManager::getActiveTypefaceName());
		lv_enable_style_refresh(true);

		{
			ZoneScopedN("Refreshing styles");
			lv_obj_report_style_change(nullptr);
			lv_xml_reapply_style_tokens();
			lv_xml_reapply_token_styles(lv_screen_active());
		}
		lv_obj_invalidate(lv_screen_active());
	}

	std::vector<const ThemeFile*> getThemeChain(std::string_view name)
	{
		return chainOf(name);
	}

	void setThemeFiles(std::vector<ThemeFile> files,
					   std::vector<std::pair<std::filesystem::path, std::string>>* problems)
	{
		ZoneScoped;
		s_files.clear();
		for (auto& file : files)
		{
			std::string name = file.name;
			s_files.insert_or_assign(std::move(name), std::move(file));
		}

		s_index.clear();
		s_themes.clear();
		for (const auto& [name, file] : s_files)
		{
			// Check every theme, so problems are reported when it loads rather than when it is chosen
			const PaletteInputs palette = paletteInputs(mergedOf(name));
			for (const auto& problem : build(name, palette, false))
			{
				LOG_WARN("Theme '{:s}': {:s}", name, problem);
				if (problems)
				{
					problems->emplace_back(file.source, problem);
				}
			}
			if (!file.abstract)
			{
				s_themes.push_back(std::make_unique<Theme>(name));
				s_index.emplace(s_themes.back()->getName(), s_themes.back().get());
			}
		}

		// After a reload keep the active theme; the first time, use the stored one
		const std::string wanted = s_current.empty() ? std::string(StorageHelper::getData(ID_THEME)) : s_current;
		Theme* theme = getThemeByName(wanted);
		if (theme == nullptr)
		{
			LOG_INFO("Theme not found, using default theme");
			theme = getDefaultTheme();
			if (theme != nullptr)
			{
				StorageHelper::setData(ID_THEME, theme->getName());
			}
		}
		if (theme != nullptr)
		{
			theme->setThemeActive();
		}
	}

	void clearThemes()
	{
		forEachStyle([](std::string_view, Style& style) { lv_style_reset(style); });
		s_records.clear();
		// Release the fonts while LVGL is still running; static destruction at exit would be too late
		s_activeFonts = ActiveFonts{};
		s_index.clear();
		s_themes.clear();
		s_files.clear();
		s_current.clear();
	}

	const std::map<std::string_view, Theme*>& getThemes()
	{
		return s_index;
	}

	Theme* getCurrentTheme()
	{
		return s_current.empty() ? nullptr : getThemeByName(s_current);
	}

	Theme* getTheme(size_t index)
	{
		if (index >= s_index.size())
		{
			LOG_ERROR("Theme with index {:d} not found", index);
			return nullptr;
		}
		auto it = s_index.begin();
		std::advance(it, index);
		return it->second;
	}

	Theme* getThemeByName(std::string_view name)
	{
		auto it = s_index.find(name);
		return it == s_index.end() ? nullptr : it->second;
	}

	Theme* getDefaultTheme()
	{
		if (Theme* theme = getThemeByName(ID_THEME.default_value))
		{
			return theme;
		}
		LOG_ERROR("Default theme not found, using first available theme");
		return s_index.empty() ? nullptr : s_index.begin()->second;
	}

	size_t getThemeCount()
	{
		return s_index.size();
	}

	bool refreshCurrentTheme()
	{
		ZoneScoped;
		UI_LOCK();
		if (Theme* theme = getCurrentTheme())
		{
			theme->setThemeActive();
			return true;
		}
		LOG_ERROR("No current theme set");
		return false;
	}

	std::vector<std::string_view> getThemeNames()
	{
		std::vector<std::string_view> names;
		for (const auto& [name, theme] : s_index)
		{
			names.push_back(name);
		}
		return names;
	}

	void setTypeface(const std::string& typeface)
	{
		ZoneScoped;
		LOG_INFO("Setting typeface to {:s}", typeface);

		// Replacing a holder releases the previous font
		s_activeFonts.header = FontManager::createFont(typeface, s_fontConfig.header.size, s_fontConfig.header.style);
		s_activeFonts.normal = FontManager::createFont(typeface, s_fontConfig.normal.size, s_fontConfig.normal.style);
		s_activeFonts.emphasis =
			FontManager::createFont(typeface, s_fontConfig.emphasis.size, s_fontConfig.emphasis.style);
		s_activeFonts.subdued =
			FontManager::createFont(typeface, s_fontConfig.subdued.size, s_fontConfig.subdued.style);

		// The styles point at the global fonts, so copying into them changes every style using them
		setFonts(*s_activeFonts.header.get(),
				 *s_activeFonts.normal.get(),
				 *s_activeFonts.emphasis.get(),
				 *s_activeFonts.subdued.get());
	}
} // namespace UI::Themes

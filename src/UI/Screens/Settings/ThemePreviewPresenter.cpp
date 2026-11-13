/*
 * ThemePreviewPresenter.cpp
 */

#include "ThemePreviewPresenter.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "i18n/i18n.h"
#include <cmath>
#include <fmt/format.h>

namespace UI
{
	namespace
	{
		/** `chroma` holds the palette's chroma times this */
		constexpr float CHROMA_SCALE = 100.0f;

		/** The colours' lightness and chroma fields hold their values times this */
		constexpr float THOUSANDTHS = 1000.0f;

		/** `value` times `scale`, rounded */
		int32_t scaled(float value, float scale)
		{
			return static_cast<int32_t>(std::lround(value * scale));
		}

		/** A colour a theme gives a style */
		struct SwatchStyle
		{
			std::string_view name;		// Style name, translated as settings.style.<name>
			const Themes::Style* style; // The style
			lv_style_prop_t colorProp;	// Which of its colours to show
			int32_t textStyle = 0;		// For a text style, the style its name is shown in (see text_style)
		};

		/** The colours shown, in order */
		std::vector<SwatchStyle> swatchStyles()
		{
			const auto& styles = Themes::getLvglStyles();
			return {
				{"bg_dark", &styles.bg_dark, LV_STYLE_BG_COLOR},
				{"bg", &styles.bg, LV_STYLE_BG_COLOR},
				{"bg_light", &styles.bg_light, LV_STYLE_BG_COLOR},
				{"bg_color_primary", &styles.bg_color_primary, LV_STYLE_BG_COLOR},
				{"bg_color_primary_muted", &styles.bg_color_primary_muted, LV_STYLE_BG_COLOR},
				{"bg_color_secondary", &styles.bg_color_secondary, LV_STYLE_BG_COLOR},
				{"bg_color_secondary_muted", &styles.bg_color_secondary_muted, LV_STYLE_BG_COLOR},
				{"text", &styles.text, LV_STYLE_TEXT_COLOR},
				{"text_muted", &styles.text_muted, LV_STYLE_TEXT_COLOR, 1},
				{"text_emphasis", &styles.text_emphasis, LV_STYLE_TEXT_COLOR, 2},
				{"text_header", &styles.text_header, LV_STYLE_TEXT_COLOR, 3},
				{"border", &styles.border, LV_STYLE_BORDER_COLOR},
				{"border_muted", &styles.border_muted, LV_STYLE_BORDER_COLOR},
				{"border_highlight", &styles.border_highlight, LV_STYLE_BORDER_COLOR},
				{"success", &styles.bg_color_success, LV_STYLE_BG_COLOR},
				{"warning", &styles.bg_color_warning, LV_STYLE_BG_COLOR},
				{"error", &styles.bg_color_error, LV_STYLE_BG_COLOR},
			};
		}
	} // namespace

	void SwatchRow::fields(Xml::FieldVisitor& f)
	{
		f.field("label", label, "The style's name");
		f.field("color", color, "Its colour");
		f.field("text_style", textStyle, "For the text styles: 1 muted, 2 emphasis, 3 header; else 0");
	}

	ThemePreviewPresenter::ThemePreviewPresenter(bool controls)
		: XmlPresenter("theme_preview")
		, m_controls(controls)
	{
		action("preview.primary_hue",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto hue = static_cast<uint16_t>(args.value().value_or(0));
				   changePalette([hue](Themes::PaletteInputs& palette) { palette.primaryHue = hue; });
			   });
		action("preview.secondary_hue",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto hue = static_cast<uint16_t>(args.value().value_or(0));
				   changePalette([hue](Themes::PaletteInputs& palette) { palette.secondaryHue = hue; });
			   });
		action("preview.chroma",
			   [this](const Xml::ActionArgs& args)
			   {
				   const float chroma = args.value().value_or(0.0f);
				   changePalette([chroma](Themes::PaletteInputs& palette) { palette.chroma = chroma; });
			   });
		action("preview.dark",
			   [this](const Xml::ActionArgs& args)
			   {
				   const bool dark = args.value().value_or(0.0f) != 0.0f;
				   changePalette([dark](Themes::PaletteInputs& palette) { palette.dark = dark; });
			   });
	}

	void ThemePreviewPresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("swatches", m_swatches, "A swatch of each colour of the theme");
		f.field("controls_shown", m_controlsShown, "1 if the palette can be changed here");
		f.field("primary_hue", m_primaryHue, "Primary hue, 0-360");
		f.field("secondary_hue", m_secondaryHue, "Secondary hue, 0-360");
		f.field("chroma", m_chroma, "Chroma, 0-0.4, in hundredths");
		f.field("dark", m_dark, "1 in dark mode");
		f.field("primary.lightness", m_primaryLightness, "The primary colour's OKLCH lightness, in thousandths");
		f.field("primary.chroma", m_primaryChroma, "The primary colour's OKLCH chroma, in thousandths");
		f.field("secondary.lightness", m_secondaryLightness, "The secondary colour's OKLCH lightness, in thousandths");
		f.field("secondary.chroma", m_secondaryChroma, "The secondary colour's OKLCH chroma, in thousandths");
	}

	void ThemePreviewPresenter::onInit()
	{
		registerEventListener<EventType::ThemeChanged>(
			[this]()
			{
				UI_LOCK();
				showTheme();
			});
	}

	void ThemePreviewPresenter::onActivate()
	{
		showTheme();
	}

	void ThemePreviewPresenter::showTheme()
	{
		const Themes::Theme* theme = Themes::getCurrentTheme();
		const bool customisable = theme != nullptr && theme->isCustomisable();
		m_controlsShown.set(m_controls && customisable ? 1 : 0);
		if (customisable)
		{
			const auto palette = theme->getPalette();
			m_primaryHue.set(palette.primaryHue);
			m_secondaryHue.set(palette.secondaryHue);
			m_chroma.set(scaled(palette.chroma, CHROMA_SCALE));
			m_dark.set(palette.dark ? 1 : 0);
			showColours();
		}
		showSwatches();
	}

	void ThemePreviewPresenter::showSwatches()
	{
		const auto styles = swatchStyles();
		m_swatches.sync(styles.size(),
						[&styles](SwatchRow& row, size_t i)
						{
							const SwatchStyle& swatch = styles[i];
							row.label.set(_(fmt::format("settings.style.{:s}", swatch.name)));
							row.textStyle.set(swatch.textStyle);
							lv_style_value_t value;
							if (swatch.style->getProp(swatch.colorProp, &value) == LV_STYLE_RES_FOUND)
							{
								row.color.set(value.color);
							}
							else
							{
								LOG_ERROR("Failed to get color for style: {}", swatch.name);
							}
						});
	}

	void ThemePreviewPresenter::showColours()
	{
		const Themes::Theme* theme = Themes::getCurrentTheme();
		if (theme == nullptr)
		{
			return;
		}
		const auto colors = theme->getColors();
		m_primaryLightness.set(scaled(colors.primary.getL(), THOUSANDTHS));
		m_primaryChroma.set(scaled(colors.primary.getC(), THOUSANDTHS));
		m_secondaryLightness.set(scaled(colors.secondary.getL(), THOUSANDTHS));
		m_secondaryChroma.set(scaled(colors.secondary.getC(), THOUSANDTHS));
	}

	void ThemePreviewPresenter::changePalette(const std::function<void(Themes::PaletteInputs&)>& change)
	{
		Themes::Theme* theme = Themes::getCurrentTheme();
		if (theme == nullptr || !theme->isCustomisable())
		{
			LOG_ERROR("The theme in use has a fixed palette");
			return;
		}
		auto palette = theme->getPalette();
		change(palette);
		theme->setPalette(palette);
		Themes::refreshCurrentTheme();
		m_primaryHue.set(palette.primaryHue);
		m_secondaryHue.set(palette.secondaryHue);
		m_chroma.set(scaled(palette.chroma, CHROMA_SCALE));
		m_dark.set(palette.dark ? 1 : 0);
		showColours();
		showSwatches();
	}
} // namespace UI

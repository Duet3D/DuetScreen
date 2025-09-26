/*
 * ThemePreview.cpp
 *
 *  Created on: 2025-08-06
 *      Author: Andy Everitt
 */

#include "ThemePreview.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Themes/DefaultTheme.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	class Swatch : public LvContainer
	{
	  public:
		Swatch(const std::string& name, LvObj& parent)
			: LvContainer(name, parent)
			, m_colorBox("color_box", getRoot())
			, m_label("label", getRoot())
		{
			m_colorBox.addStyle(Themes::getLvglStyles().border);

			setFlexFlow(LV_FLEX_FLOW_COLUMN);
			setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			m_colorBox.setWidth(LV_PCT(100));
			m_colorBox.setFlexGrow(1);
			m_colorBox.setMinHeight(20);
			m_colorBox.setStyleBgOpa(LV_OPA_COVER);
			m_label.setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_label.setMinWidth(LV_SIZE_CONTENT);
			m_label.setStyleTextAlign(LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
		}

		void setSwatchColor(lv_color_t color) { m_colorBox.setStyleBgColor(color, LV_PART_MAIN); }

		void setLabel(std::string_view text) { m_label.setText(text); }

	  private:
		LvContainer m_colorBox;
		LvLabel m_label;
		lv_style_t* m_style = nullptr;
	};

	struct LabelledStyle
	{
		std::string_view name;
		const Themes::Style& style;
		lv_style_prop_t color_prop;
	};

	ThemePreview::ThemePreview(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
		, m_swatches("swatches", getRoot())
		, m_primaryHueSlider("primary_hue_slider", getRoot())
		, m_secondaryHueSlider("secondary_hue_slider", getRoot())
		, m_chromaSlider("chroma_slider", getRoot())
		, m_darkMode("dark_mode", getRoot())
	{
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		m_swatches.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_swatches.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_swatches.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		m_primaryHueSlider.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_primaryHueSlider.setRange(0, 360);
		m_primaryHueSlider.setValue(0);
		m_primaryHueSlider.setLabel(_("primary_hue"));
		m_primaryHueSlider.setValueChangedCallback([this](int32_t) { updateThemeColors(); });

		m_secondaryHueSlider.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_secondaryHueSlider.setRange(0, 360);
		m_secondaryHueSlider.setValue(0);
		m_secondaryHueSlider.setLabel(_("secondary_hue"));
		m_secondaryHueSlider.setValueChangedCallback([this](int32_t) { updateThemeColors(); });

		m_chromaSlider.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_chromaSlider.setRange(0, 0.4);
		m_chromaSlider.setIncrementValue(0.01f);
		m_chromaSlider.setValue(0.2);
		m_chromaSlider.setLabel(_("chroma"));
		m_chromaSlider.setValueChangedCallback([this](int32_t) { updateThemeColors(); });

		m_darkMode.setText(_("dark_mode"));
		m_darkMode.setChecked(true);
		m_darkMode.setCheckedCallback([this](bool checked) { updateThemeColors(); });

		updateSwatches();
	}

	void ThemePreview::updateSwatches()
	{
		const auto& styles = Themes::getLvglStyles();
		const LabelledStyle swatch_styles[] = {
			{"style_bg_dark", styles.bg_dark, LV_STYLE_BG_COLOR},
			{"style_bg", styles.bg, LV_STYLE_BG_COLOR},
			{"style_bg_light", styles.bg_light, LV_STYLE_BG_COLOR},
			{"style_bg_color_primary", styles.bg_color_primary, LV_STYLE_BG_COLOR},
			{"style_bg_color_primary_muted", styles.bg_color_primary_muted, LV_STYLE_BG_COLOR},
			{"style_bg_color_secondary", styles.bg_color_secondary, LV_STYLE_BG_COLOR},
			{"style_bg_color_secondary_muted", styles.bg_color_secondary_muted, LV_STYLE_BG_COLOR},
			// {"style_bg_color_header", styles.bg_color_header, LV_STYLE_BG_COLOR},
			// {"style_bg_color_list_item", styles.bg_color_list_item, LV_STYLE_BG_COLOR},
			{"style_text", styles.text, LV_STYLE_TEXT_COLOR},
			{"style_text_muted", styles.text_muted, LV_STYLE_TEXT_COLOR},
			{"style_text_header", styles.text_header, LV_STYLE_TEXT_COLOR},
			{"style_border", styles.border, LV_STYLE_BORDER_COLOR},
			{"style_border_muted", styles.border_muted, LV_STYLE_BORDER_COLOR},
			{"style_border_highlight", styles.border_highlight, LV_STYLE_BORDER_COLOR},
			// {"style_border_color_primary", styles.border_color_primary, LV_STYLE_BORDER_COLOR},
			// {"style_border_color_secondary", styles.border_color_secondary, LV_STYLE_BORDER_COLOR},
		};

		m_swatches.clear();
		m_swatches.setItemCount(std::size(swatch_styles),
								[this, &swatch_styles](size_t index, LvObj& parent)
								{
									auto swatch = std::make_shared<Swatch>(fmt::format("{:d}", index), parent);
									swatch->setSize(180, LV_SIZE_CONTENT);
									swatch->setMaxWidth(LV_PCT(100));
									auto& style = swatch_styles[index];
									swatch->setLabel(style.name);

									lv_style_value_t value;
									if (lv_style_get_prop(style.style, style.color_prop, &value) == LV_STYLE_RES_FOUND)
									{
										swatch->setSwatchColor(value.color);
									}
									else
									{
										LOG_ERROR("Failed to get color for style: {}", style.name);
									}

									// swatch->addSwatchStyle(style.style);

									return swatch;
								});
	}

	void ThemePreview::setPrimaryHue(size_t hue)
	{
		m_primaryHueSlider.setValue(static_cast<float>(hue));
		updateThemeColors();
	}

	void ThemePreview::setSecondaryHue(size_t hue)
	{
		m_secondaryHueSlider.setValue(static_cast<float>(hue));
		updateThemeColors();
	}

	void ThemePreview::setChroma(float chroma)
	{
		m_chromaSlider.setValue(chroma);
		updateThemeColors();
	}

	void ThemePreview::setDarkMode(bool enable)
	{
		m_darkMode.setChecked(enable);
		updateThemeColors();
	}

	void ThemePreview::showControls(bool show)
	{
		m_primaryHueSlider.setVisible(show);
		m_secondaryHueSlider.setVisible(show);
		m_chromaSlider.setVisible(show);
		m_darkMode.setVisible(show);
	}

	void ThemePreview::updateThemeColors()
	{
		UI::Themes::Theme* theme = Themes::getCurrentTheme();

		if (!theme)
		{
			LOG_ERROR("No current theme set");
			return;
		}

		auto defaultTheme = dynamic_cast<UI::Themes::DefaultTheme*>(theme);
		if (!defaultTheme)
		{
			LOG_ERROR("Current theme does not inherit from DefaultTheme");
			return;
		}

		auto colors = UI::Themes::createThemeColors(m_primaryHueSlider.getValue(),
													m_secondaryHueSlider.getValue(),
													m_chromaSlider.getValue(),
													m_darkMode.getChecked());
		defaultTheme->updateColors(colors);

		UI::Themes::refreshCurrentTheme();
		updateSwatches();
	}

} // namespace UI

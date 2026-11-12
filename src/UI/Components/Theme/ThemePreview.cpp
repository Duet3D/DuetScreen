/*
 * ThemePreview.cpp
 *
 *  Created on: 2025-08-06
 *      Author: Andy Everitt
 */

#include "ThemePreview.h"
#include "Debug.h"
#include "UI/Styles/Color.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "i18n/i18n.h"

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
			ZoneScoped;
			m_colorBox.addStyle(Themes::getLvglStyles().border);

			setFlexFlow(LV_FLEX_FLOW_COLUMN);
			setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			m_colorBox.setWidth(LV_PCT(100));
			m_colorBox.setFlexGrow(1);
			m_colorBox.setMinHeight(20);
			m_colorBox.setMinWidth(20);
			m_colorBox.setStyleBgOpa(LV_OPA_COVER);
			m_label.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			m_label.setMinWidth(LV_SIZE_CONTENT);
			m_label.setStyleTextAlign(LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
		}

		void setSwatchColor(lv_color_t color) { m_colorBox.setStyleBgColor(color, LV_PART_MAIN); }
		void setLabel(std::string_view text) { m_label.setText(text); }

		auto& getLabel() { return m_label; }

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
	{
		ZoneScoped;
		static const int32_t s_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		static const int32_t s_row_dsc[] = {
			LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
		setGridDsc(s_col_dsc, s_row_dsc);

		setGridCell(m_swatches, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

		setGridCell(m_primaryHueLabel, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
		setGridCell(m_primaryHueSlider, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

		setGridCell(m_secondaryHueLabel, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
		setGridCell(m_secondaryHueSlider, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);

		setGridCell(m_chromaLabel, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
		setGridCell(m_chromaSlider, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);

		setGridCell(m_darkModeLabel, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
		setGridCell(m_darkMode, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 4, 1);

		m_swatches.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_swatches.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_swatches.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		m_primaryHueLabel.setText(_("theme.primary_hue"));
		m_primaryHueSlider.setLabel(_("theme.primary_hue")); // for the numberpad
		m_primaryHueSlider.getLabel().hide();
		m_primaryHueSlider.setHeight(LV_SIZE_CONTENT);
		m_primaryHueSlider.setRange(0, 360);
		m_primaryHueSlider.setValue(0);
		m_primaryHueSlider.setValueChangedCallback([this](float) { updateThemeColors(); });
		m_primaryHueSlider.getLvSlider().setStyleBgOpa(LV_OPA_0);
		m_primaryHueSlider.getLvSlider().setStyleBgOpa(LV_OPA_0, LV_PART_INDICATOR);

		m_secondaryHueLabel.setText(_("theme.secondary_hue"));
		m_secondaryHueSlider.setLabel(_("theme.secondary_hue")); // for the numberpad
		m_secondaryHueSlider.getLabel().hide();
		m_secondaryHueSlider.setHeight(LV_SIZE_CONTENT);
		m_secondaryHueSlider.setRange(0, 360);
		m_secondaryHueSlider.setValue(0);
		m_secondaryHueSlider.setValueChangedCallback([this](float) { updateThemeColors(); });
		m_secondaryHueSlider.getLvSlider().setStyleBgOpa(LV_OPA_0);
		m_secondaryHueSlider.getLvSlider().setStyleBgOpa(LV_OPA_0, LV_PART_INDICATOR);

		m_chromaLabel.setText(_("theme.chroma"));
		m_chromaSlider.setLabel(_("theme.chroma")); // for the numberpad
		m_chromaSlider.getLabel().hide();
		m_chromaSlider.setHeight(LV_SIZE_CONTENT);
		m_chromaSlider.setRange(0.0f, 0.4f);
		m_chromaSlider.setIncrementValue(0.01f);
		m_chromaSlider.setValue(0.2f);
		m_chromaSlider.setValueChangedCallback([this](float) { updateThemeColors(); });

		m_darkModeLabel.setText(_("theme.dark_mode"));
		m_darkMode.setChecked(true);
		m_darkMode.setCheckedCallback([this](bool) { updateThemeColors(); });

		m_primaryHueSlider.getLvSlider().addEventCallback(
			[this](lv_event_t* e)
			{
				const lv_event_code_t code = lv_event_get_code(e);

				switch (code)
				{
				case LV_EVENT_SIZE_CHANGED:
				case LV_EVENT_STYLE_CHANGED:
				{
					auto& slider = m_primaryHueSlider.getLvSlider();
					const auto width = slider.getWidth();
					const auto height = slider.getHeight();
					m_primaryColorPreview.setSize(width, height);
					m_primaryColorPreview.setAlignTo(slider, LV_ALIGN_TOP_LEFT, 0, 0);
					m_primaryColorPreview.getCanvas().setLocalStyleProp(LV_STYLE_RADIUS,
																		slider.getStyleProp(LV_STYLE_RADIUS));
					break;
				}
				default:
					break;
				}
			},
			LV_EVENT_ALL);
		m_primaryColorPreview.setResolution(static_cast<uint32_t>(m_primaryHueSlider.getMax()), 1u);
		m_primaryColorPreview.showTitle(false);
		m_primaryColorPreview.showXScale(false);
		m_primaryColorPreview.showYScale(false);
		m_primaryColorPreview.setStylePad(0);
		m_primaryColorPreview.setStylePad(0, LV_PART_MAIN, Padding::ROW);
		m_primaryColorPreview.setStylePad(0, LV_PART_MAIN, Padding::COLUMN);
		// m_primaryColorPreview.moveToFront();
		m_primaryColorPreview.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);

		m_secondaryHueSlider.getLvSlider().addEventCallback(
			[this](lv_event_t* e)
			{
				const lv_event_code_t code = lv_event_get_code(e);

				switch (code)
				{
				case LV_EVENT_SIZE_CHANGED:
				case LV_EVENT_STYLE_CHANGED:
				{
					auto& slider = m_secondaryHueSlider.getLvSlider();
					const auto width = slider.getWidth();
					const auto height = slider.getHeight();
					m_secondaryColorPreview.setSize(width, height);
					m_secondaryColorPreview.setAlignTo(slider, LV_ALIGN_TOP_LEFT, 0, 0);
					m_secondaryColorPreview.getCanvas().setLocalStyleProp(LV_STYLE_RADIUS,
																		  slider.getStyleProp(LV_STYLE_RADIUS));
					break;
				}
				default:
					break;
				}
			},
			LV_EVENT_ALL);
		m_secondaryColorPreview.setResolution(static_cast<uint32_t>(m_secondaryHueSlider.getMax()), 1u);
		m_secondaryColorPreview.showTitle(false);
		m_secondaryColorPreview.showXScale(false);
		m_secondaryColorPreview.showYScale(false);
		m_secondaryColorPreview.setStylePad(0);
		m_secondaryColorPreview.setStylePad(0, LV_PART_MAIN, Padding::ROW);
		m_secondaryColorPreview.setStylePad(0, LV_PART_MAIN, Padding::COLUMN);
		// m_secondaryColorPreview.moveToFront();
		m_secondaryColorPreview.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);

		updateSwatches();
	}

	ThemePreview::~ThemePreview() = default;

	void ThemePreview::updateSwatches()
	{
		ZoneScoped;
		const auto& styles = Themes::getLvglStyles();
		const LabelledStyle swatch_styles[] = {
			{.name = "bg_dark", .style = styles.bg_dark, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg", .style = styles.bg, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg_light", .style = styles.bg_light, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg_color_primary", .style = styles.bg_color_primary, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg_color_primary_muted", .style = styles.bg_color_primary_muted, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg_color_secondary", .style = styles.bg_color_secondary, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "bg_color_secondary_muted",
			 .style = styles.bg_color_secondary_muted,
			 .color_prop = LV_STYLE_BG_COLOR},
#if 0
			{.name="bg_color_header", .style=styles.bg_color_header, .color_prop=LV_STYLE_BG_COLOR},
			{.name="bg_color_list_item", .style=styles.bg_color_list_item, .color_prop=LV_STYLE_BG_COLOR},
#endif
			{.name = "text", .style = styles.text, .color_prop = LV_STYLE_TEXT_COLOR},
			{.name = "text_muted", .style = styles.text_muted, .color_prop = LV_STYLE_TEXT_COLOR},
			{.name = "text_emphasis", .style = styles.text_emphasis, .color_prop = LV_STYLE_TEXT_COLOR},
			{.name = "text_header", .style = styles.text_header, .color_prop = LV_STYLE_TEXT_COLOR},
			{.name = "border", .style = styles.border, .color_prop = LV_STYLE_BORDER_COLOR},
			{.name = "border_muted", .style = styles.border_muted, .color_prop = LV_STYLE_BORDER_COLOR},
			{.name = "border_highlight", .style = styles.border_highlight, .color_prop = LV_STYLE_BORDER_COLOR},
			{.name = "success", .style = styles.bg_color_success, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "warning", .style = styles.bg_color_warning, .color_prop = LV_STYLE_BG_COLOR},
			{.name = "error", .style = styles.bg_color_error, .color_prop = LV_STYLE_BG_COLOR},
#if 0
			{.name="style_border_color_primary", .style=styles.border_color_primary,
			.color_prop=LV_STYLE_BORDER_COLOR},
			{.name="style_border_color_secondary", .style=styles.border_color_secondary,
			.color_prop=LV_STYLE_BORDER_COLOR},
#endif
		};

		m_swatches.clear();
		m_swatches.setItemCount(std::size(swatch_styles),
								[this, &swatch_styles](size_t index, LvObj& parent)
								{
									auto swatch = std::make_unique<Swatch>(fmt::format("{:d}", index), parent);
									swatch->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
									swatch->setMaxWidth(LV_PCT(100));
									auto& style = swatch_styles[index];
									swatch->setLabel(_(fmt::format("settings.style.{:s}", style.name)));

									lv_style_value_t value;
									if (lv_style_get_prop(style.style, style.color_prop, &value) == LV_STYLE_RES_FOUND)
									{
										swatch->setSwatchColor(value.color);
									}
									else
									{
										LOG_ERROR("Failed to get color for style: {}", style.name);
									}

									if (style.name == "text_muted")
									{
										swatch->getLabel().addStyle(Themes::getLvglStyles().text_muted);
									}
									else if (style.name == "text_emphasis")
									{
										swatch->getLabel().addStyle(Themes::getLvglStyles().text_emphasis);
									}
									else if (style.name == "text_header")
									{
										swatch->getLabel().addStyle(Themes::getLvglStyles().text_header);
									}

									// swatch->addSwatchStyle(style.style);

									return swatch;
								});
	}

	void ThemePreview::updateSliders(size_t primaryHue, size_t secondaryHue, float chroma, bool darkMode)
	{
		ZoneScoped;
		m_primaryHueSlider.setSendMode(Slider::SendMode::DISABLED);
		m_secondaryHueSlider.setSendMode(Slider::SendMode::DISABLED);
		m_chromaSlider.setSendMode(Slider::SendMode::DISABLED);

		m_primaryHueSlider.setValue(static_cast<float>(primaryHue));
		m_secondaryHueSlider.setValue(static_cast<float>(secondaryHue));
		m_chromaSlider.setValue(chroma);
		m_darkMode.setChecked(darkMode);

		m_primaryHueSlider.setSendMode(Slider::SendMode::VALUE_CONFIRMED);
		m_secondaryHueSlider.setSendMode(Slider::SendMode::VALUE_CONFIRMED);
		m_chromaSlider.setSendMode(Slider::SendMode::VALUE_CONFIRMED);
	}

	void ThemePreview::setPrimaryHue(size_t hue)
	{
		ZoneScoped;
		m_primaryHueSlider.setValue(static_cast<float>(hue));
	}

	void ThemePreview::setSecondaryHue(size_t hue)
	{
		ZoneScoped;
		m_secondaryHueSlider.setValue(static_cast<float>(hue));
	}

	void ThemePreview::setChroma(float chroma)
	{
		ZoneScoped;
		m_chromaSlider.setValue(chroma);
	}

	void ThemePreview::setDarkMode(bool enable)
	{
		ZoneScoped;
		m_darkMode.setChecked(enable);
	}

	void ThemePreview::showControls(bool show)
	{
		ZoneScoped;
		m_primaryHueLabel.setVisible(show);
		m_primaryHueSlider.setVisible(show);
		m_primaryColorPreview.setVisible(show);
		m_secondaryHueLabel.setVisible(show);
		m_secondaryHueSlider.setVisible(show);
		m_secondaryColorPreview.setVisible(show);
		m_chromaLabel.setVisible(show);
		m_chromaSlider.setVisible(show);
		m_darkModeLabel.setVisible(show);
		m_darkMode.setVisible(show);

		if (show)
		{
			m_primaryHueSlider.getLvSlider().sendEvent(LV_EVENT_SIZE_CHANGED);
			m_secondaryHueSlider.getLvSlider().sendEvent(LV_EVENT_SIZE_CHANGED);
		}
	}

	void ThemePreview::setNumberPad(ModalNumberPad* numberPad)
	{
		ZoneScoped;
		m_numberPad = numberPad;

		m_primaryHueSlider.setNumberPad(numberPad);
		m_secondaryHueSlider.setNumberPad(numberPad);
		m_chromaSlider.setNumberPad(numberPad);
	}

	void ThemePreview::updateThemeColors()
	{
		ZoneScoped;
		UI::Themes::Theme* theme = Themes::getCurrentTheme();
		if (!theme)
		{
			LOG_ERROR("No current theme set");
			return;
		}
		if (!theme->isCustomisable())
		{
			LOG_ERROR("Theme '{:s}' has a fixed palette", theme->getName());
			return;
		}

		theme->setPalette({.primaryHue = static_cast<uint16_t>(m_primaryHueSlider.getValue()),
						   .secondaryHue = static_cast<uint16_t>(m_secondaryHueSlider.getValue()),
						   .chroma = m_chromaSlider.getValue(),
						   .dark = m_darkMode.getChecked()});
		UI::Themes::refreshCurrentTheme();

		const auto colors = theme->getColors();
		renderColorPreview(m_primaryColorPreview, colors.primary.getL(), colors.primary.getC());
		renderColorPreview(m_secondaryColorPreview, colors.secondary.getL(), colors.secondary.getC());
		updateSwatches();
	}

	void ThemePreview::renderColorPreview(Canvas& canvas, float luminance, float chroma)
	{
		ZoneScoped;
		uint32_t barWidth, barHeight;
		canvas.getResolution(barWidth, barHeight);
		for (uint32_t x = 0; x < barWidth; x++)
		{
			canvas.drawPx(x, 0, static_cast<lv_color_t>(Color(luminance, chroma, static_cast<float>(x))), LV_OPA_COVER);
		}
	}

	void ThemePreview::onShow()
	{
		ZoneScoped;
		m_primaryHueSlider.getLvSlider().sendEvent(LV_EVENT_SIZE_CHANGED);
		m_secondaryHueSlider.getLvSlider().sendEvent(LV_EVENT_SIZE_CHANGED);
		UI::Themes::Theme* theme = Themes::getCurrentTheme();
		if (!theme)
		{
			LOG_ERROR("No current theme set");
			return;
		}
		if (!theme->isCustomisable())
		{
			return;
		}

		const auto colors = theme->getColors();
		renderColorPreview(m_primaryColorPreview, colors.primary.getL(), colors.primary.getC());
		renderColorPreview(m_secondaryColorPreview, colors.secondary.getL(), colors.secondary.getC());
		updateSwatches();
	}
} // namespace UI

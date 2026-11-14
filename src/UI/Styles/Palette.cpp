/*
 * Palette.cpp
 */

#include "Palette.h"
#include <algorithm>
#include <charconv>

namespace UI::Themes
{
	static constexpr float s_minColorChroma = 0.12f; // Accents are never fully grey, nor dull
	static constexpr float s_mutedChroma = 0.6f;	 // A muted accent's chroma, as a share of its accent's
	static constexpr float s_maxLineChroma = 0.04f;	 // Text and lines are only tinted: colourful text is hard to read

	void forEachColor(ThemeColors& colors, const std::function<void(std::string_view name, Color& color)>& fn)
	{
		fn("bg_dark", colors.bg_dark);
		fn("bg", colors.bg);
		fn("bg_light", colors.bg_light);
		fn("primary", colors.primary);
		fn("primary_muted", colors.primary_muted);
		fn("secondary", colors.secondary);
		fn("secondary_muted", colors.secondary_muted);
		fn("text", colors.text);
		fn("text_muted", colors.text_muted);
		fn("text_header", colors.text_header);
		fn("border", colors.border);
		fn("border_muted", colors.border_muted);
		fn("highlight", colors.highlight);
		fn("shadow", colors.shadow);
		fn("success", colors.success);
		fn("warning", colors.warning);
		fn("error", colors.error);
	}

	float ColorAdjustment::Change::apply(float value) const
	{
		switch (op)
		{
		case Op::Add:
			return value + amount;
		case Op::Multiply:
			return value * amount;
		case Op::Set:
		default:
			return amount;
		}
	}

	std::optional<ColorAdjustment::Change> ColorAdjustment::Change::parse(std::string_view text)
	{
		Change change;
		if (text.starts_with('+'))
		{
			change.op = Op::Add;
			text.remove_prefix(1);
		}
		else if (text.starts_with('*'))
		{
			change.op = Op::Multiply;
			text.remove_prefix(1);
		}
		else if (text.starts_with('-'))
		{
			change.op = Op::Add; // "-0.1" adds a negative amount
		}

		const char* end = text.data() + text.size();
		auto [ptr, ec] = std::from_chars(text.data(), end, change.amount);
		if (text.empty() || ec != std::errc() || ptr != end)
		{
			return std::nullopt;
		}
		return change;
	}

	/**
	 * The palette before adjustments. Each colour's lightness is chosen for its role, so text stays readable on
	 * whatever it is drawn on whatever the hues and chroma:
	 * - Surfaces step away from the screen: the screen (bg_dark), a card on it (bg), a control on a card (bg_light).
	 * - Accents are filled behind body text (#text), so they keep a contrast of about 4.5:1 with it in both modes.
	 * - Muted accents sit between their accent and the background: darker in dark mode, a pale tint in light mode.
	 * - The state colours (success, warning, error) work both as a fill behind body text and as text on a card.
	 * - The highlight is a track or a grid line: plainly visible on a card, quieter than a border.
	 */
	static ThemeColors generateBase(PaletteInputs in)
	{
		ThemeColors colors;
		in.primaryHue = std::clamp<uint16_t>(in.primaryHue, 0u, 360u);
		in.secondaryHue = std::clamp<uint16_t>(in.secondaryHue, 0u, 360u);
		in.chroma = std::clamp<float>(in.chroma, 0.0f, 0.2f);

		const float bgChroma = in.chroma / 2;
		const float lineChroma = std::min(in.chroma, s_maxLineChroma);
		const float colorChroma = std::max(s_minColorChroma, in.chroma);
		const float mutedChroma = colorChroma * s_mutedChroma;
		const float colorL = in.dark ? 0.55f : 0.68f;
		const float mutedL = in.dark ? 0.38f : 0.87f;

		colors.bg_dark = Color(in.dark ? 0.16f : 0.93f, bgChroma, in.primaryHue);
		colors.bg = Color(in.dark ? 0.205f : 0.985f, bgChroma, in.primaryHue);
		colors.bg_light = Color(in.dark ? 0.28f : 1.0f, bgChroma, in.primaryHue);

		colors.primary = Color(colorL, colorChroma, in.primaryHue);
		colors.primary_muted = Color(mutedL, mutedChroma, in.primaryHue);
		colors.secondary = Color(colorL, colorChroma, in.secondaryHue);
		colors.secondary_muted = Color(mutedL, mutedChroma, in.secondaryHue);

		colors.text = Color(in.dark ? 0.96f : 0.2f, lineChroma, in.primaryHue);
		colors.text_muted = Color(in.dark ? 0.76f : 0.42f, lineChroma, in.primaryHue);
		colors.text_header = Color(in.dark ? 1.0f : 0.1f, lineChroma, in.primaryHue);

		colors.border = Color(in.dark ? 0.42f : 0.72f, lineChroma, in.primaryHue);
		colors.border_muted = Color(in.dark ? 0.31f : 0.86f, lineChroma, in.primaryHue);
		colors.highlight = Color(in.dark ? 0.36f : 0.84f, lineChroma, in.primaryHue);
		colors.shadow = Color(in.dark ? 0.08f : 0.4f, bgChroma, in.primaryHue);

		const float stateL = in.dark ? 0.56f : 0.64f;
		colors.success = Color(stateL, std::max(0.14f, in.chroma), 150);
		colors.warning = Color(in.dark ? 0.6f : 0.76f, std::max(0.14f, in.chroma), 75); // Amber is only amber if light
		colors.error = Color(stateL, std::max(0.18f, in.chroma), 27);
		return colors;
	}

	/** The colour called `name`, or nullptr */
	static Color* findColor(ThemeColors& colors, std::string_view name)
	{
		Color* found = nullptr;
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

	ThemeColors generatePalette(const PaletteInputs& inputs,
								const std::vector<ColorAdjustment>& adjustments,
								std::vector<std::string>* errors)
	{
		ThemeColors colors = generateBase(inputs);
		for (const auto& adjustment : adjustments)
		{
			if ((adjustment.when == ColorAdjustment::When::Dark && !inputs.dark) ||
				(adjustment.when == ColorAdjustment::When::Light && inputs.dark))
			{
				continue;
			}

			Color* target = findColor(colors, adjustment.color);
			const Color* from = adjustment.from.empty() ? target : findColor(colors, adjustment.from);
			if (target == nullptr || from == nullptr)
			{
				if (errors)
				{
					errors->push_back("unknown palette colour '" +
									  (target == nullptr ? adjustment.color : adjustment.from) + "'");
				}
				continue;
			}

			// Read both from the starting colour before changing anything
			const float lightness = from->getL();
			const float chroma = from->getC();
			if (adjustment.lightness)
			{
				target->setL(std::clamp(adjustment.lightness->apply(lightness), 0.0f, 1.0f));
			}
			if (adjustment.chroma)
			{
				target->setC(std::max(adjustment.chroma->apply(chroma), 0.0f));
			}
		}
		return colors;
	}
} // namespace UI::Themes

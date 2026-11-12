/*
 * Palette.cpp
 */

#include "Palette.h"
#include <algorithm>
#include <charconv>

namespace UI::Themes
{
	static constexpr float s_minColorChroma = 0.1f; // Accents are never fully grey
	static constexpr float s_mutedDiff = 0.2f;		// How much darker a muted accent is

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

	/** The palette before adjustments */
	static ThemeColors generateBase(PaletteInputs in)
	{
		ThemeColors colors;
		in.primaryHue = std::clamp<uint16_t>(in.primaryHue, 0u, 360u);
		in.secondaryHue = std::clamp<uint16_t>(in.secondaryHue, 0u, 360u);
		in.chroma = std::clamp<float>(in.chroma, 0.0f, 0.2f);

		const float bgChroma = in.chroma / 2;
		const float colorChroma = std::max(s_minColorChroma, in.chroma);
		const float colorL = in.dark ? 0.45f : 0.7f;
		const float mutedL = colorL - s_mutedDiff;

		colors.bg_dark = Color(in.dark ? 0.1f : 0.92f, bgChroma, in.primaryHue);
		colors.bg = Color(in.dark ? 0.15f : 0.96f, bgChroma, in.primaryHue);
		colors.bg_light = Color(in.dark ? 0.25f : 1.0f, bgChroma, in.primaryHue);

		colors.primary = Color(colorL, colorChroma, in.primaryHue);
		colors.primary_muted = Color(mutedL, colorChroma, in.primaryHue);
		colors.secondary = Color(colorL, colorChroma, in.secondaryHue);
		colors.secondary_muted = Color(mutedL, colorChroma, in.secondaryHue);

		colors.text = Color(in.dark ? 0.96f : 0.15f, in.chroma, in.primaryHue);
		colors.text_muted = Color(in.dark ? 0.82f : 0.25f, in.chroma, in.primaryHue);
		colors.text_header = Color(in.dark ? 1.0f : 0.0f, in.chroma, in.primaryHue);

		colors.border = Color(in.dark ? 0.40f : 0.6f, in.chroma, in.primaryHue);
		colors.border_muted = Color(in.dark ? 0.30f : 0.7f, in.chroma, in.primaryHue);
		colors.highlight = Color(in.dark ? 0.70f : 1.0f, in.chroma, in.primaryHue);
		colors.shadow = Color(in.dark ? 0.2f : 0.4f, bgChroma, in.primaryHue);

		colors.success = Color(in.dark ? 0.6f : 0.8f, std::max(0.15f, in.chroma), 144);
		colors.warning = Color(in.dark ? 0.75f : 0.85f, std::max(0.17f, in.chroma), 78);
		colors.error = Color(in.dark ? 0.6f : 0.8f, std::max(0.2f, in.chroma), 27);
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

/*
 * Palette.h
 *
 * A theme's colours are generated from a few inputs: two hues, a chroma and light or dark mode. Every named colour
 * (bg, primary, text, …) is a lightness and chroma chosen for its role, in the OKLCH colour space, so any hue gives a
 * readable, consistent palette. A theme can then adjust individual colours.
 *
 * In a theme file:
 *
 *     <palette primary_hue="340" secondary_hue="45" chroma="0.05" dark="true" customisable="true">
 *         <color name="primary" chroma="+0.1"/>                                  <!-- adjust -->
 *         <color name="primary" lightness="+0.2" when="dark"/>                   <!-- only in dark mode -->
 *         <color name="primary_muted" from="primary" chroma="*0.5" lightness="+0.2" when="dark"/>
 *     </palette>
 *
 * Adjustments run in order. `lightness` and `chroma` take `+x`, `-x`, `*x` or a plain value; `from` starts from
 * another colour's current value. Lightness is kept within 0-1 and chroma at or above 0.
 */

#pragma once

#include "Color.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Themes
{
	/** The named colours of a theme */
	struct ThemeColors
	{
		Color bg_dark;		   // Screen background
		Color bg;			   // Card background
		Color bg_light;		   // Raised background, e.g. list items
		Color primary;		   // Main accent: buttons, indicators
		Color primary_muted;   // Darker primary
		Color secondary;	   // Second accent: action buttons
		Color secondary_muted; // Darker secondary

		Color text;		   // Body text
		Color text_muted;  // Secondary text
		Color text_header; // Headings

		Color border;		// Card and button borders
		Color border_muted; // Subtle borders
		Color highlight;	// Highlights, tracks
		Color shadow;		// Shadows

		Color success; // Success state
		Color warning; // Warning state
		Color error;   // Error state
	};

	/** Call `fn(name, colour)` for every colour, with the name theme files use */
	void forEachColor(ThemeColors& colors, const std::function<void(std::string_view name, Color& color)>& fn);

	/** What a palette is generated from */
	struct PaletteInputs
	{
		uint16_t primaryHue = 0;   // Hue of primary, background and text colours, degrees
		uint16_t secondaryHue = 0; // Hue of secondary colours, degrees
		float chroma = 0.0f;	   // Colourfulness, 0 (grey) to 0.2
		bool dark = false;		   // Dark mode
	};

	/** A change to one generated colour (`<color>` inside `<palette>`) */
	struct ColorAdjustment
	{
		/** How to change a value: add, multiply or replace */
		struct Change
		{
			enum class Op
			{
				Add,	  // value + amount
				Multiply, // value * amount
				Set		  // amount
			};
			Op op = Op::Set;	 // What to do
			float amount = 0.0f; // By how much

			/** Apply the change to `value` */
			float apply(float value) const;

			/** Parse "+0.1", "-0.1", "*0.5" or "0.3". @return nullopt if malformed */
			static std::optional<Change> parse(std::string_view text);
		};

		/** When the adjustment applies */
		enum class When
		{
			Always,
			Dark, // Only in dark mode
			Light // Only in light mode
		};

		std::string color;				  // Colour to change
		std::string from;				  // Start from this colour's value instead; empty to keep its own
		std::optional<Change> lightness; // Change to lightness
		std::optional<Change> chroma;	  // Change to chroma
		When when = When::Always;		  // When to apply
	};

	/**
	 * Generate a palette and apply `adjustments` in order
	 * @param errors Receives a message for each adjustment naming an unknown colour
	 */
	ThemeColors generatePalette(const PaletteInputs& inputs,
								const std::vector<ColorAdjustment>& adjustments,
								std::vector<std::string>* errors = nullptr);
} // namespace UI::Themes

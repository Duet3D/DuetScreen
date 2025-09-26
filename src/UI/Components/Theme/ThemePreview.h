/*
 * ThemePreview.h
 *
 *  Created on: 2025-08-06
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Input/Slider.h"
#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/List/List.h"

namespace UI
{
	class Swatch;

	class ThemePreview : public LvContainer
	{
	  public:
		ThemePreview(const std::string& name, LvObj& parent);

		void updateSwatches();
		void setPrimaryHue(size_t hue);
		void setSecondaryHue(size_t hue);
		void setChroma(float chroma);
		void setDarkMode(bool enable);

		void showControls(bool show);

	  private:
		void updateThemeColors();

		List<Swatch> m_swatches;
		Slider m_primaryHueSlider;
		Slider m_secondaryHueSlider;
		Slider m_chromaSlider;
		LvCheckbox m_darkMode;
	};
} // namespace UI

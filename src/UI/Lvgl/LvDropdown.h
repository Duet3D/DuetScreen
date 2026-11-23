/*
 * LvDropdown.h
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/generated/LvDropdown.gen.h"
#include <span>
#include <string>

namespace UI
{
	class LvDropdown : public LvDropdownGen
	{
	  public:
		using LvDropdownGen::LvDropdownGen;

		/* Override LvDropdownGen text methods so text is never NULL */

		void setText(const std::string& text);
		void clearText();

		template <typename T>
			requires std::convertible_to<T, std::string_view>
		void setOptions(std::span<T> options)
		{
			ZoneScoped;
			UI_LOCK();
			std::string opt;
			for (const auto& option : options)
			{
				opt += option;
				if (&option != &options.back())
				{
					opt += "\n";
				}
			}
			LvDropdownGen::setOptions(opt.c_str());
		}

		void addOption(const std::string& option, uint32_t pos) { LvDropdownGen::addOption(option.c_str(), pos); }
		using LvDropdownGen::setSelected;
		bool setSelected(const std::string& option);

		std::string getSelectedString() const;
		int32_t getOptionIndex(const std::string& option) const
		{
			return LvDropdownGen::getOptionIndex(option.c_str());
		}

	  private:
	};
} // namespace UI

/*
 * LvLabel.h
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/generated/LvLabel.gen.h"

namespace UI
{
	class LvLabel : public LvLabelGen
	{
	  public:
		using LvLabelGen::LvLabelGen;

		void setText(std::string_view text);

	  private:
	};
} // namespace UI

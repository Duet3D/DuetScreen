/*
 * LvImage.h
 *
 *  Created on: 2025-08-11
 */

#pragma once

#include "UI/Lvgl/generated/LvImage.gen.h"

namespace UI
{
	class LvImage : public LvImageGen
	{
	  public:
		LvImage(const std::string& name, LvObj& parent);
	};
} // namespace UI

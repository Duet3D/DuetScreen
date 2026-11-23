#pragma once

#include "UI/Lvgl/generated/LvTextarea.gen.h"

namespace UI
{
	class LvTextarea : public LvTextareaGen
	{
	  public:
		LvTextarea(const std::string& name, LvObj& parent);

		void setText(const std::string& text) { LvTextareaGen::setText(text.c_str()); }
		void addText(const std::string& text) { LvTextareaGen::addText(text.c_str()); }
		void setPlaceholderText(const std::string& text) { LvTextareaGen::setPlaceholderText(text.c_str()); }

		std::string_view getText() const { return LvTextareaGen::getText(); }
	};
} // namespace UI

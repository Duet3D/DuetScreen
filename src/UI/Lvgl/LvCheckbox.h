/*
 * LvCheckbox.h
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/generated/LvCheckbox.gen.h"

namespace UI
{
	class LvCheckbox : public LvCheckboxGen
	{
	  public:
		using checked_callback_t = std::function<void(bool)>;

		// Constructors
		LvCheckbox(const std::string& name, LvObj& parent);

		// Setters
		void setText(const std::string& text) { LvCheckboxGen::setText(text.c_str()); }
		void setTextStatic(const std::string& text) { LvCheckboxGen::setTextStatic(text.c_str()); }
		void setChecked(bool checked);
		void setCheckedCallback(checked_callback_t cb);

		// Getters
		std::string_view getText() const { return LvCheckboxGen::getText(); }
		bool getChecked() const;

	  private:
		checked_callback_t m_checkedCallback;
		bool m_checkedInitialised = false;
	};
} // namespace UI

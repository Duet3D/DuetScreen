/*
 * LvCheckbox.h
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class LvCheckbox : public LvObj
	{
	  public:
		using checked_callback_t = std::function<void(bool)>;

		// Constructors
		LvCheckbox(const std::string& name, LvObj& parent);

		// Setters
		void setText(std::string_view txt);
		void setTextStatic(const char* txt);
		void setChecked(bool checked);
		void setCheckedCallback(checked_callback_t cb);

		// Getters
		std::string_view getText() const;
		bool getChecked() const;

	  private:
      checked_callback_t m_checkedCallback;
	};
} // namespace UI

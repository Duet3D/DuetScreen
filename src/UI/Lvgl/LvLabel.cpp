/*
 * LvLabel.cpp
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#include "LvLabel.h"
#include "Debug.h"

namespace UI
{
	/**
	 * @brief Set the text of the label
	 * @param text The text to set
	 *
	 * @note Because lv_label_set_text will allocate memory for the new text, if we take a `const std::string&` we may
	 * end up allocating memory for the string twice. Instead using `std::string_view` allows us to avoid this extra
	 * allocation. However `lv_label_set_text` expects a null-terminated string, so we need to use
	 * `lv_label_set_text_fmt` to do this safely.
	 */
	void LvLabel::setText(std::string_view text)
	{
		ZoneScoped;
		/**
		 * Because lv_label_set_text will allocate memory for the new text, if we take a `const std::string&` we may end
		 * up allocating memory for the string twice */
		UI_LOCK();
		if (text == getText())
		{
			return;
		}
		if (text.data() == nullptr)
		{
			lv_label_set_text(getRootPtr(), nullptr);
			return;
		}
		lv_label_set_text_fmt(getRootPtr(), "%.*s", (int)text.length(), text.data());
	}
} // namespace UI

/*
 * LvDropdown.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "LvDropdown.h"
#include "Debug.h"

namespace UI
{
	LvDropdown::LvDropdown(const std::string& name, LvObj& parent)
		: LvObj(lv_dropdown_create, name, parent)
	{
	}

	void LvDropdown::setText(const std::string& text)
	{
		UI_LOCK();
		m_text = text;
		lv_dropdown_set_text(getRoot(), m_text.c_str());
	}

	const std::string& LvDropdown::getText() const
	{
		UI_LOCK();
		return m_text;
	}

	void LvDropdown::clearText()
	{
		UI_LOCK();
		m_text.clear();
		lv_dropdown_set_text(getRoot(), NULL);
	}

	void LvDropdown::setOptions(const std::string& options)
	{
		UI_LOCK();
		lv_dropdown_set_options(getRoot(), options.c_str());
	}

	void LvDropdown::setOptions(const std::vector<std::string>& options)
	{
		UI_LOCK();
		std::string opt;
		for (size_t i = 0; i < options.size(); ++i)
		{
			opt += options[i];
			if (i < options.size() - 1)
			{
				opt += "\n";
			}
		}
		lv_dropdown_set_options(getRoot(), opt.c_str());
	}

	void LvDropdown::addOption(const std::string& option, uint32_t pos)
	{
		UI_LOCK();
		lv_dropdown_add_option(getRoot(), option.c_str(), pos);
	}

	void LvDropdown::clearOptions()
	{
		UI_LOCK();
		lv_dropdown_clear_options(getRoot());
	}

	void LvDropdown::setSelected(uint32_t selected)
	{
		UI_LOCK();
		lv_dropdown_set_selected(getRoot(), selected);
	}

	bool LvDropdown::setSelected(const std::string& option)
	{
		UI_LOCK();
		int32_t index = getOptionIndex(option);
		if (index < 0)
		{
			if (!option.empty())
			{
				LOG_WARN("Failed to find option '{}' in dropdown", option);
			}
			setSelectedHighlight(false);
			return false;
		}
		setSelected(index);
		setSelectedHighlight(true);
		return true;
	}

	void LvDropdown::setDir(lv_dir_t dir)
	{
		UI_LOCK();
		lv_dropdown_set_dir(getRoot(), dir);
	}

	void LvDropdown::setSymbol(const void* symbol)
	{
		UI_LOCK();
		lv_dropdown_set_symbol(getRoot(), symbol);
	}

	void LvDropdown::setSelectedHighlight(bool en)
	{
		UI_LOCK();
		lv_dropdown_set_selected_highlight(getRoot(), en);
	}

	const char* LvDropdown::getOptions() const
	{
		UI_LOCK();
		return lv_dropdown_get_options(getRoot());
	}
	uint32_t LvDropdown::getSelected() const
	{
		UI_LOCK();
		return lv_dropdown_get_selected(getRoot());
	}

	uint32_t LvDropdown::getOptionCount() const
	{
		UI_LOCK();
		return lv_dropdown_get_option_count(getRoot());
	}

	std::string LvDropdown::getSelectedString() const
	{
		UI_LOCK();
		char buf[64];
		lv_dropdown_get_selected_str(getRoot(), buf, sizeof(buf));
		return std::string(buf);
	}

	int32_t LvDropdown::getOptionIndex(const std::string& option) const
	{
		UI_LOCK();
		return lv_dropdown_get_option_index(getRoot(), option.c_str());
	}

	const char* LvDropdown::getSymbol() const
	{
		UI_LOCK();
		return lv_dropdown_get_symbol(getRoot());
	}

	bool LvDropdown::getSelectedHighlight() const
	{
		UI_LOCK();
		return lv_dropdown_get_selected_highlight(getRoot());
	}

	lv_dir_t LvDropdown::getDir() const
	{
		UI_LOCK();
		return lv_dropdown_get_dir(getRoot());
	}

	void LvDropdown::open()
	{
		UI_LOCK();
		lv_dropdown_open(getRoot());
	}

	void LvDropdown::close()
	{
		UI_LOCK();
		lv_dropdown_close(getRoot());
	}

	bool LvDropdown::isOpen() const
	{
		UI_LOCK();
		return lv_dropdown_is_open(getRoot());
	}
} // namespace UI

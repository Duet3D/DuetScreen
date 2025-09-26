/*
 * LvDropdown.h
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include <vector>
#include <string>

namespace UI
{
	class LvDropdown : public LvObj
	{
	  public:
		LvDropdown(const std::string& name, LvObj& parent);

		void setText(const std::string& text);
		const std::string& getText() const;
		void clearText();
		lv_obj_t* getDropdownMenu() const { return getRoot(); }

		void setOptions(const std::string& options);
		void setOptions(const std::vector<std::string>& options);
		void addOption(const std::string& option, uint32_t pos = LV_DROPDOWN_POS_LAST);
		void clearOptions();
		void setSelected(uint32_t selected);
		bool setSelected(const std::string& option);
		void setDir(lv_dir_t dir);
		void setSymbol(const void* symbol);
		void setSelectedHighlight(bool en);

		const char* getOptions() const;
		uint32_t getSelected() const;
		uint32_t getOptionCount() const;
		std::string getSelectedString() const;
		int32_t getOptionIndex(const std::string& option) const;
		const char* getSymbol() const;
		bool getSelectedHighlight() const;
		lv_dir_t getDir() const;

		void open();
		void close();
		bool isOpen() const;

	  private:
		std::string m_text;
	};
} // namespace UI

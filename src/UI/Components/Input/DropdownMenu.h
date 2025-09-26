/*
 * TextBox.h
 *
 *  Created on: 2025-05-08
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvDropdown.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class DropdownMenu : public LvObj
	{
	  public:
		DropdownMenu(const std::string& name, LvObj& parent);
		DropdownMenu(const std::string& name, LvObj& parent, layout_t layout);

		void setLabel(const std::string& label);
		void setText(const std::string& text);
		const std::string& getText() const;
		void clearText();
		LvDropdown& getDropdownMenu() { return m_dropdown; }

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

		void addEventCallback(lv_event_cb_t cb, lv_event_code_t code, void* userData);

	  private:
		void init();

		LvLabel m_label;
		LvDropdown m_dropdown;
	};
} // namespace UI

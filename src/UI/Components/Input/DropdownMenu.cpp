/*
 * DropdownMenu.cpp
 *
 *  Created on: 2025-05-08
 *      Author: Andy Everitt
 */

#include "DropdownMenu.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	DropdownMenu::DropdownMenu(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_label("label", getRoot())
		, m_dropdown("dropdown", getRoot())
	{
		init();
	}

	DropdownMenu::DropdownMenu(const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(lv_obj_create, name, parent, layout)
		, m_label("label", getRoot())
		, m_dropdown("dropdown", getRoot())
	{
		init();
	}

	void DropdownMenu::init()
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// Label
		m_label.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		// Dropdown
		m_dropdown.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_dropdown.setFlexGrow(1);
		m_dropdown.clearOptions();

		m_dropdown.addStyle(Themes::getLvglStyles().input);

		setLabel("");
	}

	void DropdownMenu::setLabel(const std::string& label)
	{
		m_label.setFlag(LV_OBJ_FLAG_HIDDEN, label.empty());
		m_label.setText(label);
	}
	void DropdownMenu::setText(const std::string& text)
	{
		m_dropdown.setText(text);
	}
	const std::string& DropdownMenu::getText() const
	{
		return m_dropdown.getText();
	}

	void DropdownMenu::clearText()
	{
		m_dropdown.clearText();
	}

	void DropdownMenu::setOptions(const std::string& options)
	{
		m_dropdown.setOptions(options);
	}

	void DropdownMenu::setOptions(const std::vector<std::string>& options)
	{
		m_dropdown.setOptions(options);
	}

	void DropdownMenu::addOption(const std::string& option, uint32_t pos)
	{
		m_dropdown.addOption(option, pos);
	}

	void DropdownMenu::clearOptions()
	{
		m_dropdown.clearOptions();
	}

	void DropdownMenu::setSelected(uint32_t selected)
	{
		m_dropdown.setSelected(selected);
	}

	bool DropdownMenu::setSelected(const std::string& option)
	{
		return m_dropdown.setSelected(option);
	}

	void DropdownMenu::setDir(lv_dir_t dir)
	{
		m_dropdown.setDir(dir);
	}

	void DropdownMenu::setSymbol(const void* symbol)
	{
		m_dropdown.setSymbol(symbol);
	}

	void DropdownMenu::setSelectedHighlight(bool en)
	{
		m_dropdown.setSelectedHighlight(en);
	}

	const char* DropdownMenu::getOptions() const
	{
		return m_dropdown.getOptions();
	}

	uint32_t DropdownMenu::getSelected() const
	{
		return m_dropdown.getSelected();
	}

	uint32_t DropdownMenu::getOptionCount() const
	{
		return m_dropdown.getOptionCount();
	}

	std::string DropdownMenu::getSelectedString() const
	{
		return m_dropdown.getSelectedString();
	}

	int32_t DropdownMenu::getOptionIndex(const std::string& option) const
	{
		return m_dropdown.getOptionIndex(option);
	}

	const char* DropdownMenu::getSymbol() const
	{
		return m_dropdown.getSymbol();
	}

	bool DropdownMenu::getSelectedHighlight() const
	{
		return m_dropdown.getSelectedHighlight();
	}

	lv_dir_t DropdownMenu::getDir() const
	{
		return m_dropdown.getDir();
	}

	void DropdownMenu::open()
	{
		m_dropdown.open();
	}

	void DropdownMenu::close()
	{
		m_dropdown.close();
	}

	bool DropdownMenu::isOpen() const
	{
		return m_dropdown.isOpen();
	}

	void DropdownMenu::addEventCallback(lv_event_cb_t cb, lv_event_code_t code, void* userData)
	{
		m_dropdown.addEventCallback(cb, code, userData);
	}
} // namespace UI

/*
 * LvList.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "LvList.h"
#include "Debug.h"

namespace UI
{
	/**
	 * Create a list object
	 * @param name     Debug/name tag for this wrapper instance
	 * @param parent   pointer to an object, it will be the parent of the new list
	 */
	LvList::LvList(const std::string& name, LvObj& parent)
		: LvObj(lv_list_create, name, parent)
	{
		UI_LOCK();
	}

	/**
	 * Add text to a list
	 * @param txt  text of the new label
	 * @return     pointer to the created label
	 */
	lv_obj_t* LvList::addText(std::string_view txt)
	{
		UI_LOCK();
		return lv_list_add_text(getRoot(), txt.data());
	}

	/**
	 * Add button to a list
	 * @param icon icon for the button, when NULL it will have no icon
	 * @param txt  text of the new button, when NULL no text will be added
	 * @return     pointer to the created button
	 */
	lv_obj_t* LvList::addButton(const void* icon, std::string_view txt)
	{
		UI_LOCK();
		return lv_list_add_button(getRoot(), icon, txt.data());
	}

	/**
	 * Get text of a given list button
	 * @param btn  pointer to the button
	 * @return     text of btn, if btn doesn't have text "" will be returned
	 */
	std::string_view LvList::getButtonText(lv_obj_t* btn) const
	{
		UI_LOCK();
		return lv_list_get_button_text(getRoot(), btn);
	}

	/**
	 * Set text of a given list button
	 * @param btn  pointer to the button
	 * @param txt  pointer to the text
	 */
	void LvList::setButtonText(lv_obj_t* btn, std::string_view txt)
	{
		UI_LOCK();
		lv_list_set_button_text(getRoot(), btn, txt.data());
	}
} // namespace UI

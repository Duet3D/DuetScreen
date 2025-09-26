/*
 * LvButtonMatrix.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "LvButtonMatrix.h"
#include "Debug.h"

namespace UI
{
	LvButtonMatrix::LvButtonMatrix(const std::string& name, LvObj& parent)
		: LvObj(lv_buttonmatrix_create, name, parent)
	{
		UI_LOCK();
		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	}

	void LvButtonMatrix::setMap(const char* map[])
	{
		UI_LOCK();
		lv_buttonmatrix_set_map(getRoot(), map);
	}

	void LvButtonMatrix::setCtrlMap(const lv_buttonmatrix_ctrl_t ctrl_map[])
	{
		UI_LOCK();
		lv_buttonmatrix_set_ctrl_map(getRoot(), ctrl_map);
	}

	void LvButtonMatrix::setSelectedButton(uint32_t btn_id)
	{
		UI_LOCK();
		lv_buttonmatrix_set_selected_button(getRoot(), btn_id);
	}

	void LvButtonMatrix::setButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl)
	{
		UI_LOCK();
		lv_buttonmatrix_set_button_ctrl(getRoot(), btn_id, ctrl);
	}

	void LvButtonMatrix::clearButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl)
	{
		UI_LOCK();
		lv_buttonmatrix_clear_button_ctrl(getRoot(), btn_id, ctrl);
	}

	void LvButtonMatrix::setButtonCtrlAll(lv_buttonmatrix_ctrl_t ctrl)
	{
		UI_LOCK();
		lv_buttonmatrix_set_button_ctrl_all(getRoot(), ctrl);
	}

	void LvButtonMatrix::clearButtonCtrlAll(lv_buttonmatrix_ctrl_t ctrl)
	{
		UI_LOCK();
		lv_buttonmatrix_clear_button_ctrl_all(getRoot(), ctrl);
	}

	void LvButtonMatrix::setButtonWidth(uint32_t btn_id, uint32_t width)
	{
		UI_LOCK();
		lv_buttonmatrix_set_button_width(getRoot(), btn_id, width);
	}

	void LvButtonMatrix::setOneChecked(bool en)
	{
		UI_LOCK();
		lv_buttonmatrix_set_one_checked(getRoot(), en);
	}

	const char* const* LvButtonMatrix::getMap() const
	{
		UI_LOCK();
		return lv_buttonmatrix_get_map(getRoot());
	}

	uint32_t LvButtonMatrix::getSelectedButton() const
	{
		UI_LOCK();
		return lv_buttonmatrix_get_selected_button(getRoot());
	}

	const char* LvButtonMatrix::getButtonText(uint32_t btn_id) const
	{
		UI_LOCK();
		return lv_buttonmatrix_get_button_text(getRoot(), btn_id);
	}

	bool LvButtonMatrix::hasButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl) const
	{
		UI_LOCK();
		return lv_buttonmatrix_has_button_ctrl(getRoot(), btn_id, ctrl);
	}

	bool LvButtonMatrix::getOneChecked() const
	{
		UI_LOCK();
		return lv_buttonmatrix_get_one_checked(getRoot());
	}
} // namespace UI

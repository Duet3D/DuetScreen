/*
 * LvButtonMatrix.h
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
    class LvButtonMatrix : public LvObj
    {
      public:
		LvButtonMatrix(const std::string& name, LvObj& parent);

		void setMap(const char* map[]);
        void setCtrlMap(const lv_buttonmatrix_ctrl_t ctrl_map[]);
        void setSelectedButton(uint32_t btn_id);
        void setButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl);
        void clearButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl);
        void setButtonCtrlAll(lv_buttonmatrix_ctrl_t ctrl);
        void clearButtonCtrlAll(lv_buttonmatrix_ctrl_t ctrl);
        void setButtonWidth(uint32_t btn_id, uint32_t width);
        void setOneChecked(bool en);
        
        const char* const* getMap() const;
        uint32_t getSelectedButton() const;
        const char* getButtonText(uint32_t btn_id) const;
        bool hasButtonCtrl(uint32_t btn_id, lv_buttonmatrix_ctrl_t ctrl) const;
        bool getOneChecked() const;
      private:
    };
} // namespace UI

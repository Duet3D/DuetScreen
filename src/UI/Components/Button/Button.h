#pragma once

#include "UI/Components/Icon/Icon.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Core/View.h"
#include "lvgl/lvgl.h"

namespace UI
{
	class Button : public LvObj
	{
	  public:
		Button(const std::string& name, LvObj& parent);
		Button(const std::string& name, LvObj& parent, std::string_view text);
		Button(const std::string& name, LvObj& parent, std::string_view text, layout_t layout);

		void setText(std::string_view text);
		std::string_view getText() const { return m_label.getText(); }
		void addClickedCallback(lv_event_cb_t event_cb, void* user_data);
		void setIcon(const char* icon_path);
		void setCheckable(bool checkable);
		void setChecked(const bool checked);
		const bool getChecked() const;
		void setDisabled(bool disabled);

		LvLabel& getLabel() { return m_label; }
		lv_obj_t* getButton() const { return getRootPtr(); }
		Icon& getIcon() { return m_icon; }

	  private:
		void init(std::string_view text);

		Icon m_icon;
		LvLabel m_label;
	};
} // namespace UI

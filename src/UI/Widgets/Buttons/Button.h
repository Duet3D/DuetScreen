#pragma once

#include "UI/Widgets/Display/Icon.h"
#include "UI/Lvgl/LvLabel.h"
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
		/** For widgets built by the XML engine, whose parent has no C++ wrapper */
		Button(const std::string& name, lv_obj_t* parent, std::string_view text);

		void setText(std::string_view text);
		std::string_view getText() const { return m_label.getText(); }
		void addClickedCallback(lv_event_cb_t event_cb, void* user_data);
		void addClickedCallback(std::function<void(lv_event_t*)> event_cb);
		void setIcon(std::string_view icon_path);
		void setFixedIcon(const std::string& icon_path);
		void clearIcon();
		void setCheckable(bool checkable);
		void setChecked(const bool checked);
		bool getChecked() const;
		void setDisabled(bool disabled);

		LvLabel& getLabel() { return m_label; }
		lv_obj_t* getButton() const { return getRootPtr(); }
		Icon& getIcon() { return m_icon; }

	  private:
		void init(std::string_view text);
		void updateIconVisibility();

		Icon m_icon;
		LvLabel m_label;
	};
} // namespace UI

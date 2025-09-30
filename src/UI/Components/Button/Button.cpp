#include "Button.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Button::Button(const std::string& name, LvObj& parent)
		: LvObj(lv_button_create, name, parent)
		, m_icon("icon", getRoot())
		, m_label("label", getRoot())
	{
		init("");
	}

	Button::Button(const std::string& name, LvObj& parent, std::string_view text)
		: LvObj(lv_button_create, name, parent)
		, m_icon("icon", getRoot())
		, m_label("label", getRoot())
	{
		init(text);
	}

	Button::Button(const std::string& name, LvObj& parent, std::string_view text, layout_t layout)
		: LvObj(lv_button_create, name, parent, layout)
		, m_icon("icon", getRoot())
		, m_label("label", getRoot())
	{
		init(text);
	}

	void Button::init(std::string_view text)
	{
		UI_LOCK();
		setUserData(this);
		m_label.setUserData(this);

		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		setMinHeight(25);
		setMinWidth(25);

		// Initialise the label obj
		setText(text);
		m_label.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_label.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_label.setLongMode(LV_LABEL_LONG_MODE_DOTS);

		// icon
		m_icon.hide();
		m_icon.setWidth(LV_PCT(100));
		m_icon.setFlexGrow(1);
		m_icon.setMinHeight(25);
		m_icon.setMinWidth(25);

		addEventCallback(
			[](lv_event_t* e)
			{
				// Update the label width
				auto& btn = *static_cast<Button*>(lv_event_get_user_data(e));
				lv_obj_t* parent = btn.getParent();

				// while (parent != nullptr)
				// {

				// 	if (!(LV_COORD_IS_PCT(parent_width) && parent_min_width == LV_SIZE_CONTENT))
				// 	{
				// 		can_set_full_width = true;
				// 		break;
				// 	}

				// 	parent = lv_obj_get_parent(parent);
				// }
				lv_layout_t parent_layout = static_cast<lv_layout_t>(lv_obj_get_style_layout(parent, LV_PART_MAIN));
				lv_flex_flow_t parent_flex_flow = lv_obj_get_style_flex_flow(parent, LV_PART_MAIN);
				int32_t parent_width = lv_obj_get_style_width(parent, LV_PART_MAIN);
				int32_t parent_height = lv_obj_get_style_height(parent, LV_PART_MAIN);
				int32_t parent_min_width = lv_obj_get_style_min_width(parent, LV_PART_MAIN);
				int32_t parent_min_height = lv_obj_get_style_min_height(parent, LV_PART_MAIN);

				uint8_t flex_grow = lv_obj_get_style_flex_grow(btn, LV_PART_MAIN);
				int32_t width = lv_obj_get_style_width(btn, LV_PART_MAIN);
				int32_t height = lv_obj_get_style_height(btn, LV_PART_MAIN);
				int32_t min_width = lv_obj_get_style_min_width(btn, LV_PART_MAIN);
				int32_t min_height = lv_obj_get_style_min_height(btn, LV_PART_MAIN);

				if ((width != LV_SIZE_CONTENT && min_width != LV_SIZE_CONTENT) ||
					(parent_layout == LV_LAYOUT_FLEX && parent_flex_flow == LV_FLEX_FLOW_ROW && flex_grow > 0))
				{
					btn.m_label.setWidth(LV_PCT(100));
				}
				else
				{
					btn.m_label.setWidth(LV_SIZE_CONTENT);
				}

				if ((height != LV_SIZE_CONTENT && min_height != LV_SIZE_CONTENT) ||
					(parent_layout == LV_LAYOUT_FLEX && parent_flex_flow == LV_FLEX_FLOW_COLUMN && flex_grow > 0))
				{
					btn.m_label.setMaxHeight(LV_PCT(100));
				}
				else
				{
					btn.m_label.setMaxHeight(LV_SIZE_CONTENT);
				}
			},
			static_cast<lv_event_code_t>(LV_EVENT_STYLE_CHANGED),
			this);
	}

	void Button::setText(std::string_view text)
	{
		UI_LOCK();
		m_label.setText(text);
		m_label.setVisible(!text.empty());
	}

	void Button::addClickedCallback(lv_event_cb_t event_cb, void* user_data)
	{
		UI_LOCK();
		lv_obj_add_event_cb(getRoot(), event_cb, LV_EVENT_CLICKED, user_data);
	}

	void Button::setIcon(const char* icon_path)
	{
		UI_LOCK();
		// If the icon is null, remove the icon and center the label
		m_icon.setSrc(icon_path);

		// Check the icon has loaded correctly
		bool icon_loaded = m_icon.getSrc() != nullptr;
		if (icon_path && !icon_loaded)
		{
			LOG_ERROR("Failed to load icon from path: {}", icon_path);
		}
		m_icon.setVisible(icon_loaded);
	}

	void Button::setCheckable(bool checkable)
	{
		UI_LOCK();
		setFlag(LV_OBJ_FLAG_CHECKABLE, checkable);
	}

	void Button::setChecked(const bool checked)
	{
		UI_LOCK();
		setState(LV_STATE_CHECKED, checked);
	}

	const bool Button::getChecked() const
	{
		UI_LOCK();
		if (hasFlag(LV_OBJ_FLAG_CHECKABLE))
		{
			return hasState(LV_STATE_CHECKED);
		}
		return false;
	}

	void Button::setDisabled(bool disabled)
	{
		setState(LV_STATE_DISABLED, disabled);
	}
} // namespace UI

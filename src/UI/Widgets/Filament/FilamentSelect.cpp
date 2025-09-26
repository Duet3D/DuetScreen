/*
 * FilamentSelect.cpp
 *
 *  Created on: 2025-07-15
 *      Author: Andy Everitt
 */

#include "FilamentSelect.h"
#include "Debug.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	class FilamentSelect::ToolItem : public ListItem
	{
	  public:
		ToolItem(const size_t index, LvObj& parent, FilamentSelect& widget)
			: ListItem(index, parent)
			, m_toolName("tool", getRoot())
			, m_filament("filament", getRoot())
			, m_widget(widget)
		{
			UI_LOCK();
			setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
			setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			setSize(LV_PCT(100), LV_SIZE_CONTENT);

			m_toolName.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			m_filament.setHeight(LV_SIZE_CONTENT);
			m_filament.setFlexGrow(1);
			m_filament.setMinWidth(LV_SIZE_CONTENT);
			m_filament.setUserData(&m_filament);
			m_filament.addClickedCallback(onToolSelectEvent, this);
		}

		void setToolName(std::string_view name) { m_toolName.setText(name); }

		void setFilamentName(std::string_view filament)
		{
			UI_LOCK();
			m_filament.setText(filament);
		}

	  private:
		static void onToolSelectEvent(lv_event_t* e)
		{
			auto& control = *static_cast<ToolItem*>(lv_event_get_user_data(e));
			auto& btn = *static_cast<Button*>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));

			auto presenter = control.m_widget.getPresenter();
			presenter->setSelectedToolBySlot(control.getIndex());
		}

		LvLabel m_toolName;
		Button m_filament;
		FilamentSelect& m_widget;
	};

	FilamentSelect::FilamentSelect(const std::string& name, LvObj& parent, lv_obj_t* messageBoxParent)
		: View(name, parent)
		, m_header("header", getRoot())
		, m_cont("cont", getRoot())
		, m_toolList("tool_list", m_cont)
		, m_confirmation(
			  "confirmation", messageBoxParent ? messageBoxParent : getRoot(), layout_t(0, 0, 50, LV_SIZE_CONTENT))
		, m_filamentOptions("filament_options", m_confirmation.getBody())
		, m_unload("unload", m_confirmation.getFooter(), _("unload"))
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_cont.setWidth(LV_PCT(100));
		m_cont.setFlexGrow(1);
		m_cont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		m_toolList.setSize(LV_PCT(100), LV_PCT(100));
		m_filamentOptions.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_filamentOptions.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		// m_filamentOptions.setAlign(LV_ALIGN_OUT_BOTTOM_MID, 0, 0); // Position outside the visible area initially

		m_toolList.setListFlow(LV_FLEX_FLOW_COLUMN);
		m_toolList.getListContainer().setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_toolList.setListGrow(1);

		m_header.setText(_("filament_management_header"));
		m_toolList.setTitle(_("filament_select_tool_list_header"));

		m_confirmation.setMaxHeight(LV_PCT(70));
		m_confirmation.cancelVisible(true);
		m_confirmation.setOkBtnText(_("confirm_filament_change"));
		m_confirmation.getOkBtn().addStyle(Themes::getLvglStyles().actionBtn);
		m_unload.addStyle(Themes::getLvglStyles().actionBtn);
		m_confirmation.setOkCallback(
			[this]()
			{
				auto selectedFilament = m_filamentOptions.getItem(m_selectedFilamentIndex);
				if (!selectedFilament)
				{
					return;
				}
				getPresenter()->setFilament(selectedFilament->getLabel().getText());
			});

		m_filamentOptions.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_filamentOptions.getListContainer().setFlexAlign(
			LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_unload.setHeight(LV_SIZE_CONTENT);
		m_unload.setFlexGrow(1);
		m_unload.addClickedCallback(
			[](lv_event_t* e)
			{
				auto& control = *static_cast<FilamentSelect*>(lv_event_get_user_data(e));
				control.getPresenter()->unloadFilament();
				control.m_confirmation.close();
			},
			this);
	}

	void FilamentSelect::setToolCount(size_t count)
	{
		UI_LOCK();
		m_toolList.setItemCount(count, *this);
	}

	void FilamentSelect::setToolData(size_t index, std::string_view toolName, std::string_view filamentName)
	{
		UI_LOCK();
		if (index >= m_toolList.getItemCount())
		{
			LOG_WARN("Index {} out of bounds for tool data in {}", index, getName());
			return;
		}

		auto item = m_toolList.getItem(index);
		if (!item)
		{
			LOG_ERROR("Failed to get tool item at index {} in {}", index, getName());
			return;
		}
		item->setToolName(toolName);
		item->setFilamentName(filamentName);
	}

	void FilamentSelect::setFilamentOptions(const std::vector<std::string>& options)
	{
		UI_LOCK();
		LOG_DBG("Setting filament options for {}", getName());
		m_filamentOptions.clear();
		m_filamentOptions.setItemCount(options.size(),
									   [this, &options](size_t index, LvObj& parent) -> std::shared_ptr<Button>
									   {
										   auto btn = std::make_shared<Button>(fmt::format("{}", index), parent);
										   btn->setText(options[index]);
										   btn->setFlexGrow(1);
										   //    btn->setMinWidth(LV_SIZE_CONTENT);
										   btn->setUserData(reinterpret_cast<void*>(index));
										   btn->addClickedCallback(onFilamentOptionClicked, this);
										   return btn;
									   });
	}

	void FilamentSelect::showToolSelect(bool show)
	{
		m_toolList.setVisible(show);
	}

	void FilamentSelect::showSelection(std::string_view toolName, std::string_view filamentName)
	{
		UI_LOCK();
		m_confirmation.setTitle(fmt::format(fmt::runtime(_("filament_select_tool")), toolName));
		for (size_t i = 0; i < m_filamentOptions.getItemCount(); i++)
		{
			auto item = m_filamentOptions.getItem(i);
			if (!item)
			{
				LOG_ERROR("Failed to get filament option item at index {} in {}", i, getName());
				continue;
			}
			bool match = item->getText() == filamentName;
			item->setVisible(!match);
		}
		m_unload.setVisible(!filamentName.empty());
		m_confirmation.getOkBtn().hide();
		openModal(&m_confirmation);
	}

	void FilamentSelect::setSelectedFilament(std::string_view filamentName)
	{
		LOG_DBG("Setting selected filament to {}", filamentName);
		UI_LOCK();
		if (m_confirmation.isVisible())
		{
			return;
		}
		for (size_t i = 0; i < m_filamentOptions.getItemCount(); i++)
		{
			auto item = m_filamentOptions.getItem(i);
			if (!item)
			{
				LOG_ERROR("Failed to get filament option item at index {} in {}", i, getName());
				continue;
			}
			bool match = item->getLabel().getText() == filamentName;
			item->setChecked(match);
			if (match)
			{
				m_selectedFilamentIndex = i;
			}
		}
	}

	void FilamentSelect::onFilamentOptionClicked(lv_event_t* e)
	{
		auto& control = *static_cast<FilamentSelect*>(lv_event_get_user_data(e));
		lv_obj_t* obj = lv_event_get_target_obj(e);
		size_t index = reinterpret_cast<size_t>(lv_obj_get_user_data(obj));
		auto presenter = control.getPresenter();

		auto btn = control.m_filamentOptions.getItem(index);
		if (!btn)
		{
			return;
		}

		std::string_view selectedFilament = btn->getText();
		auto prev_btn = control.m_filamentOptions.getItem(control.m_selectedFilamentIndex);
		if (prev_btn)
		{
			prev_btn->setChecked(false);
		}
		btn->setChecked(true);
		control.m_selectedFilamentIndex = index;
		control.m_unload.hide();
		control.m_confirmation.getOkBtn().show();
	}

	void FilamentSelect::onShow()
	{
		m_confirmation.hide();
	}
} // namespace UI

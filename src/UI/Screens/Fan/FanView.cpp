#include "FanView.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	FanView::FanView(LvObj& parent)
		: View("fan_view", parent, layout_t(0, 0, 100, 100))
		, m_fans("list", getRoot())
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);

		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

		m_fans.setTitle(_("fan_header"));
		m_fans.setSize(LV_PCT(100), LV_PCT(100));
		m_fans.setListGrow(1);
	}

	void FanView::setFanCount(size_t count)
	{
		m_fans.setItemCount(count, *this);
	}

	void FanView::setFanLabel(size_t index, std::string_view label)
	{
		UI_LOCK();
		auto fan = m_fans.getItem(index);
		if (!fan)
		{
			return;
		}
		fan->setLabel(label);
	}

	void FanView::setFanValue(size_t index, uint32_t value)
	{
		UI_LOCK();
		auto fan = m_fans.getItem(index);
		if (!fan)
		{
			return;
		}

		fan->setValue(value);
	}

	FanView::FanItem::FanItem(size_t index, LvObj& parent, FanView& view)
		: ListItem(index, parent)
		, m_view(view)
		, m_off(utils::format("fan_off_%d", index).c_str(), getRoot(), _("off"))
		, m_slider(utils::format("fan_slider_%d", index).c_str(), getRoot())
		, m_max(utils::format("fan_max_%d", index).c_str(), getRoot(), _("max"))
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().card);

		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_slider.setFlexGrow(1);

		m_slider.setLabel(_("fan"));
		m_slider.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_slider.setRange(0, 100);
		m_slider.setValueChangedCallback([this](int32_t value) { m_view.m_presenter->setFanSpeed(getIndex(), value); });
		m_slider.setStylePad(5, LV_PART_MAIN, Padding::ALL);
		m_slider.addStyle(Themes::getLvglStyles().no_border, 0);

		m_off.addClickedCallback(onFanOffClicked, this);
		m_max.addClickedCallback(onFanMaxClicked, this);
	}

	void FanView::FanItem::setLabel(std::string_view label)
	{
		UI_LOCK();
		m_slider.setLabel(label);
	}

	void FanView::FanItem::setValue(uint32_t value)
	{
		UI_LOCK();
		if (m_slider.isFocused())
		{
			return;
		}
		m_slider.setValue(value);
	}

	void FanView::FanItem::onFanOffClicked(lv_event_t* e)
	{
		UI_LOCK();
		FanItem* item = static_cast<FanItem*>(lv_event_get_user_data(e));
		item->m_view.m_presenter->setFanSpeed(item->getIndex(), 0);
	}

	void FanView::FanItem::onFanMaxClicked(lv_event_t* e)
	{
		UI_LOCK();
		FanItem* item = static_cast<FanItem*>(lv_event_get_user_data(e));
		item->m_view.m_presenter->setFanSpeed(item->getIndex(), 100);
	}
} // namespace UI

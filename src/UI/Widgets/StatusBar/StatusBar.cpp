#include "StatusBar.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	StatusBar::StatusBar(LvObj& parent)
		: View("status_bar", parent)
		, m_duetStatus("status", getRoot())
		, m_duetName("name", getRoot())
		, m_time("time", getRoot())
	{
		activate();

		UI_LOCK();
		// setFlexFlow(LV_FLEX_FLOW_ROW);
		// setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_duetStatus.setAlign(LV_ALIGN_LEFT_MID);
		m_duetName.setAlign(LV_ALIGN_CENTER);
		m_time.setAlign(LV_ALIGN_RIGHT_MID);
	}

	void StatusBar::setDuetName(std::string_view name)
	{
		UI_LOCK();
		m_duetName.setText(name);
	}

	void StatusBar::setDuetStatus(std::string_view status)
	{
		UI_LOCK();
		m_duetStatus.setText(status);
	}

	void StatusBar::setTime(std::string_view time)
	{
		UI_LOCK();
		m_time.setText(time);
	}
} // namespace UI

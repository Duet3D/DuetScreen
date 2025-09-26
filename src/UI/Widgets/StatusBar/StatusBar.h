#pragma once

#include "StatusBarPresenter.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Core/View.h"

namespace UI
{
	class StatusBar : public View<StatusBarPresenter>
	{
	  public:
		StatusBar(LvObj& parent);

		void setDuetName(std::string_view name);
		void setDuetStatus(std::string_view status);
		void setTime(std::string_view time);

	  private:
		LvLabel m_duetName;
		LvLabel m_duetStatus;
		LvLabel m_time;
	};
} // namespace UI
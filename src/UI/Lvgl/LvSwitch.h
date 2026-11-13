#pragma once

#include "UI/Lvgl/generated/LvSwitch.gen.h"

namespace UI
{
	class LvSwitch : public LvSwitchGen
	{
	  public:
		using checked_callback_t = std::function<void(bool)>;

		LvSwitch(const std::string& name, LvObj& parent);

		void setCheckedCallback(checked_callback_t cb)
		{
			UI_LOCK();
			m_checkedCallback = std::move(cb);
		}
		void setChecked(const bool checked);
		bool getChecked() const { return hasState(LV_STATE_CHECKED); }

	  private:
		checked_callback_t m_checkedCallback;
		bool m_checkedInitialised = false;
	};
} // namespace UI

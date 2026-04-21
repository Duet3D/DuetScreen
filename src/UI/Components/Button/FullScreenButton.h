#pragma once

#include "Button.h"

namespace UI
{
	class FullScreenButton : public Button
	{
	  public:
		FullScreenButton(const std::string& name, LvObj& parent);

	  private:
		void onClicked(lv_event_t* e);
	};
} // namespace UI
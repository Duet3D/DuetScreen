#pragma once

#include "FanPresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"

namespace UI
{
	class FanView : public View<FanPresenter>
	{
	  public:
		class FanItem : ListItem
		{
		  public:
			FanItem(size_t index, LvObj& parent, FanView& view);

			void setLabel(std::string_view label);
			void setValue(uint32_t value);

		  private:
			static void onFanOffClicked(lv_event_t* e);
			static void onFanMaxClicked(lv_event_t* e);

			// Fans
			FanView& m_view;
			Button m_off;
			Slider m_slider;
			Button m_max;
		};

		FanView(LvObj& parent);

		size_t getFanCount() const { return m_fans.getItemCount(); }
		void setFanCount(size_t count);
		void setFanLabel(size_t index, std::string_view label);
		void setFanValue(size_t index, uint32_t value);

	  private:
		List<FanItem> m_fans;
	};
} // namespace UI
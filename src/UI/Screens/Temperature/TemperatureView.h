#pragma once

#include "TemperaturePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/List/List.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Core/View.h"
#include "UI/Widgets/Filament/FilamentSelect.h"
#include "UI/Widgets/Temperature/ToolControl.h"
#include "UI/Widgets/ToolList/ToolList.h"

namespace UI
{
	class TemperatureView : public View<TemperaturePresenter>
	{
	  public:
		friend class ToolItem;
		friend class TemperaturePresenter;

		TemperatureView(LvObj& parent);

		auto& getTools() { return m_tools; }
		auto& getBeds() { return m_beds; }
		auto& getChambers() { return m_chambers; }
		auto& getNumberPad() { return m_numberPad; }

	  private:
		virtual void onShow() override;
		virtual void onHide() override;

		LvContainer m_temperatureCont;
		List<ToolControl> m_tools;
		List<HeaterSlider> m_beds;
		List<HeaterSlider> m_chambers;

		FilamentSelect m_filamentSelect;

		ModalNumberPad m_numberPad;
	};
} // namespace UI
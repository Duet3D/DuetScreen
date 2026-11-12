#pragma once

#include "TemperaturePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/List/List.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Core/View.h"
#include "UI/Widgets/ExtruderControl/ExtruderControl.h"
#include "UI/Widgets/Temperature/ToolControl.h"

namespace UI
{
	class TemperatureView : public View<TemperaturePresenter>
	{
	  public:
		friend class ToolItem;
		friend class TemperaturePresenter;

		TemperatureView(const std::string& name, LvObj& parent);

		auto& getTools() { return m_tools; }
		auto& getBeds() { return m_beds; }
		auto& getChambers() { return m_chambers; }
		auto& getNumberPad() { return m_numberPad; }

		void setSelectedToolName(std::string_view tool_name);

	  private:
		void onShow() override;
		void onHide() override;

		LvContainer m_temperatureCont{"temperature_cont", getRoot()};
		List<ToolControl> m_tools{"tools", m_temperatureCont};
		List<HeaterSlider> m_beds{"beds", m_temperatureCont};
		List<HeaterSlider> m_chambers{"chambers", m_temperatureCont};

		LvContainer m_controlCont{"control_cont", getRoot()};
		LvLabel m_selectedToolLabel{"selected_tool_label", m_controlCont};
		ExtruderControl m_extruderControl{"extruder_control", m_controlCont};

		ModalNumberPad m_numberPad{"number_pad", getRoot()};
	};
} // namespace UI
#pragma once

#include "MovePresenter.h"
#include "UI/Components/AxisControl/AxisJogList.h"
#include "UI/Components/AxisControl/GenericAxisControl.h"
#include "UI/Components/AxisControl/XYControl.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/ExtruderControl/ExtruderControl.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"

namespace UI
{
	class MoveView : public View<MovePresenter>
	{
	  public:
		MoveView(LvObj& parent);

		void setAxisData(const std::vector<MovePresenter::AxisData>& axis_data);
		void setPositionType(const MovePresenter::PositionType type);
		void setCanJogUnhomed(bool enable);
		void setAxisPosition(char axis_letter, float position);
		void setAxisHomed(char axis_letter, bool homed);
		void setAxisDisabled(char axis_letter, bool disabled);
		void setAxisJogDisabled(char axis_letter, bool disabled);
		void setAxisHomeDisabled(char axis_letter, bool disabled);

		void setToolCount(const size_t count);
		void setToolName(const size_t index, const std::string& name);
		void setCurrentTool(const int32_t index);
		void setFilamentDisabled(bool disabled) { m_extruderControl.setFilamentDisabled(disabled); }
		void setFilamentOptions(const std::vector<std::string>& options)
		{
			m_extruderControl.setFilamentOptions(options);
		}
		void setLoadedFilament(const std::string& filament) { m_extruderControl.setFilamentSelected(filament); }

		void clear();

	  private:
		static void onHomeAllEvent(lv_event_t* e);
		static void onTrueBedLevelEvent(lv_event_t* e);
		static void onMeshBedLevelEvent(lv_event_t* e);
		static void onHeightmapEvent(lv_event_t* e);
		static void onDisableMotorsEvent(lv_event_t* e);
		static void onDistanceEvent(lv_event_t* e);

		virtual void onShow() override;
		virtual void onHide() override;

		const size_t getAxisCount() const { return m_axisList.getItemCount(); }
		void setAxisCount(const size_t count);
		std::shared_ptr<AxisItem> getAxisItem(size_t index);

		void configureNumberpadForAxis(char axis_letter, float position);
		void configureNumberpadForExtruder(const std::string& header, float value);

		int32_t m_layoutColDsc[2];
		int32_t m_layoutRowDsc[4];

		Card m_topBarCont;
		Card m_bottomBarCont;

		// Top Bar
		Button m_homeAll;
		Button m_trueBedLevel;
		Button m_meshBedLevel;
		Button m_heightmap;
		Button m_disableMotors;

		// Axis Control
		Card m_axisControlCont;
		XYControl m_xyControl;
		GenericAxisControl m_zControl;
		List<GenericAxisControl> m_genericAxisControls;
		AxisJogList m_axisList;

		// Extruder Control
		ExtruderControl m_extruderControl;

		// Bottom Bar
		List<Button> m_distances;

		ModalNumberPad m_numberpad;

		const MovePresenter::AxisDataList* m_axisDataListPtr = nullptr;
	};
} // namespace UI

#pragma once

#include "ControlPresenter.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Core/View.h"
#include "UI/Screens/Heightmap/HeightmapView.h"
#include "UI/Screens/Move/MoveView.h"
#include "UI/Screens/ObjectCancel/ObjectCancelView.h"
#include "UI/Screens/Temperature/TemperatureView.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"

namespace UI
{
	class ControlView : public View<ControlPresenter>
	{
	  public:
		ControlView(const std::string& name, LvObj& parent);

		void showMoveView() { m_tabs.setActiveTab(0); }
		void showTemperatureView() { m_tabs.setActiveTab(1); }
		void showHeightmapView() { m_tabs.setActiveTab(2); }
		void showObjectCancelView() { m_tabs.setActiveTab(3); }
		void showFanView() { m_tabs.setActiveTab(4); }

		MoveView& getMoveView() { return m_moveView; }
		TemperatureView& getTemperatureView() { return m_temperatureView; }
		HeightmapView& getHeightmapView() { return m_heightmapView; }
		ObjectCancelView& getObjectCancelView() { return m_objectCancelView; }
		Xml::XmlView& getFanView() { return m_fanView; }

	  private:
		TabView m_tabs{"tabs", getRoot()};
		MoveView m_moveView{"move", m_tabs.addTab(_("control.move_tab"))};
		TemperatureView m_temperatureView{"temperature", m_tabs.addTab(_("control.temperature_tab"))};
		HeightmapView m_heightmapView{"heightmap", m_tabs.addTab(_("control.heightmap_tab"))};
		ObjectCancelView m_objectCancelView{"object_cancel", m_tabs.addTab(_("control.object_cancel_tab"))};
		Xml::XmlView m_fanView{"fan", m_tabs.addTab(_("control.fan_tab")), "fan_screen"};
	};
} // namespace UI
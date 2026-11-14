/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "UI/Widgets/Buttons/DsButton.h"
#include "UI/Widgets/Buttons/DsDragButton.h"
#include "UI/Widgets/Buttons/DsPasswordToggle.h"
#include "UI/Widgets/Display/DsArc.h"
#include "UI/Widgets/Display/DsBar.h"
#include "UI/Widgets/Display/DsBarMarker.h"
#include "UI/Widgets/Display/DsGraph.h"
#include "UI/Widgets/Display/DsGraphKey.h"
#include "UI/Widgets/Display/DsHeatmap.h"
#include "UI/Widgets/Display/DsIcon.h"
#include "UI/Widgets/Display/DsPlot.h"
#include "UI/Widgets/Display/DsPlotLayer.h"
#include "UI/Widgets/Display/DsPlotRect.h"
#include "UI/Widgets/Display/DsThumbnail.h"
#include "UI/Widgets/Inputs/DsChoice.h"
#include "UI/Widgets/Inputs/DsHueSlider.h"
#include "UI/Widgets/Inputs/DsKeyboard.h"
#include "UI/Widgets/Inputs/DsSlider.h"
#include "UI/Widgets/Inputs/DsTextarea.h"
#include "UI/Widgets/Inputs/DsToggle.h"
#include "UI/Widgets/Layout/DsLayout.h"
#include "UI/Widgets/Layout/DsList.h"
#include "UI/Widgets/Layout/DsMoveHandle.h"
#include "UI/Widgets/Layout/DsScope.h"
#include "UI/Widgets/Layout/DsTabs.h"
#include "UI/Widgets/Layout/DsView.h"
#include "UI/Xml/Dialogs/DialogHost.h"

#include <map>

namespace UI::Xml::Widgets
{
	/** Specs by element name. Registrations last for the life of the program, like the engine's. */
	static std::map<std::string, WidgetSpec, std::less<>>& registry()
	{
		static std::map<std::string, WidgetSpec, std::less<>> s_specs;
		return s_specs;
	}

	void add(WidgetSpec spec, lv_xml_widget_create_cb_t create, lv_xml_widget_apply_cb_t apply)
	{
		std::string name = spec.name;
		lv_xml_register_widget(name.c_str(), create, apply);
		registry().insert_or_assign(std::move(name), std::move(spec));
	}

	std::vector<const WidgetSpec*> specs()
	{
		std::vector<const WidgetSpec*> result;
		for (const auto& [name, spec] : registry())
		{
			result.push_back(&spec);
		}
		return result;
	}

	void registerAll()
	{
		DsArc::registerXml();
		DsBar::registerXml();
		DsBarMarker::registerXml();
		DsButton::registerXml();
		DsChoice::registerXml();
		DsDragButton::registerXml();
		DialogHost::registerXml();
		DsGraph::registerXml();
		DsGraphKey::registerXml();
		DsHeatmap::registerXml();
		DsHueSlider::registerXml();
		DsIcon::registerXml();
		DsSlider::registerXml();
		DsKeyboard::registerXml();
		DsLayout::registerXml();
		DsList::registerXml();
		DsMoveHandle::registerXml();
		DsPasswordToggle::registerXml();
		DsPlot::registerXml();
		DsPlotLayer::registerXml();
		DsPlotRect::registerXml();
		DsScope::registerXml();
		DsTabs::registerXml();
		DsTextarea::registerXml();
		DsThumbnail::registerXml();
		DsToggle::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

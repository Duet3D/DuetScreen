/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsBar.h"
#include "DsBarMarker.h"
#include "DsButton.h"
#include "DsDragButton.h"
#include "DsDropdown.h"
#include "DsGraph.h"
#include "DsHeatmap.h"
#include "DsHueSlider.h"
#include "DsIcon.h"
#include "DsKeyboard.h"
#include "DsList.h"
#include "DsModal.h"
#include "DsPasswordToggle.h"
#include "DsPlot.h"
#include "DsPlotLayer.h"
#include "DsPlotRect.h"
#include "DsSlider.h"
#include "DsTabs.h"
#include "DsTextarea.h"
#include "DsThumbnail.h"
#include "DsToggle.h"
#include "DsView.h"

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
		DsBar::registerXml();
		DsBarMarker::registerXml();
		DsButton::registerXml();
		DsDragButton::registerXml();
		DsDropdown::registerXml();
		DsGraph::registerXml();
		DsHeatmap::registerXml();
		DsHueSlider::registerXml();
		DsIcon::registerXml();
		DsSlider::registerXml();
		DsKeyboard::registerXml();
		DsList::registerXml();
		DsModal::registerXml();
		DsPasswordToggle::registerXml();
		DsPlot::registerXml();
		DsPlotLayer::registerXml();
		DsPlotRect::registerXml();
		DsTabs::registerXml();
		DsTextarea::registerXml();
		DsThumbnail::registerXml();
		DsToggle::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

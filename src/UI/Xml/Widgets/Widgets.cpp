/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsBar.h"
#include "DsBarMarker.h"
#include "DsButton.h"
#include "DsDropdown.h"
#include "DsGraph.h"
#include "DsHeatmap.h"
#include "DsIcon.h"
#include "DsKeyboard.h"
#include "DsList.h"
#include "DsModal.h"
#include "DsPlot.h"
#include "DsPlotLayer.h"
#include "DsPlotRect.h"
#include "DsSlider.h"
#include "DsTabs.h"
#include "DsTextarea.h"
#include "DsThumbnail.h"
#include "DsView.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsBar::registerXml();
		DsBarMarker::registerXml();
		DsButton::registerXml();
		DsDropdown::registerXml();
		DsGraph::registerXml();
		DsHeatmap::registerXml();
		DsIcon::registerXml();
		DsSlider::registerXml();
		DsKeyboard::registerXml();
		DsList::registerXml();
		DsModal::registerXml();
		DsPlot::registerXml();
		DsPlotLayer::registerXml();
		DsPlotRect::registerXml();
		DsTabs::registerXml();
		DsTextarea::registerXml();
		DsThumbnail::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

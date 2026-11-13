/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsBar.h"
#include "DsButton.h"
#include "DsGraph.h"
#include "DsIcon.h"
#include "DsList.h"
#include "DsModal.h"
#include "DsSlider.h"
#include "DsTabs.h"
#include "DsThumbnail.h"
#include "DsView.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsBar::registerXml();
		DsButton::registerXml();
		DsGraph::registerXml();
		DsIcon::registerXml();
		DsSlider::registerXml();
		DsList::registerXml();
		DsModal::registerXml();
		DsTabs::registerXml();
		DsThumbnail::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsButton.h"
#include "DsGraph.h"
#include "DsList.h"
#include "DsModal.h"
#include "DsSlider.h"
#include "DsTabs.h"
#include "DsView.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsButton::registerXml();
		DsGraph::registerXml();
		DsSlider::registerXml();
		DsList::registerXml();
		DsModal::registerXml();
		DsTabs::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsButton.h"
#include "DsGraph.h"
#include "DsList.h"
#include "DsSlider.h"
#include "DsView.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsButton::registerXml();
		DsGraph::registerXml();
		DsSlider::registerXml();
		DsList::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

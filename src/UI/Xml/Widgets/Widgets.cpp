/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsButton.h"
#include "DsList.h"
#include "DsSlider.h"
#include "DsView.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsButton::registerXml();
		DsSlider::registerXml();
		DsList::registerXml();
		DsView::registerXml();
	}
} // namespace UI::Xml::Widgets

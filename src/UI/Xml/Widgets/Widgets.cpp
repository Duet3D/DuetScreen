/*
 * Widgets.cpp
 */

#include "Widgets.h"
#include "DsButton.h"
#include "DsList.h"
#include "DsSlider.h"

namespace UI::Xml::Widgets
{
	void registerAll()
	{
		DsButton::registerXml();
		DsSlider::registerXml();
		DsList::registerXml();
	}
} // namespace UI::Xml::Widgets

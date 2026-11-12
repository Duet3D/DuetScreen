/*
 * NavigationSource.cpp
 */

#include "NavigationSource.h"
#include "UI/Core/Navigation.h"

namespace UI::Xml
{
	NavigationSource::NavigationSource()
		: DataSource("navigation")
	{
	}

	void NavigationSource::fields(FieldVisitor& f)
	{
		f.field("nav.screen", m_screen, "Name of the open screen, or empty when the home screens are showing");
	}

	void NavigationSource::onInit()
	{
		registerEventListener<EventType::NavigationCurrentScreen>(this, &NavigationSource::screenChanged);
	}

	void NavigationSource::update()
	{
		m_screen.set(getCurrentScreenName());
	}

	void NavigationSource::screenChanged(LvObj* screen)
	{
		m_screen.set(getViewName(screen));
	}
} // namespace UI::Xml

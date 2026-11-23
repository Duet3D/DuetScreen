/*
 * ThemeSource.cpp
 */

#include "ThemeSource.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Theme.h"

namespace UI::Xml
{
	ThemeSource::ThemeSource()
		: DataSource("theme")
	{
	}

	void ThemeSource::fields(FieldVisitor& f)
	{
		f.field("primary", m_primary, "The theme's primary colour");
		f.field("secondary", m_secondary, "The theme's secondary colour");
		f.field("customisable",
				m_customisable,
				"1 if the theme's palette can be changed: between dark and light (source.settings.dark), and its hues");
	}

	void ThemeSource::onInit()
	{
		registerEventListener<EventType::ThemeChanged>(
			[this]()
			{
				UI_LOCK();
				update();
			});
	}

	void ThemeSource::update()
	{
		const Themes::Theme* theme = Themes::getCurrentTheme();
		m_customisable.set(theme != nullptr && theme->isCustomisable() ? 1 : 0);
		if (theme != nullptr)
		{
			const auto colors = theme->getColors();
			m_primary.set(colors.primary);
			m_secondary.set(colors.secondary);
		}
	}
} // namespace UI::Xml

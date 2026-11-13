/*
 * WidgetSource.cpp
 */

#include "WidgetSource.h"

namespace UI::Xml
{
	WidgetSource::WidgetSource()
		: DataSource("widget")
	{
	}

	void WidgetSource::fields(FieldVisitor& f)
	{
		m_page.fields(f);
	}
} // namespace UI::Xml

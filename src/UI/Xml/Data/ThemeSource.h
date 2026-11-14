/*
 * ThemeSource.h
 *
 * Publishes the theme in use for every view: its primary and secondary colours, e.g. for a swatch, and whether its
 * palette can be changed (a dark mode, hues). Which theme is used is a setting (source.settings.theme).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** Publishes `source.theme.*` */
	class ThemeSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ThemeSource();

		/** Declare `source.theme.*` */
		void fields(FieldVisitor& f) override;

		/** Show the theme in use */
		void update() override;

		/** The theme doesn't depend on the printer */
		void clear() override {}

	  protected:
		/** Follow the theme changing */
		void onInit() override;

	  private:
		ColorSubject m_primary;		 // `primary`
		ColorSubject m_secondary;	 // `secondary`
		IntSubject m_customisable;	 // `customisable`
	};
} // namespace UI::Xml

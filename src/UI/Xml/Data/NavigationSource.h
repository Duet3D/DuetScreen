/*
 * NavigationSource.h
 *
 * Publishes which screen is open, so navigation controls can show it, e.g.
 * `<bind_state_if_eq subject="nav.screen" state="checked" ref_value="control"/>`
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI
{
	class LvObj;
}

namespace UI::Xml
{
	class NavigationSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		NavigationSource();

		/** Declare `nav.screen` */
		void fields(FieldVisitor& f) override;

		/** Read the open screen from Navigation */
		void update() override;

		/** Navigation isn't printer data: nothing to forget */
		void clear() override {}

	  protected:
		/** Listen for screens opening and closing */
		void onInit() override;

	  private:
		/** A screen opened or closed */
		void screenChanged(LvObj* screen);

		StringSubject<32> m_screen; // Name of the open screen, or empty
	};
} // namespace UI::Xml

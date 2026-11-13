/*
 * CustomiseSource.h
 *
 * Publishes the state of customising the screen (UI/Xml/Layout/LayoutEditor.h) for every view: whether the user is
 * editing, what is selected, and what can be done with it. The edit overlays <ds_layout> puts over each widget, and
 * the toolbar (customise.xml), bind to these.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Layout/LayoutEditor.h"

namespace UI::Xml
{
	class CustomiseSource : public DataSource, public EditorListener
	{
	  public:
		/** Create the source, following the layout editor */
		CustomiseSource();

		/** Stop following the editor */
		~CustomiseSource() override;

		/** Declare `customise.*` */
		void fields(FieldVisitor& f) override;

		/** Read the editor */
		void update() override;

		/** The editor isn't the printer's: nothing to forget when it disconnects */
		void clear() override {}

		/** The editor changed: show it */
		void onEditorChanged() override { update(); }

		/** The value `customise.selected` has while item `id` of `slot` is selected */
		static std::string selectedKey(std::string_view slot, std::string_view id);

	  private:
		IntSubject m_editing;		 // 1 while customising
		StringSubject<96> m_selected; // "slot/item" of the selected item, else empty
		IntSubject m_hasItem;		 // 1 if an item is selected
		IntSubject m_canRemove;		 // 1 if it can be removed
		IntSubject m_canConfigure;	 // 1 if it has settings to change
		IntSubject m_canUndo;		 // 1 if there is a change to undo
		IntSubject m_isGrid;		 // 1 if widgets are added to a grid
		StringSubject<64> m_title;	 // The selected widget's name, translated
	};
} // namespace UI::Xml

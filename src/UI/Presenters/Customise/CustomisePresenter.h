/*
 * CustomisePresenter.h
 *
 * Behaviour for customising the screen (assets/ui/components/customise/customise.xml): the toolbar shown while the
 * user customises, and its dialogs: the widget pool, a widget's settings, the choice of one of a setting's values,
 * typing a setting's text, and the grid's size. The edits themselves are the layout editor's
 * (UI/Xml/Layout/LayoutEditor.h); <ds_layout> turns touches on the widgets into moves, resizes and selections, and the
 * edit's state is global data (`customise.*`, UI/Xml/Data/CustomiseSource.h).
 *
 * The home screen hosts customise.xml once, as `customise`. Any view starts customising with the global action
 * `customise.start` (Settings does), and opens the widget pool with `customise.open_catalogue` (the place marker in
 * a grid does).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>

namespace UI
{
	/** A widget of the pool that fits where it would be added */
	class CatalogueRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> id;			 // The widget's id
		Xml::StringSubject<64> title;		 // Its name, translated
		Xml::StringSubject<160> description; // What it shows, translated
		Xml::StringSubject<64> icon;		 // Its icon, or empty
	};

	/** One setting of the selected widget */
	class SettingRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> name;   // The setting's name (its prop)
		Xml::StringSubject<64> title;  // Its name as shown, translated
		Xml::StringSubject<16> kind;   // "int", "bool", "text", "choice" or "row"
		Xml::StringSubject<128> value; // Its value as shown, e.g. "Fan 1"
		Xml::IntSubject on;			   // bool: 1 when on
	};

	/** One of the values a choice or row setting can take */
	class OptionRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<128> value; // The value stored
		Xml::StringSubject<128> title; // As shown, translated
		Xml::IntSubject chosen;		   // 1 for the current value
	};

	class CustomisePresenter : public Xml::XmlPresenter, public Xml::EditorListener
	{
	  public:
		/** Create the presenter, with every dialog closed */
		CustomisePresenter();

		/** Stop following the editor */
		~CustomisePresenter() override;

		/** Declare the toolbar's and dialogs' fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Register `customise.start` and `customise.open_catalogue` as global actions, for every view */
		static void registerActions();

		/** The edit changed: close the dialogs when it ends; keep the toolbar off a newly selected widget */
		void onEditorChanged() override;

		/** Open the widget pool, with the widgets that fit where one would be added */
		void openCatalogue();

	  private:
		/** Fill the settings dialog with the selected widget's settings */
		void showSettings();

		/** Change setting `row`: on the number pad, a choice of values, or typed */
		void editSetting(const SettingRow& row);

		/** Set the selected widget's setting `name` to `value`, and show the settings again */
		void setSetting(const std::string& name, const nlohmann::json& value);

		/** Show the grid's size */
		void showGrid();

		/** Close every dialog */
		void closeDialogs();

		/** Open or close the dialog whose subject is `dialog`, and show whether any is open */
		void setOpen(Xml::IntSubject& dialog, bool open);

		/** Stop customising, keeping the changes; say so if they couldn't be saved */
		void finish();

		Xml::IntSubject m_atTop;						 // 1 if the toolbar is at the top of the screen
		Xml::IntSubject m_dialogOpen;					 // 1 while any of the dialogs is open
		Xml::List<CatalogueRow> m_catalogue;			 // The widgets that can be added
		Xml::IntSubject m_catalogueOpen;				 // 1 while the widget pool is open
		Xml::List<SettingRow> m_settings;				 // The selected widget's settings
		Xml::IntSubject m_settingsOpen;					 // 1 while its settings are open
		Xml::List<OptionRow> m_options;					 // The values the setting being changed can take
		Xml::IntSubject m_optionsOpen;					 // 1 while they are shown
		Xml::StringSubject<64> m_optionsTitle;			 // The setting they are for
		Xml::StringSubject<64> m_textTitle;				 // The text setting being typed
		Xml::StringSubject<128> m_textValue;			 // Its text as typed so far
		Xml::IntSubject m_textOpen;						 // 1 while it is typed
		Xml::IntSubject m_gridOpen;						 // 1 while the grid's size is shown
		Xml::StringSubject<8> m_columns;				 // The grid's columns
		Xml::StringSubject<8> m_rows;					 // The grid's rows
		std::string m_selected;							 // "slot/item" of the selection the toolbar was placed for
		std::string m_choosing;							 // The setting whose options or text are shown
	};
} // namespace UI

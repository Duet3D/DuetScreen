/*
 * CustomisePresenter.h
 *
 * Behaviour for customising the screen (assets/ui/components/customise/customise.xml): the toolbar shown while the
 * user customises, and its dialogs: the widget pool, by category, a widget's settings (text is typed in them; numbers
 * and choices are the settings' next step, a prompt), and saving the layouts under a name and loading them again
 * (UI/Xml/Layout/LayoutLibrary.h); and snapping, on or off. The edits themselves are the layout editor's
 * (UI/Xml/Layout/LayoutEditor.h); <ds_layout> turns touches on the widgets into moves, resizes and selections, and the
 * edit's state is global data (`customise.*`, UI/Xml/Data/CustomiseSource.h).
 *
 * The home screen hosts customise.xml once, as `customise`. Any view starts customising with the global action
 * `actions.customise.start` (Settings does), and opens the widget pool with `actions.customise.open_catalogue` (the
 * place marker in a free layout does); both are in CustomiseActions.cpp.
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

	/** A category of the widget pool, with its widgets */
	class CategoryRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> title;		 // The category's name, translated
		Xml::List<CatalogueRow> widgets;	 // Its widgets that fit, by name
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
		Xml::StringSubject<8> control; // What changes it: "button" (opens the number pad or values), "switch", "text"
		Xml::StringSubject<128> value; // Its value as shown, e.g. "Fan 1"
		Xml::IntSubject on;			   // bool: 1 when on
	};

	/** A layout the user saved */
	class SavedRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<40> name; // Its name
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

		/** The shared customise presenter: the view called `customise`'s; nullptr, with a warning, if there is none */
		static CustomisePresenter* shared();

		/** The edit changed: close the dialogs when it ends */
		void onEditorChanged() override;

		/** Open the widget pool, with the widgets that fit where one would be added */
		void openCatalogue();

	  private:
		/** Fill the settings dialog with the selected widget's settings */
		void showSettings();

		/** Change setting `row` on a prompt that goes with `asker` (its row): a number pad or a choice of values */
		void editSetting(const SettingRow& row, lv_obj_t* asker);

		/** Set the selected widget's setting `name` to `value`, and show the settings again */
		void setSetting(const std::string& name, const nlohmann::json& value);

		/** Close every dialog */
		void closeDialogs();

		/** Ask for a name, and save every slot's layout as it is now under it */
		void askSaveName();

		/** Save under the name typed, or say why it can't be */
		void saveAs();

		/** List the saved layouts */
		void showSaved();

		/** Show the layout saved as `name`, as one change */
		void loadSaved(const std::string& name);

		/** Tell the user `message`, e.g. why a layout couldn't be saved */
		static void warn(const std::string& message);

		/** Open or close the dialog whose subject is `dialog`, and show whether any is open */
		void setOpen(Xml::IntSubject& dialog, bool open);

		/** Stop customising, keeping the changes; say so if they couldn't be saved */
		void finish();

		Xml::IntSubject m_dialogOpen;					 // 1 while any of the dialogs is open
		Xml::List<CategoryRow> m_catalogue;				 // The widgets that can be added, by category
		Xml::IntSubject m_catalogueOpen;				 // 1 while the widget pool is open
		Xml::List<SettingRow> m_settings;				 // The selected widget's settings
		Xml::IntSubject m_settingsOpen;					 // 1 while its settings are open
		Xml::IntSubject m_canSave;						 // 1 if layouts can be saved (not in safe mode)
		Xml::IntSubject m_saveOpen;						 // 1 while asking for a name to save as
		Xml::StringSubject<40> m_saveName;				 // The name typed so far
		Xml::StringSubject<96> m_saveProblem;			 // Why the name typed can't be used; empty if it can
		Xml::IntSubject m_loadOpen;						 // 1 while the saved layouts are listed
		Xml::List<SavedRow> m_saved;					 // The saved layouts, by name
		Xml::IntSubject m_hasSaved;						 // 1 if there are any
	};
} // namespace UI

/*
 * FileBrowserPresenter.h
 *
 * Behaviour for the file browser (assets/ui/components/file_browser/file_browser.xml): browse the jobs or macros
 * folder, sort, and start a job, run a macro or delete a file after asking.
 *
 * The browser is shown in several places, each with its own folder and remembered settings, so each placement
 * registers a presenter of its own (see Presenters/Presenters.cpp) and names it: `<ds_view view="file_browser"
 * presenter="dashboard_jobs"/>`.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "ObjectModel/Files.h"
#include "Storage.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <string_view>

namespace UI
{
	/** One file or folder */
	class FileRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<128> name;	   // Shown name
		Xml::StringSubject<24> date;	   // Modification date
		Xml::StringSubject<256> thumbnail; // Thumbnail path, or empty
		Xml::IntSubject isFolder;		   // 1 for a folder
	};

	/** One folder of the open folder's path */
	class CrumbRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<128> name; // Folder name
		Xml::IntSubject last;		  // 1 for the open folder
		std::string path;			  // The folder's path below the base folder, e.g. "parts/"; not shown
	};

	class FileBrowserPresenter : public Xml::XmlPresenter
	{
	  public:
		/** The folder a browser shows */
		enum class BaseFolder
		{
			GCODES, // Jobs
			MACROS	// Macros
		};

		/** How files are shown; stored, so the values must not change */
		enum class DisplayMode
		{
			List, // One per line, with the date
			Tile  // A grid of thumbnails
		};

		using SortBy = OM::FileSystem::SortBy;

		/** What a placement shows, and where its settings are remembered */
		struct Settings
		{
			BaseFolder folder;					 // The folder browsed
			StorageKey<SortBy> sortBy;			 // Order
			StorageKey<bool> sortDescending;	 // Direction
			StorageKey<DisplayMode> displayMode; // List or tiles
		};

		/**
		 * @param name The name the presenter is registered under
		 * @param settings What it shows and where its settings are kept
		 */
		FileBrowserPresenter(std::string name, Settings settings);

		/** Declare the browser's fields */
		void fields(Xml::FieldVisitor& f) override;

		/** `name` as shown in `folder`: a macro's "12_" ordering prefix is left out */
		static std::string_view getDisplayName(std::string_view name, BaseFolder folder);

	  protected:
		/** Listen for thumbnails and printer changes; show the stored order and display mode */
		void onInit() override;

		/** Read the open folder */
		void onActivate() override;

		/** Close the dialog: whatever it asked about may have changed */
		void onDeactivate() override;

		/** Forget the files */
		void onDisconnect() override;

	  private:
		/** Open `folder`, a path below the base folder ("" for the base folder itself) */
		void openFolder(std::string_view folder);

		/** Ask the printer for the open folder's contents */
		void requestFiles();

		/** The printer sent the open folder's contents */
		void receiveFiles(OM::FileSystem::ItemList items);

		/** Sort the files and show them */
		void showFiles();

		/** Show the open folder's path */
		void showCrumbs();

		/** Show the order on the sort buttons */
		void showSort();

		/** Show files as a list or tiles, and remember it */
		void setDisplayMode(DisplayMode mode);

		/** Open a folder, or ask what to do with a file */
		void open(const FileRow& row);

		/** Ask to start the job `item` */
		void askStartPrint(const OM::FileSystem::FileSystemItem& item);

		/** Ask to run the macro `item` */
		void askRunMacro(const OM::FileSystem::FileSystemItem& item);

		/** Ask to delete the file the dialog is about */
		void askDelete();

		/** Do what the dialog asks */
		void confirm();

		/** Close the dialog */
		void closeDialog();

		/** A thumbnail arrived: show it on its file, and in the dialog if it is about that file */
		void thumbnailReceived(const std::string& path);

		/** The base folder's path, e.g. "0:/gcodes" */
		std::string_view baseFolderPath() const;

		Settings m_settings;					  // What is shown and where settings are kept
		std::string m_folder;					  // Open folder below the base folder, e.g. "parts/"; "" for the base
		OM::FileSystem::ItemList m_items;		  // The open folder's contents, in shown order
		SortBy m_sortBy = SortBy::DATE;			  // Order
		bool m_sortDescending = true;			  // Direction
		std::string m_selected;					  // Path of the file the dialog is about
		std::string m_selectedName;				  // Its name

		Xml::List<FileRow> m_files;				  // Shown files
		Xml::List<CrumbRow> m_crumbs;			  // Open folder's path
		Xml::IntSubject m_sort;					  // Order: 0 name, 1 date, 2 size
		Xml::StringSubject<32> m_sortName;		  // Name-order button text
		Xml::StringSubject<32> m_sortDate;		  // Date-order button text
		Xml::StringSubject<32> m_sortSize;		  // Size-order button text
		Xml::IntSubject m_tiles;				  // 1 when showing tiles
		Xml::StringSubject<8> m_modeIcon;		  // Symbol for the other display mode
		Xml::StringSubject<16> m_dialog;		  // What the dialog asks, or empty
		Xml::StringSubject<64> m_dialogTitle;	  // Dialog title
		Xml::StringSubject<256> m_dialogText;	  // Dialog message
		Xml::StringSubject<256> m_dialogImage;	  // Thumbnail shown in the dialog, or empty
		Xml::IntSubject m_dialogHasImage;		  // 1 if there is a thumbnail
		Xml::IntSubject m_dialogCanDelete;		  // 1 if the dialog offers to delete
		Xml::IntSubject m_infoShown;			  // 1 if the job's details are known
		Xml::StringSubject<32> m_infoDate;		  // Job modification date
		Xml::StringSubject<32> m_infoSize;		  // Job size
		Xml::StringSubject<64> m_infoGeneratedBy; // Slicer
		Xml::StringSubject<16> m_infoPrintTime;	  // Estimated print time
		Xml::StringSubject<16> m_infoHeight;	  // Object height
		Xml::StringSubject<16> m_infoLayerHeight; // Layer height
	};
} // namespace UI

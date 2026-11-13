/*
 * FileBrowserPresenter.h
 *
 * Behaviour for the file browser (assets/ui/components/file_browser.xml): browse the jobs or macros folder, sort, and
 * start a job, run a macro or delete a file after asking.
 *
 * The browser is shown in several places, each with its own folder and remembered settings, so each placement
 * registers a presenter of its own (see Screens/XmlScreens.cpp) and names it: `<ds_view view="file_browser"
 * presenter="dashboard_jobs"/>`.
 *
 * Fields
 *   files              list, one row per file and folder in the open folder, in the chosen order
 *     index            position in the list
 *     name             shown name (a macro's "12_" ordering prefix is left out)
 *     date             modification date, e.g. "2025-03-18 18:21:58"
 *     thumbnail        thumbnail image path, or empty if there is none (yet)
 *     is_folder        1 for a folder, else 0
 *   crumbs             list, the open folder's path from the base folder down, e.g. "0:/gcodes", "parts"; set when
 *                      a folder is opened or its listing arrives
 *     index            position in the path
 *     name             the folder's name
 *     last             1 for the open folder, else 0 (the others can be tapped to go back up)
 *   sort               the order: 0 by name, 1 by date, 2 by size
 *   sort_name          the name-order button's text, with an arrow showing the direction when it is in use
 *   sort_date          as sort_name, for the date order
 *   sort_size          as sort_name, for the size order
 *   tiles              1 when files are shown as tiles, 0 as a list
 *   mode_icon          the symbol for the other way of showing files
 *   dialog             what the dialog asks: "start_print", "run_macro" or "delete"; empty when it is closed
 *   dialog_title       its title
 *   dialog_text        its message
 *   dialog_image       the file's thumbnail, or empty
 *   dialog_has_image   1 if there is a thumbnail, else 0
 *   dialog_can_delete  1 if the dialog offers to delete the file (starting a job does), else 0
 *   info_shown         1 if the job's details below are known, else 0
 *   info_date, info_size, info_generated_by, info_print_time, info_height, info_layer_height   the job's details
 *
 * Actions
 *   files.open         (in a files row) open the folder, or ask to start the job or run the macro
 *   files.crumb        (in a crumbs row) open that folder
 *   files.sort         arg name, date or size: sort that way; choosing the order in use reverses it
 *   files.toggle_mode  show tiles instead of a list, or the other way round
 *   files.refresh      read the folder again
 *   files.confirm      do what the dialog asks, and close it
 *   files.ask_delete   ask to delete the file the dialog is about
 *   files.close_dialog close the dialog without doing anything
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

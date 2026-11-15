/*
 * FileBrowserPresenter.h
 *
 * Behaviour for the file browser (assets/ui/components/file_browser/file_browser.xml): browse the jobs or macros
 * folder, up and down its folders, sort it, and show it as a list or as tiles. Each file says how long its job takes
 * and the filament it uses once the printer has said, else its date and size (FilesSource::detail). Tapping a job or
 * macro opens the shared file dialog (UI/Xml/Data/FilesSource.h), which asks whether to start or run it, or delete
 * it; the folder is listed again whenever the printer's files change.
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
#include "UI/Xml/Om/OmWatch.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <string_view>

namespace UI
{
	/** One file or folder */
	class FileRow : public Xml::Row
	{
	  public:
		/** What a file's or folder's picture is, as `picture` publishes it */
		enum class Picture : int32_t
		{
			FileIcon = 0,  // A file with no thumbnail (yet)
			Thumbnail = 1, // A file's thumbnail
			FolderIcon = 2 // A folder
		};

		/** Show the thumbnail at `path` ("" for none), or a folder (`folder`) */
		void showPicture(bool folder, const std::string& path);

		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<128> name;	   // Shown name
		Xml::StringSubject<64> detail;	   // Its job's time and filament, else its date and size
		Xml::StringSubject<256> thumbnail; // Thumbnail path, or empty
		Xml::IntSubject isFolder;		   // 1 for a folder
		Xml::IntSubject picture;		   // What its picture is: a Picture
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

	  protected:
		/** Listen for thumbnails and printer changes; show the stored order and display mode */
		void onInit() override;

		/** Read the open folder */
		void onActivate() override;

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

		/** Open a folder, or ask (in the shared file dialog) whether to start the job or run the macro */
		void open(const FileRow& row);

		/** A thumbnail arrived: show it on its file */
		void thumbnailReceived(const std::string& path);

		/** A file's info arrived: show its job's time and filament */
		void fileInfoReceived(const std::string& path);

		/** The row showing the file at `path`, or nullptr if it isn't shown */
		FileRow* rowOf(const std::string& path);

		/** The base folder's path, e.g. "0:/gcodes". It reads the object model, so call it only on the UI's thread,
		 * not from a reply's callback (see m_baseFolder). */
		std::string baseFolderPath() const;

		Settings m_settings;					  // What is shown and where settings are kept
		std::string m_folder;					  // Open folder below the base folder, e.g. "parts/"; "" for the base
		std::string m_baseFolder;				  // The base folder, e.g. "0:/gcodes", noted on the UI's thread
		Xml::OmWatch m_directoriesWatch;		  // Notes the base folder as the printer's folders change
		Xml::OmWatch m_filesWatch;				  // Lists the folder again as the printer's files change
		OM::FileSystem::ItemList m_items;		  // The open folder's contents, in shown order
		SortBy m_sortBy = SortBy::DATE;			  // Order
		bool m_sortDescending = true;			  // Direction

		Xml::List<FileRow> m_files;				  // Shown files
		Xml::List<CrumbRow> m_crumbs;			  // Open folder's path
		Xml::IntSubject m_sort;					  // Order: 0 name, 1 date, 2 size
		Xml::StringSubject<32> m_sortName;		  // Name-order button text
		Xml::StringSubject<32> m_sortDate;		  // Date-order button text
		Xml::StringSubject<32> m_sortSize;		  // Size-order button text
		Xml::IntSubject m_tiles;				  // 1 when showing tiles
		Xml::IntSubject m_inSubfolder;			  // 1 while a folder below the base folder is open
		Xml::IntSubject m_empty;				  // 1 once the open folder is read and has nothing in it
	};
} // namespace UI

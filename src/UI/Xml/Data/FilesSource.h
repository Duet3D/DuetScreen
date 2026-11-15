/*
 * FilesSource.h
 *
 * Publishes the jobs and macros any view can offer: the newest jobs in the jobs folder, and the macros in the macros
 * folder. They are listed afresh on connecting, on coming back to the home screens, when a job ends, when asked
 * (actions.files.refresh), and whenever the printer counts a change to its files (seqs.volChanges; not every firmware
 * reports it). And the shared dialog that asks
 * before starting a job (with its details and thumbnail, and an offer to delete it) or running a macro, which the file
 * browser and the dashboard both open (assets/ui/components/modals/file_dialog.xml, hosted by the home screen).
 *
 * The global actions that open and answer the dialog are in Presenters/FileBrowser/FilesActions.h. The names it
 * publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "ObjectModel/Files.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Om/OmWatch.h"
#include <cstddef>
#include <string>
#include <string_view>

namespace UI::Xml
{
	/** One of the newest jobs */
	class RecentJobRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<128> name;	   // File name
		StringSubject<64> detail;	   // How long it takes and how much filament, else its date
		StringSubject<256> thumbnail;  // Thumbnail path, or empty
		std::string path;			   // The file's path, e.g. "0:/gcodes/benchy.gcode"; not shown
	};

	/** One of the macros */
	class MacroRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<128> name; // Shown name: no ordering prefix or ".g"
		std::string path;		 // The file's path, e.g. "0:/macros/Preheat PLA.g"; not shown
	};

	class FilesSource : public DataSource
	{
	  public:
		/** How many of the newest jobs are listed, at most; a view shows as many of them as it likes (ds_list limit) */
		static constexpr size_t RECENT_JOBS = 20;

		/** Create the source. Nothing is published until DataSources::start(). */
		FilesSource();

		/** Declare `source.files.*` */
		void fields(FieldVisitor& f) override;

		/** List the jobs and macros again */
		void update() override;

		/** Forget the jobs and macros, and close the dialog */
		void clear() override;

		/** `name` as shown in a list of macros (`macro`) or jobs: a macro's "12_" ordering prefix is left out */
		static std::string_view shownName(std::string_view name, bool macro);

		/** List the jobs and macros again */
		void refresh();

		/** Ask whether to start the job `item`, with its details and thumbnail */
		void askStart(const OM::FileSystem::FileSystemItem& item);

		/** Ask whether to run the macro `item` */
		void askRun(const OM::FileSystem::FileSystemItem& item);

		/** Ask whether to start the job of a row of `recent` */
		void askStart(const RecentJobRow& row);

		/** Ask whether to run the macro of a row of `macros` */
		void askRun(const MacroRow& row);

		/** Do what the dialog asks: start the job (and show the dashboard) or run the macro */
		void confirm();

		/** Ask whether to delete the job the dialog is about, as its next step; the question goes with `asker` */
		void askDelete(lv_obj_t* asker);

	  protected:
		/** Follow the printer's files and folders, and thumbnails arriving */
		void onInit() override;

	  private:
		/** Ask the printer for the jobs folder's and macros folder's files */
		void requestFiles();

		/** The printer sent the jobs folder's files: list the newest */
		void receiveJobs(OM::FileSystem::ItemList items);

		/** The printer sent the macros folder's files: list them in order */
		void receiveMacros(OM::FileSystem::ItemList items);

		/** Fill in `row` for the job `item`: its thumbnail, and what its file info says, else its date */
		static void showJob(RecentJobRow& row, const OM::FileSystem::FileSystemItem& item);

		/** A file's info arrived: show its job's time and filament */
		void fileInfoReceived(const std::string& path);

		/** A thumbnail arrived: show it on its job, and in the dialog if it is about that job */
		void thumbnailReceived(const std::string& path);

		/** Close the dialog */
		void closeDialog();

		OM::FileSystem::ItemList m_jobs;   // The newest jobs, newest first, as `recent` shows them
		OM::FileSystem::ItemList m_macros; // The macros, as `macros` shows them
		OmWatch m_watch;				   // Lists the files again as they or the folders change, or a job ends
		bool m_home = true;				   // The home screens were showing when the screen last changed
		std::string m_selected;			   // Path of the file the dialog is about
		std::string m_selectedName;		   // Its name

		List<RecentJobRow> m_recent;		 // `recent`
		IntSubject m_recentCount;			 // `recent_count`
		List<MacroRow> m_macroRows;			 // `macros`
		IntSubject m_macroCount;			 // `macro_count`
		StringSubject<16> m_dialog;			 // `dialog`: what the dialog asks, or empty
		StringSubject<64> m_dialogTitle;	 // `dialog_title`
		StringSubject<256> m_dialogText;	 // `dialog_text`
		StringSubject<256> m_dialogImage;	 // `dialog_image`: the job's thumbnail, or empty
		IntSubject m_dialogHasImage;		 // `dialog_has_image`
		IntSubject m_dialogCanDelete;		 // `dialog_can_delete`
		IntSubject m_infoShown;				 // `info_shown`
		StringSubject<32> m_infoDate;		 // `info_date`
		StringSubject<32> m_infoSize;		 // `info_size`
		StringSubject<64> m_infoGeneratedBy; // `info_generated_by`
		StringSubject<16> m_infoPrintTime;	 // `info_print_time`
		StringSubject<16> m_infoHeight;		 // `info_height`
		StringSubject<16> m_infoLayerHeight; // `info_layer_height`
	};
} // namespace UI::Xml

/*
 * FilesSource.cpp
 */

#include "FilesSource.h"
#include "Comm/Connection.h"
#include "Comm/FileInfoCache.h"
#include "Debug.h"
#include "JobSource.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <cctype>
#include <numeric>

namespace UI::Xml
{
	namespace
	{
		/** `date` as shown, to the minute: "2025-03-18T18:21:58" becomes "2025-03-18 18:21" */
		std::string shownDate(std::string date)
		{
			std::replace(date.begin(), date.end(), 'T', ' ');
			constexpr size_t TO_THE_MINUTE = 16; // "2025-03-18 18:21"
			if (date.size() > TO_THE_MINUTE)
			{
				date.resize(TO_THE_MINUTE);
			}
			return date;
		}

		/** `items` without their folders */
		OM::FileSystem::ItemList filesOf(OM::FileSystem::ItemList items)
		{
			std::erase_if(items,
						  [](const OM::FileSystem::ItemPtr& item)
						  { return item == nullptr || item->GetType() == OM::FileSystem::FileSystemItemType::folder; });
			return items;
		}
	} // namespace

	void RecentJobRow::fields(FieldVisitor& f)
	{
		f.field("name", name, "The job's file name, e.g. \"benchy.gcode\"");
		f.field("detail",
				detail,
				"How long it should take and the filament it uses, e.g. \"2 h 40 min · 12.4 m\", once the printer has "
				"said; until then, its date and size, e.g. \"2025-03-18 18:21 · 1.2 MB\"");
		f.field("thumbnail", thumbnail, "Thumbnail image path, or empty if there is none (yet)");
	}

	void MacroRow::fields(FieldVisitor& f)
	{
		f.field("name", name, "The macro's name, without its \"12_\" ordering prefix or \".g\", e.g. \"Preheat PLA\"");
	}

	FilesSource::FilesSource()
		: DataSource("files")
	{
	}

	void FilesSource::fields(FieldVisitor& f)
	{
		f.list(
			"recent",
			m_recent,
			"The jobs in the jobs folder (not in its folders), newest first: every one, so show it windowed. Tap one "
			"to ask whether to start it (actions.files.ask_start).");
		f.field("recent_count", m_recentCount, "How many jobs `recent` lists");
		f.list("macros",
			   m_macroRows,
			   "The macros in the macros folder (not in its folders), in the folder's order. Tap one to ask whether to "
			   "run it (actions.files.ask_run).");
		f.field("macro_count", m_macroCount, "How many macros `macros` lists");
		f.field("dialog",
				m_dialog,
				"What the shared file dialog asks: \"start_print\" or \"run_macro\"; empty when it is closed");
		f.field("dialog_title", m_dialogTitle, "The file dialog's title");
		f.field("dialog_text", m_dialogText, "The file dialog's message");
		f.field("dialog_image", m_dialogImage, "The job's thumbnail, shown in the dialog, or empty");
		f.field("dialog_has_image", m_dialogHasImage, "1 if the dialog shows a thumbnail, else 0");
		f.field("dialog_confirm",
				m_dialogConfirm,
				"Starting the job or running the macro (actions.files.confirm): 1 disabled (a job runs, see "
				"`dialog_reason`), 2 usable");
		f.field("dialog_delete",
				m_dialogDelete,
				"Deleting the file (actions.files.ask_delete): 0 hidden (a macro), 1 disabled (it is the job running), "
				"2 usable");
		f.field("dialog_reason",
				m_dialogReason,
				"Why the job can't be started or deleted now, e.g. \"Another job is running\"; empty if it can");
		f.field("info_shown", m_infoShown, "1 if the job's details (the info_* fields) are known, else 0");
		f.field("info_date", m_infoDate, "The job's modification date");
		f.field("info_size", m_infoSize, "The job's size");
		f.field("info_generated_by", m_infoGeneratedBy, "The slicer that made the job");
		f.field("info_print_time", m_infoPrintTime, "The job's estimated print time");
		f.field("info_height", m_infoHeight, "The object's height");
		f.field("info_layer_height", m_infoLayerHeight, "The job's layer height");
	}

	void FilesSource::onInit()
	{
		registerEventListener<EventType::ThumbnailData>(this, &FilesSource::thumbnailReceived);
		registerEventListener<EventType::FileInfo>(this, &FilesSource::fileInfoReceived);
		// A job starting or ending changes what the dialog allows
		registerEventListener<EventType::Status>([this](OM::PrinterStatus) { showAllowed(); });
		// A different printer has different files
		registerEventListener<EventType::PrinterUniqueId>([this]() { requestFiles(); });
		// Coming back to the home screens, where the files are offered, from another screen
		registerEventListener<EventType::NavigationCurrentScreen>(
			[this](LvObj* screen)
			{
				const bool home = screen == nullptr;
				if (home && !m_home)
				{
					requestFiles();
				}
				m_home = home;
			});
		// The printer counts each volume's changes to its files, where it reports them: an upload or a deletion. A job
		// ending (a new last job), or a change of folders, lists them again too
		m_watch = OmWatch({"seqs.volChanges", "directories", "job.lastFileName"}, [this] { requestFiles(); });
	}

	void FilesSource::update()
	{
		requestFiles();
	}

	void FilesSource::refresh()
	{
		requestFiles();
	}

	void FilesSource::clear()
	{
		m_jobs.clear();
		m_macros.clear();
		m_recent.clear();
		m_recentCount.set(0);
		m_macroRows.clear();
		m_macroCount.set(0);
		closeDialog();
	}

	std::string_view FilesSource::shownName(std::string_view name, bool macro)
	{
		if (!macro)
		{
			return name;
		}

		// "12_Preheat.g" is shown as "Preheat.g"
		size_t digits = 0;
		while (digits < name.size() && std::isdigit(static_cast<unsigned char>(name[digits])))
		{
			digits++;
		}
		if (digits == 0 || digits + 1 >= name.size() || name[digits] != '_')
		{
			return name;
		}
		return name.substr(digits + 1);
	}

	void FilesSource::requestFiles()
	{
		ZoneScoped;
		if (OM::GetPrinterUniqueId().empty())
		{
			return; // Not connected
		}
		// The replies arrive on the connection's thread, and may come after the source has stopped
		std::weak_ptr<DataSource> self = weak_from_this();
		OM::FileSystem::RequestFiles(OM::Directories::DirectoryType::GCODES,
									 "",
									 [self](OM::FileSystem::ItemList items)
									 {
										 if (auto source = self.lock())
										 {
											 static_cast<FilesSource&>(*source).receiveJobs(std::move(items));
										 }
									 });
		OM::FileSystem::RequestFiles(OM::Directories::DirectoryType::MACROS,
									 "",
									 [self](OM::FileSystem::ItemList items)
									 {
										 if (auto source = self.lock())
										 {
											 static_cast<FilesSource&>(*source).receiveMacros(std::move(items));
										 }
									 });
	}

	void FilesSource::receiveJobs(OM::FileSystem::ItemList items)
	{
		ZoneScoped;
		UI_LOCK();
		m_jobs = filesOf(std::move(items));
		OM::FileSystem::SortFilesBy(m_jobs, OM::FileSystem::SortBy::DATE, true);
		m_recent.sync(m_jobs.size(),
					  [this](RecentJobRow& row, size_t i)
					  {
						  const auto& item = *m_jobs[i];
						  row.path = item.GetPath();
						  row.name.set(item.GetName());
						  showJob(row, item);
					  });
		m_recentCount.set(static_cast<int32_t>(m_jobs.size()));
	}

	void FilesSource::receiveMacros(OM::FileSystem::ItemList items)
	{
		ZoneScoped;
		UI_LOCK();
		m_macros = filesOf(std::move(items));
		// The folder's order is its names': "01_", "02_" prefixes order them
		OM::FileSystem::SortFilesBy(m_macros, OM::FileSystem::SortBy::NAME, false);
		m_macroRows.sync(m_macros.size(),
						 [this](MacroRow& row, size_t i)
						 {
							 const auto& item = *m_macros[i];
							 std::string_view name = shownName(item.GetName(), true);
							 if (name.ends_with(".g"))
							 {
								 name.remove_suffix(2);
							 }
							 row.path = item.GetPath();
							 row.name.set(name);
						 });
		m_macroCount.set(static_cast<int32_t>(m_macros.size()));
	}

	std::string FilesSource::detail(const OM::FileSystem::FileSystemItem& item)
	{
		if (item.GetType() == OM::FileSystem::FileSystemItemType::folder)
		{
			return shownDate(item.GetDate());
		}
		const std::optional<Comm::FileInfo> info = Comm::FileInfoCache::get().find(item.GetPath());
		const uint32_t seconds = info ? std::max(info->printTime, info->simulatedTime) : 0;
		if (seconds == 0)
		{
			return _("file.date_and_size", shownDate(item.GetDate()), item.GetReadableSize());
		}
		const double filament = std::accumulate(info->filament.begin(), info->filament.end(), 0.0); // mm
		return filament > 0 ? _("file.time_and_filament", JobSource::duration(seconds), filament / 1000.0)
							: JobSource::duration(seconds);
	}

	void FilesSource::showJob(RecentJobRow& row, const OM::FileSystem::FileSystemItem& item)
	{
		const std::string path = item.GetPath();
		const Comm::FileInfoCache& cache = Comm::FileInfoCache::get();
		row.thumbnail.set(cache.hasThumbnail(path) ? cache.thumbnailPath(path).string() : std::string());
		row.detail.set(detail(item));
	}

	void FilesSource::askStart(const OM::FileSystem::FileSystemItem& item)
	{
		const std::string path = item.GetPath();
		m_selected = path;
		m_selectedName = item.GetName();
		Comm::FileInfoCache::get().requestThumbnail(path, Comm::FileInfoCache::ThumbnailSize::large);

		m_infoDate.set(shownDate(item.GetDate()));
		m_infoSize.set(item.GetReadableSize());
		const std::optional<Comm::FileInfo> info = Comm::FileInfoCache::get().find(path);
		m_infoShown.set(info ? 1 : 0);
		if (info)
		{
			m_infoGeneratedBy.set(info->generatedBy);
			m_infoPrintTime.set(fmt::format(
				"{:02d}:{:02d}:{:02d}", info->printTime / 3600, info->printTime / 60 % 60, info->printTime % 60));
			m_infoHeight.set(fmt::format("{:g} {:s}", info->height, Units::getDisplayedDistanceUnit()));
			m_infoLayerHeight.set(fmt::format("{:g} {:s}", info->layerHeight, Units::getDisplayedDistanceUnit()));
		}

		const bool hasImage = Comm::FileInfoCache::get().hasThumbnail(path);
		m_dialogImage.set(hasImage ? Comm::FileInfoCache::get().thumbnailPath(path).string() : std::string());
		m_dialogHasImage.set(hasImage ? 1 : 0);
		m_dialogTitle.set(_("file.start_print_title"));
		m_dialogText.set(_("file.start_print.file", item.GetName()));
		m_dialog.set("start_print");
		showAllowed();
	}

	void FilesSource::askRun(const OM::FileSystem::FileSystemItem& item)
	{
		m_selected = item.GetPath();
		m_selectedName = item.GetName();
		m_infoShown.set(0);
		m_dialogImage.set("");
		m_dialogHasImage.set(0);
		m_dialogTitle.set(_("file.run_macro_title"));
		m_dialogText.set(_("file.run_macro_message", shownName(item.GetName(), true)));
		m_dialog.set("run_macro");
		showAllowed();
	}

	void FilesSource::askStart(const RecentJobRow& row)
	{
		for (const auto& item : m_jobs)
		{
			if (item->GetPath() == row.path)
			{
				askStart(*item);
				return;
			}
		}
	}

	void FilesSource::askRun(const MacroRow& row)
	{
		for (const auto& item : m_macros)
		{
			if (item->GetPath() == row.path)
			{
				askRun(*item);
				return;
			}
		}
	}

	void FilesSource::confirm()
	{
		ZoneScoped;
		const std::string asked(m_dialog.get());
		closeDialog();
		if (asked == "start_print" && m_dialogConfirm.get() == static_cast<int32_t>(Control::Usable))
		{
			OM::FileSystem::StartPrint(m_selected);
			home();
		}
		else if (asked == "run_macro")
		{
			OM::FileSystem::RunMacro(m_selected);
		}
	}

	void FilesSource::askDelete(lv_obj_t* asker)
	{
		if (m_dialogDelete.get() != static_cast<int32_t>(Control::Usable))
		{
			return;
		}
		// The question goes with the dialog's button, and so with the dialog. Deleting changes the volume's files,
		// which lists them again everywhere (seqs.volChanges)
		Prompts::askQuestion({.title = _("file.delete_title"),
							  .text = _("file.delete_message", m_selectedName),
							  .onYes =
								  [this, path = m_selected]
							  {
								  closeDialog();
								  Comm::Connection::get().deleteFile(path);
							  },
							  .asker = asker});
	}

	void FilesSource::showAllowed()
	{
		const auto set = [](IntSubject& subject, Control control) { subject.set(static_cast<int32_t>(control)); };
		if (m_dialog.get() != "start_print")
		{
			// A macro runs whenever: during a job too, e.g. to change filament
			set(m_dialogConfirm, Control::Usable);
			set(m_dialogDelete, Control::Hidden);
			m_dialogReason.set("");
			return;
		}

		// A job can't start while another runs, nor the one running be deleted
		const bool active = OM::IsPrintingStatus(OM::GetStatus());
		bool running = false;
		{
			MODEL_LOCK();
			running = active && OM::Tree::get().root().at("job.file.fileName").text() == m_selected;
		}
		set(m_dialogConfirm, active ? Control::Disabled : Control::Usable);
		set(m_dialogDelete, running ? Control::Disabled : Control::Usable);
		m_dialogReason.set(running ? _("file.running_this") : active ? _("file.running_another") : std::string());
	}

	void FilesSource::closeDialog()
	{
		m_dialog.set("");
	}

	void FilesSource::fileInfoReceived(const std::string& path)
	{
		ZoneScoped;
		UI_LOCK();
		for (size_t i = 0; i < m_jobs.size() && i < m_recent.size(); i++)
		{
			if (m_jobs[i]->GetPath() == path)
			{
				showJob(m_recent[i], *m_jobs[i]);
				break;
			}
		}
	}

	void FilesSource::thumbnailReceived(const std::string& path)
	{
		ZoneScoped;
		UI_LOCK();
		for (size_t i = 0; i < m_jobs.size() && i < m_recent.size(); i++)
		{
			if (m_jobs[i]->GetPath() == path)
			{
				m_recent[i].thumbnail.set(Comm::FileInfoCache::get().thumbnailPath(path).string());
				break;
			}
		}

		if (m_dialog.get() == "start_print" && path == m_selected)
		{
			m_dialogImage.set(Comm::FileInfoCache::get().thumbnailPath(path).string());
			m_dialogHasImage.set(1);
		}
	}
} // namespace UI::Xml

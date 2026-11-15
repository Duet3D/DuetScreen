/*
 * FilesSource.cpp
 */

#include "FilesSource.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "JobSource.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/PrinterStatus.h"
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
		/** `date` as shown: "2025-03-18T18:21:58" becomes "2025-03-18 18:21:58" */
		std::string shownDate(std::string date)
		{
			std::replace(date.begin(), date.end(), 'T', ' ');
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
				"said; until then, its date");
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
		f.list("recent",
			   m_recent,
			   "The newest jobs in the jobs folder (not in its folders), newest first, at most five. Tap one to ask "
			   "whether to start it (actions.files.ask_start).");
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
		f.field("dialog_can_delete",
				m_dialogCanDelete,
				"1 if the dialog offers to delete the file (starting a job does), else 0");
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
		// The printer counts each volume's changes to its files: an upload or a deletion lists them again. So does a
		// different printer or a change of folders
		registerEventListener<EventType::PrinterUniqueId>([this]() { requestFiles(); });
		m_watch = OmWatch({"seqs.volChanges", "directories"}, [this] { requestFiles(); });
	}

	void FilesSource::update()
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
		if (m_jobs.size() > RECENT_JOBS)
		{
			m_jobs.resize(RECENT_JOBS);
		}
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

	void FilesSource::showJob(RecentJobRow& row, const OM::FileSystem::FileSystemItem& item)
	{
		const std::string path = item.GetPath();
		row.thumbnail.set(IsThumbnailCached(path) ? GetThumbnailPath(path).string() : std::string());

		Comm::FileInfoPtr info = FILEINFO_CACHE->GetFileInfo(path);
		const uint32_t seconds = info ? std::max(info->printTime, info->simulatedTime) : 0;
		if (seconds == 0)
		{
			row.detail.set(shownDate(item.GetDate()));
			return;
		}
		const float filament = std::accumulate(info->filament.begin(), info->filament.end(), 0.0f); // mm
		row.detail.set(filament > 0 ? _("file.time_and_filament", JobSource::duration(seconds), filament / 1000.0f)
									: JobSource::duration(seconds));
	}

	void FilesSource::askStart(const OM::FileSystem::FileSystemItem& item)
	{
		const std::string path = item.GetPath();
		m_selected = path;
		m_selectedName = item.GetName();
		FILEINFO_CACHE->QueueLargeThumbnailRequest(path);

		m_infoDate.set(shownDate(item.GetDate()));
		m_infoSize.set(item.GetReadableSize());
		Comm::FileInfoPtr info = FILEINFO_CACHE->GetFileInfo(path);
		m_infoShown.set(info ? 1 : 0);
		if (info)
		{
			const auto& printTime = info->GetPrintTime();
			m_infoGeneratedBy.set(info->generatedBy.c_str());
			m_infoPrintTime.set(
				fmt::format("{:02d}:{:02d}:{:02d}", printTime.tm_hour, printTime.tm_min, printTime.tm_sec));
			m_infoHeight.set(fmt::format("{:g} {:s}", info->height, Units::getDisplayedDistanceUnit()));
			m_infoLayerHeight.set(fmt::format("{:g} {:s}", info->layerHeight, Units::getDisplayedDistanceUnit()));
		}

		const bool hasImage = IsThumbnailCached(path);
		m_dialogImage.set(hasImage ? GetThumbnailPath(path).string() : std::string());
		m_dialogHasImage.set(hasImage ? 1 : 0);
		m_dialogTitle.set(_("file.start_print_title"));
		m_dialogText.set(_("file.start_print.file", item.GetName()));
		m_dialogCanDelete.set(1);
		m_dialog.set("start_print");
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
		m_dialogCanDelete.set(0);
		m_dialog.set("run_macro");
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
		if (asked == "start_print")
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
		// The question goes with the dialog's button, and so with the dialog. Deleting changes the volume's files,
		// which lists them again everywhere (seqs.volChanges)
		Prompts::askQuestion({.title = _("file.delete_title"),
							  .text = _("file.delete_message", m_selectedName),
							  .onYes =
								  [this, path = m_selected]
							  {
								  closeDialog();
								  if (!Comm::DUET.DeleteFile(path))
								  {
									  LOG_WARN("Failed to delete file {:s}", path);
								  }
							  },
							  .asker = asker});
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
				m_recent[i].thumbnail.set(GetThumbnailPath(path).string());
				break;
			}
		}

		if (m_dialog.get() == "start_print" && path == m_selected)
		{
			m_dialogImage.set(GetThumbnailPath(path).string());
			m_dialogHasImage.set(1);
		}
	}
} // namespace UI::Xml

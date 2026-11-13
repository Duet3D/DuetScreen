/*
 * FileBrowserPresenter.cpp
 */

#include "FileBrowserPresenter.h"
#include "Comm/FileInfo.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <cctype>

namespace UI
{
	namespace
	{
		/** The directory type of `folder` */
		OM::Directories::DirectoryType directoryOf(FileBrowserPresenter::BaseFolder folder)
		{
			return folder == FileBrowserPresenter::BaseFolder::MACROS ? OM::Directories::DirectoryType::MACROS
																	  : OM::Directories::DirectoryType::GCODES;
		}

		/** `date` as shown: "2025-03-18T18:21:58" becomes "2025-03-18 18:21:58" */
		std::string shownDate(std::string date)
		{
			std::replace(date.begin(), date.end(), 'T', ' ');
			return date;
		}

		/** A sort button's text: its name, with an arrow for the direction if it is the order in use */
		std::string sortText(const char* key, bool inUse, bool descending)
		{
			if (!inUse)
			{
				return _(key);
			}
			return fmt::format("{:s} {:s}", descending ? LV_SYMBOL_DOWN : LV_SYMBOL_UP, _(key));
		}
	} // namespace

	void FileRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "Shown name");
		f.field("date", date, "Modification date");
		f.field("thumbnail", thumbnail, "Thumbnail path, or empty");
		f.field("is_folder", isFolder, "1 for a folder");
	}

	void CrumbRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "Folder name");
		f.field("last", last, "1 for the open folder");
	}

	FileBrowserPresenter::FileBrowserPresenter(std::string name, Settings settings)
		: XmlPresenter(std::move(name))
		, m_settings(std::move(settings))
	{
		action("files.open",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<FileRow>())
				   {
					   open(*row);
				   }
			   });
		action("files.crumb",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<CrumbRow>())
				   {
					   openFolder(row->path);
				   }
			   });
		action("files.sort",
			   [this](const Xml::ActionArgs& args)
			   {
				   SortBy by = SortBy::NAME;
				   if (args.arg == "date")
				   {
					   by = SortBy::DATE;
				   }
				   else if (args.arg == "size")
				   {
					   by = SortBy::SIZE;
				   }
				   m_sortDescending = by == m_sortBy ? !m_sortDescending : m_sortDescending;
				   m_sortBy = by;
				   StorageHelper::setData(m_settings.sortBy, m_sortBy);
				   StorageHelper::setData(m_settings.sortDescending, m_sortDescending);
				   showFiles();
			   });
		action("files.toggle_mode",
			   [this](const Xml::ActionArgs&)
			   { setDisplayMode(m_tiles.get() != 0 ? DisplayMode::List : DisplayMode::Tile); });
		action("files.refresh",
			   [this](const Xml::ActionArgs&)
			   {
				   closeDialog();
				   requestFiles();
			   });
		action("files.confirm", [this](const Xml::ActionArgs&) { confirm(); });
		action("files.ask_delete", [this](const Xml::ActionArgs&) { askDelete(); });
		action("files.close_dialog", [this](const Xml::ActionArgs&) { closeDialog(); });
	}

	void FileBrowserPresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("files", m_files, "Files and folders in the open folder");
		f.list("crumbs", m_crumbs, "The open folder's path");
		f.field("sort", m_sort, "Order: 0 by name, 1 by date, 2 by size");
		f.field("sort_name", m_sortName, "Name-order button text");
		f.field("sort_date", m_sortDate, "Date-order button text");
		f.field("sort_size", m_sortSize, "Size-order button text");
		f.field("tiles", m_tiles, "1 when files are shown as tiles");
		f.field("mode_icon", m_modeIcon, "Symbol for the other way of showing files");
		f.field("dialog", m_dialog, "What the dialog asks, or empty");
		f.field("dialog_title", m_dialogTitle, "Dialog title");
		f.field("dialog_text", m_dialogText, "Dialog message");
		f.field("dialog_image", m_dialogImage, "Thumbnail shown in the dialog, or empty");
		f.field("dialog_has_image", m_dialogHasImage, "1 if the dialog shows a thumbnail");
		f.field("dialog_can_delete", m_dialogCanDelete, "1 if the dialog offers to delete the file");
		f.field("info_shown", m_infoShown, "1 if the job's details are known");
		f.field("info_date", m_infoDate, "Job modification date");
		f.field("info_size", m_infoSize, "Job size");
		f.field("info_generated_by", m_infoGeneratedBy, "Slicer");
		f.field("info_print_time", m_infoPrintTime, "Estimated print time");
		f.field("info_height", m_infoHeight, "Object height");
		f.field("info_layer_height", m_infoLayerHeight, "Layer height");
	}

	std::string_view FileBrowserPresenter::getDisplayName(std::string_view name, BaseFolder folder)
	{
		if (folder != BaseFolder::MACROS)
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

	void FileBrowserPresenter::onInit()
	{
		registerEventListener<EventType::ThumbnailData>(this, &FileBrowserPresenter::thumbnailReceived);
		// A different printer has different folders
		registerEventListener<EventType::PrinterUniqueId>([this]() { openFolder(""); });

		m_sortBy = StorageHelper::getData(m_settings.sortBy);
		m_sortDescending = StorageHelper::getData(m_settings.sortDescending);
		showSort();
		setDisplayMode(StorageHelper::getData(m_settings.displayMode));
	}

	void FileBrowserPresenter::onActivate()
	{
		if (!OM::GetPrinterUniqueId().empty())
		{
			requestFiles();
		}
	}

	void FileBrowserPresenter::onDeactivate()
	{
		closeDialog();
	}

	void FileBrowserPresenter::onDisconnect()
	{
		UI_LOCK();
		m_items.clear();
		m_files.clear();
	}

	void FileBrowserPresenter::openFolder(std::string_view folder)
	{
		m_folder = folder;
		if (!m_folder.empty() && m_folder.back() != '/')
		{
			m_folder += '/';
		}
		showCrumbs();
		requestFiles();
	}

	void FileBrowserPresenter::requestFiles()
	{
		ZoneScoped;
		// The reply may come after the browser has gone
		std::weak_ptr<XmlPresenter> self = weak_from_this();
		OM::FileSystem::RequestFiles(
			directoryOf(m_settings.folder),
			m_folder,
			[self](OM::FileSystem::ItemList items)
			{
				if (auto presenter = self.lock())
				{
					static_cast<FileBrowserPresenter&>(*presenter).receiveFiles(std::move(items));
				}
			},
			true);
	}

	void FileBrowserPresenter::receiveFiles(OM::FileSystem::ItemList items)
	{
		ZoneScoped;
		UI_LOCK();
		std::erase(items, nullptr);
		m_items = std::move(items);
		showCrumbs(); // The path of the listing shown
		showFiles();
	}

	void FileBrowserPresenter::showFiles()
	{
		ZoneScoped;
		UI_LOCK();
		OM::FileSystem::SortFilesBy(m_items, m_sortBy, m_sortDescending);
		showSort();
		m_files.sync(m_items.size(),
					 [this](FileRow& row, size_t i)
					 {
						 const auto& item = *m_items[i];
						 const bool isFolder = item.GetType() == OM::FileSystem::FileSystemItemType::folder;
						 row.name.set(getDisplayName(item.GetName(), m_settings.folder));
						 row.date.set(shownDate(item.GetDate()));
						 row.isFolder.set(isFolder ? 1 : 0);
						 row.thumbnail.set(!isFolder && IsThumbnailCached(item.GetPath())
											   ? GetThumbnailPath(item.GetPath()).string()
											   : std::string());
					 });
	}

	void FileBrowserPresenter::showCrumbs()
	{
		// The base folder, e.g. "0:/gcodes", then each folder below it
		std::string_view base = baseFolderPath();
		std::vector<std::pair<std::string, std::string>> crumbs; // Name and path below the base folder
		if (!base.empty())
		{
			crumbs.emplace_back(base, "");
			std::string path;
			size_t start = 0;
			while (start < m_folder.size())
			{
				const size_t end = std::min(m_folder.find('/', start), m_folder.size());
				if (end > start)
				{
					path += m_folder.substr(start, end - start) + '/';
					crumbs.emplace_back(m_folder.substr(start, end - start), path);
				}
				start = end + 1;
			}
		}

		m_crumbs.sync(crumbs.size(),
					  [&](CrumbRow& row, size_t i)
					  {
						  row.name.set(crumbs[i].first);
						  row.path = crumbs[i].second;
						  row.last.set(i + 1 == crumbs.size() ? 1 : 0);
					  });
	}

	void FileBrowserPresenter::showSort()
	{
		m_sort.set(static_cast<int32_t>(m_sortBy));
		m_sortName.set(sortText("file.sort_by_name", m_sortBy == SortBy::NAME, m_sortDescending));
		m_sortDate.set(sortText("file.sort_by_date", m_sortBy == SortBy::DATE, m_sortDescending));
		m_sortSize.set(sortText("file.sort_by_size", m_sortBy == SortBy::SIZE, m_sortDescending));
	}

	void FileBrowserPresenter::setDisplayMode(DisplayMode mode)
	{
		StorageHelper::setData(m_settings.displayMode, mode);
		m_tiles.set(mode == DisplayMode::Tile ? 1 : 0);
		m_modeIcon.set(mode == DisplayMode::Tile ? LV_SYMBOL_LIST : LV_SYMBOL_IMAGE);
	}

	void FileBrowserPresenter::open(const FileRow& row)
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto index = static_cast<size_t>(row.index.get());
		if (index >= m_items.size())
		{
			return;
		}
		const auto& item = *m_items[index];
		if (item.GetType() == OM::FileSystem::FileSystemItemType::folder)
		{
			closeDialog();
			// The folder's path below the base folder
			std::string path = item.GetPath();
			const std::string_view base = OM::Directories::GetDirectory(directoryOf(m_settings.folder));
			openFolder(path.starts_with(base) ? path.substr(base.size()) : path);
			return;
		}

		m_selected = item.GetPath();
		m_selectedName = item.GetName();
		if (m_settings.folder == BaseFolder::MACROS)
		{
			askRunMacro(item);
		}
		else
		{
			askStartPrint(item);
		}
	}

	void FileBrowserPresenter::askStartPrint(const OM::FileSystem::FileSystemItem& item)
	{
		const std::string path = item.GetPath();
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

	void FileBrowserPresenter::askRunMacro(const OM::FileSystem::FileSystemItem& item)
	{
		m_infoShown.set(0);
		m_dialogImage.set("");
		m_dialogHasImage.set(0);
		m_dialogTitle.set(_("file.run_macro_title"));
		m_dialogText.set(_("file.run_macro_message", getDisplayName(item.GetName(), m_settings.folder)));
		m_dialogCanDelete.set(0);
		m_dialog.set("run_macro");
	}

	void FileBrowserPresenter::askDelete()
	{
		m_infoShown.set(0);
		m_dialogImage.set("");
		m_dialogHasImage.set(0);
		m_dialogTitle.set(_("file.delete_title"));
		m_dialogText.set(_("file.delete_message", m_selectedName));
		m_dialogCanDelete.set(0);
		m_dialog.set("delete");
	}

	void FileBrowserPresenter::confirm()
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
		else if (asked == "delete")
		{
			if (Comm::DUET.DeleteFile(m_selected))
			{
				requestFiles();
			}
			else
			{
				LOG_WARN("Failed to delete file {:s}", m_selected);
			}
		}
	}

	void FileBrowserPresenter::closeDialog()
	{
		m_dialog.set("");
	}

	void FileBrowserPresenter::thumbnailReceived(const std::string& path)
	{
		ZoneScoped;
		UI_LOCK();
		for (size_t i = 0; i < m_items.size() && i < m_files.size(); i++)
		{
			if (m_items[i]->GetPath() == path)
			{
				m_files[i].thumbnail.set(GetThumbnailPath(path).string());
				break;
			}
		}

		if (m_dialog.get() == "start_print" && path == m_selected)
		{
			m_dialogImage.set(GetThumbnailPath(path).string());
			m_dialogHasImage.set(1);
		}
	}

	std::string_view FileBrowserPresenter::baseFolderPath() const
	{
		std::string_view path = OM::Directories::GetDirectory(directoryOf(m_settings.folder));
		if (!path.empty() && path.back() == '/')
		{
			path.remove_suffix(1);
		}
		return path;
	}
} // namespace UI

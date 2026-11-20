/*
 * FileBrowserPresenter.cpp
 */

#include "FileBrowserPresenter.h"
#include "Comm/FileInfoCache.h"
#include "Debug.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/FilesSource.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <algorithm>

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
		f.field("name", name, "Shown name; a macro's \"12_\" ordering prefix is left out");
		f.field("detail",
				detail,
				"Under its name: its job's time and filament once the printer has said, e.g. \"2 h 40 min · 12.4 m\"; "
				"else its date and size, e.g. \"2025-03-18 18:21 · 1.2 MB\"; a folder's date");
		f.field("thumbnail", thumbnail, "Thumbnail image path, or empty if there is none (yet)");
		f.field("is_folder", isFolder, "1 for a folder, else 0");
		f.field(
			"picture", picture, "What its picture is: 0 a file icon (no thumbnail yet), 1 its thumbnail, 2 a folder");
	}

	void FileRow::showPicture(bool folder, const std::string& path)
	{
		thumbnail.set(folder ? std::string() : path);
		picture.set(static_cast<int32_t>(folder			? Picture::FolderIcon
										 : path.empty() ? Picture::FileIcon
														: Picture::Thumbnail));
	}

	void CrumbRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "The folder's name");
		f.field("last", last, "1 for the open folder, else 0 (the others can be tapped to go back up)");
	}

	FileBrowserPresenter::FileBrowserPresenter(std::string name, Settings settings)
		: XmlPresenter(std::move(name))
		, m_settings(std::move(settings))
	{
		action("files.open",
			   "Open the folder, or ask to start the job or run the macro. From a row of `files`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<FileRow>())
				   {
					   open(*row);
				   }
			   });
		action("files.crumb",
			   "Open that folder. From a row of `crumbs`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<CrumbRow>())
				   {
					   openFolder(row->path);
				   }
			   });
		action("files.sort",
			   "Sort by `arg`: `name`, `date` or `size`; choosing the order in use reverses it. Remembered.",
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
		action("files.up",
			   "Open the folder above the open one",
			   [this](const Xml::ActionArgs&)
			   {
				   if (m_folder.empty())
				   {
					   return;
				   }
				   // "parts/brackets/" goes up to "parts/"
				   const size_t slash = m_folder.find_last_of('/', m_folder.size() - 2);
				   const std::string above = slash == std::string::npos ? std::string() : m_folder.substr(0, slash + 1);
				   openFolder(above);
			   });
		action("files.mode",
			   "Show the files as a `list` or as `tiles`, as `arg` says. Remembered.",
			   [this](const Xml::ActionArgs& args)
			   { setDisplayMode(args.arg == "tiles" ? DisplayMode::Tile : DisplayMode::List); });
		action("files.refresh", "Read the folder again", [this](const Xml::ActionArgs&) { requestFiles(); });
	}

	void FileBrowserPresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("files", m_files, "One row per file and folder in the open folder, in the chosen order");
		f.list("crumbs", m_crumbs, "The open folder's path from the base folder down, e.g. \"0:/gcodes\", \"parts\"");
		f.field("sort", m_sort, "Order: 0 by name, 1 by date, 2 by size");
		f.field("sort_name",
				m_sortName,
				"The name-order button's text, with an arrow showing the direction when it is in use");
		f.field("sort_date", m_sortDate, "As `sort_name`, for the date order");
		f.field("sort_size", m_sortSize, "As `sort_name`, for the size order");
		f.field("tiles", m_tiles, "1 when files are shown as tiles, 0 as a list");
		f.field(
			"in_subfolder", m_inSubfolder, "1 while a folder below the base folder is open (so there is one above)");
		f.field("empty", m_empty, "1 once the open folder has been read and has nothing in it, else 0");
	}

	void FileBrowserPresenter::onInit()
	{
		registerEventListener<EventType::ThumbnailData>(this, &FileBrowserPresenter::thumbnailReceived);
		registerEventListener<EventType::FileInfo>(this, &FileBrowserPresenter::fileInfoReceived);
		// A different printer has different folders
		registerEventListener<EventType::PrinterUniqueId>([this]() { openFolder(""); });
		// The base folder can arrive after the browser opens, and its files can't be asked for until then; it is noted
		// here, on the UI's thread, and an open browser lists the folder again
		m_directoriesWatch = Xml::OmWatch({"directories"},
										  [this]
										  {
											  if (isActive())
											  {
												  requestFiles(); // notes the base folder too
											  }
											  else
											  {
												  m_baseFolder = baseFolderPath();
												  showCrumbs();
											  }
										  });
		// The printer counts each volume's changes to its files: an upload or a deletion lists the folder again
		m_filesWatch = Xml::OmWatch({"seqs.volChanges"},
									[this]
									{
										if (isActive())
										{
											requestFiles();
										}
									});

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

	void FileBrowserPresenter::onDisconnect()
	{
		UI_LOCK();
		m_items.clear();
		m_files.clear();
		m_empty.set(0);
	}

	void FileBrowserPresenter::openFolder(std::string_view folder)
	{
		m_folder = folder;
		if (!m_folder.empty() && m_folder.back() != '/')
		{
			m_folder += '/';
		}
		m_inSubfolder.set(m_folder.empty() ? 0 : 1);
		m_baseFolder = baseFolderPath();
		showCrumbs();
		requestFiles();
	}

	void FileBrowserPresenter::requestFiles()
	{
		ZoneScoped;
		// The listing arrives on the connection's thread, where the object model can't be read under the UI lock:
		// note the base folder now, for its path, which shows while the folder is listed
		m_baseFolder = baseFolderPath();
		showCrumbs();
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
		m_empty.set(m_items.empty() ? 1 : 0);
		showCrumbs(); // The path of the listing shown
		showFiles();
	}

	void FileBrowserPresenter::showFiles()
	{
		ZoneScoped;
		UI_LOCK();
		OM::FileSystem::SortFilesBy(m_items, m_sortBy, m_sortDescending);
		showSort();
		m_files.sync(
			m_items.size(),
			[this](FileRow& row, size_t i)
			{
				const auto& item = *m_items[i];
				const bool isFolder = item.GetType() == OM::FileSystem::FileSystemItemType::folder;
				row.name.set(Xml::FilesSource::shownName(item.GetName(), m_settings.folder == BaseFolder::MACROS));
				row.detail.set(Xml::FilesSource::detail(item));
				row.isFolder.set(isFolder ? 1 : 0);
				row.showPicture(
					isFolder,
					isFolder ? std::string()
							 : Xml::FilesSource::thumbnail(item.GetPath(), Comm::FileInfoCache::ThumbnailSize::large));
			});
	}

	void FileBrowserPresenter::showCrumbs()
	{
		// The base folder, named for what it holds ("Jobs", not "0:/gcodes"), then each folder below it
		std::vector<std::pair<std::string, std::string>> crumbs; // Name and path below the base folder
		if (!m_baseFolder.empty())
		{
			crumbs.emplace_back(_(m_settings.folder == BaseFolder::MACROS ? "file.macros" : "file.jobs"), "");
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
			// The folder's path below the base folder
			std::string path = item.GetPath();
			const std::string base = OM::Directories::GetDirectory(directoryOf(m_settings.folder));
			openFolder(path.starts_with(base) ? path.substr(base.size()) : path);
			return;
		}

		auto* files = Xml::DataSources::find<Xml::FilesSource>();
		if (files == nullptr)
		{
			LOG_WARN("The files source isn't running");
			return;
		}
		if (m_settings.folder == BaseFolder::MACROS)
		{
			files->askRun(item);
		}
		else
		{
			files->askStart(item);
		}
	}

	void FileBrowserPresenter::thumbnailReceived(const std::string& path)
	{
		ZoneScoped;
		UI_LOCK();
		if (FileRow* row = rowOf(path))
		{
			row->showPicture(false, Xml::FilesSource::thumbnail(path, Comm::FileInfoCache::ThumbnailSize::large));
		}
	}

	void FileBrowserPresenter::fileInfoReceived(const std::string& path)
	{
		ZoneScoped;
		UI_LOCK();
		if (FileRow* row = rowOf(path))
		{
			row->detail.set(Xml::FilesSource::detail(*m_items[static_cast<size_t>(row->index.get())]));
		}
	}

	FileRow* FileBrowserPresenter::rowOf(const std::string& path)
	{
		for (size_t i = 0; i < m_items.size() && i < m_files.size(); i++)
		{
			if (m_items[i]->GetPath() == path)
			{
				return &m_files[i];
			}
		}
		return nullptr;
	}

	std::string FileBrowserPresenter::baseFolderPath() const
	{
		std::string path = OM::Directories::GetDirectory(directoryOf(m_settings.folder));
		if (!path.empty() && path.back() == '/')
		{
			path.pop_back();
		}
		return path;
	}
} // namespace UI

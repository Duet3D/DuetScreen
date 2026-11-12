#include "FilePresenter.h"
#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "FileView.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Directories.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Dashboard/DashboardTabs.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <cctype>

namespace UI
{
	static std::string s_emptyStr = "";

	static std::string_view stripMacroDisplayPrefix(std::string_view name)
	{
		if (name.empty() || !std::isdigit(static_cast<unsigned char>(name.front())))
		{
			return name;
		}

		size_t prefixEnd = 0;
		while (prefixEnd < name.size() && std::isdigit(static_cast<unsigned char>(name[prefixEnd])))
		{
			prefixEnd++;
		}

		if (prefixEnd == 0 || prefixEnd >= name.size() || name[prefixEnd] != '_')
		{
			return name;
		}

		return name.substr(prefixEnd + 1);
	}

	static OM::Directories::DirectoryType getBaseFolderType(FilePresenter::BaseFolder folder)
	{
		ZoneScoped;
		switch (folder)
		{
		case FilePresenter::BaseFolder::GCODES:
			return OM::Directories::DirectoryType::GCODES;
		case FilePresenter::BaseFolder::MACROS:
			return OM::Directories::DirectoryType::MACROS;
		default:
			LOG_ERROR("Invalid base folder type");
			return OM::Directories::DirectoryType::GCODES;
		}
	}

	std::string_view FilePresenter::getDisplayName(std::string_view name, BaseFolder baseFolder)
	{
		if (baseFolder != BaseFolder::MACROS)
		{
			return name;
		}

		return stripMacroDisplayPrefix(name);
	}

	std::string_view FilePresenter::getBaseFolderPath() const
	{
		ZoneScoped;
		std::string_view baseFolder;
		switch (m_baseFolder)
		{
		case BaseFolder::GCODES:
			baseFolder = OM::Directories::GetGcodesDirectory();
			break;
		case BaseFolder::MACROS:
			baseFolder = OM::Directories::GetMacrosDirectory();
			break;
		default:
			baseFolder = "";
		}

		if (baseFolder.back() == '/')
		{
			return baseFolder.substr(0, baseFolder.size() - 1);
		}
		return baseFolder;
	}

	void FilePresenter::setFolder(std::string_view folder)
	{
		ZoneScoped;
		if (!folder.empty() && folder.starts_with(getBaseFolderPath()))
		{
			m_currentFolder = folder.substr(getBaseFolderPath().length());
		}
		else
		{
			m_currentFolder = folder;
		}
		if (!m_currentFolder.empty() && m_currentFolder.back() != '/')
		{
			m_currentFolder += '/';
		}

		LOG_DBG("set folder to '{:s}'", m_currentFolder);
		m_view->setFolder(fmt::format("{}{}", getBaseFolderPath(), m_currentFolder));
		requestFiles();
	}

	void FilePresenter::itemClicked(const size_t index)
	{
		ZoneScoped;
		MODEL_LOCK();
		if (index >= m_items.size())
		{
			LOG_ERROR("item {:d} out of range", index);
			return;
		}

		auto item = m_items[index];

		m_view->cancelStartPrint();
		if (item == nullptr)
		{
			LOG_WARN("item {:d} is null", index);
			return;
		}

		if (item->GetType() == OM::FileSystem::FileSystemItemType::folder)
		{
			setFolder(item->GetPath());
			return;
		}

		m_clickedItem = item->GetPath();

		// File
		m_gcodePath = item->GetPath();
		if (m_gcodePath.starts_with(OM::Directories::GetGcodesDirectory()))
		{
			Comm::FileInfoPtr fileInfo = FILEINFO_CACHE->GetFileInfo(item->GetPath());
			FILEINFO_CACHE->QueueLargeThumbnailRequest(item->GetPath());
			std::string date = item->GetDate();
			std::replace(date.begin(), date.end(), 'T', ' ');

			auto& modal = getView()->getConfirmModal();
			modal.setFile(item->GetName());
			modal.setFileDate(date);
			modal.setFileSize(item->GetReadableSize());
			if (fileInfo)
			{
				modal.getFileInfo().setVisible(true);
				modal.setGeneratedBy(fileInfo->generatedBy.c_str());
				const auto& printTime = fileInfo->GetPrintTime();
				modal.setPrintTime(
					fmt::format("{:02d}:{:02d}:{:02d}", printTime.tm_hour, printTime.tm_min, printTime.tm_sec));
				modal.setHeight(fileInfo->height);
				modal.setLayerHeight(fileInfo->layerHeight);
			}
			else
			{
				modal.getFileInfo().setVisible(false);
			}
			m_view->confirmStartPrint(item->GetName(), GetThumbnailPath(item->GetPath()));
		}
		else if (m_gcodePath.starts_with(OM::Directories::GetMacrosDirectory()))
		{
			m_view->confirmRunMacro(getDisplayName(item->GetName(), m_baseFolder));
		}
	}

	void FilePresenter::deleteSelectedItem()
	{
		ZoneScoped;
		if (m_gcodePath.empty())
		{
			LOG_WARN("No file selected for deletion");
			return;
		}

		if (!Comm::DUET.DeleteFile(m_gcodePath))
		{
			LOG_WARN("Failed to delete file {:s}", m_gcodePath);
			return;
		}

		requestFiles();
	}

	void FilePresenter::startPrint()
	{
		ZoneScoped;
		OM::FileSystem::StartPrint(m_gcodePath);
		if (auto* dashboard = findView<DashboardTabs>("dashboard_tabs"))
		{
			dashboard->disableJobsTab(true);
		}
		home();
	}

	void FilePresenter::runMacro()
	{
		ZoneScoped;
		OM::FileSystem::RunMacro(m_gcodePath);
	}

	void FilePresenter::displayFiles()
	{
		ZoneScoped;
		m_view->setFileCount(m_items.size());
		m_view->showSort(m_sortBy, m_sortOrder);

		auto item_count = m_items.size();
		for (size_t i = 0; i < item_count; i++)
		{
			auto item = m_view->getFileItem(i);
			if (!item)
			{
				continue;
			}
			const auto& file = m_items.at(i);
			if (file == nullptr)
			{
				continue;
			}

			item->m_filename = getDisplayName(file->GetName(), m_baseFolder);
			item->m_date = file->GetDate();
#if SHOW_FILE_ITEM_SIZE
			item->m_size = file->GetReadableSize();
#endif
			item->m_isFolder = file->GetType() == OM::FileSystem::FileSystemItemType::folder;

			// Set thumbnail
			if (file->GetType() == OM::FileSystem::FileSystemItemType::file && IsThumbnailCached(file->GetPath()))
			{
				item->m_thumbnail = GetThumbnailPath(file->GetPath());
			}
			else
			{
				item->m_thumbnail = "";
			}
		}
		getView()->getList().refresh();
	}

	void FilePresenter::requestFiles()
	{
		ZoneScoped;
		LOG_DBG("Requesting files for folder {:s}{:s}", getBaseFolderPath(), m_currentFolder);
		UI_LOCK();
		OM::FileSystem::RequestFiles(
			getBaseFolderType(m_baseFolder),
			m_currentFolder,
			[this](OM::FileSystem::ItemList files)
			{
				ZoneScoped;
				LOG_DBG("Received {:d} files for folder {:s}{:s}",
						files.size(),
						getBaseFolderPath(),
						this->m_currentFolder);
				UI_LOCK();
				m_items = std::move(files);
				this->m_view->setFolder(fmt::format("{}{}", getBaseFolderPath(), this->m_currentFolder));
				this->sortFiles();
				this->displayFiles();
			},
			true);
	}

	void FilePresenter::refreshFiles()
	{
		ZoneScoped;
		requestFiles();
	}

	void FilePresenter::sortFiles()
	{
		ZoneScoped;
		UI_LOCK();
		OM::FileSystem::SortFilesBy(m_items, m_sortBy, m_sortOrder);
	}

	void FilePresenter::setSort(SortBy by, bool descending)
	{
		ZoneScoped;
		m_sortBy = by;
		m_sortOrder = descending;
		StorageHelper::setData(m_view->getStorageKeys().sortBy, by);
		StorageHelper::setData(m_view->getStorageKeys().sortDescending, descending);
		sortFiles();
		displayFiles();
	}

	bool FilePresenter::back()
	{
		ZoneScoped;
		if (m_currentFolder == DEFAULT_GCODES_PATH)
		{
			return false; // Already at the root folder
		}
		if (m_currentFolder == DEFAULT_MACROS_PATH)
		{
			return false; // Already at the root folder
		}

		if (m_currentFolder.empty())
		{
			return false;
		}

		size_t pos = m_currentFolder.substr(0, m_currentFolder.size() - 1).find_last_of('/');
		if (pos != std::string::npos)
		{
			setFolder(m_currentFolder.substr(0, pos));
			return true;
		}
		else
		{
			setFolder("");
			return true;
		}
		return false;
	}

	void FilePresenter::newThumbnailData(const std::string& filename)
	{
		ZoneScoped;
		UI_LOCK();
		for (size_t i = 0; i < this->m_view->getFileCount(); i++)
		{
			if (i >= m_items.size())
			{
				LOG_WARN("File count mismatch");
				break;
			}
			auto file = m_items.at(i);
			if (file == nullptr)
			{
				continue;
			}
			if (file->GetPath() == filename)
			{
				auto item = this->m_view->getFileItem(i);
				if (item)
				{
					item->m_thumbnail = GetThumbnailPath(filename.c_str());
				}
				break;
			}
		}
		getView()->getList().refresh();

		/* Update the start print modal */
		auto& modal = getView()->getConfirmModal();
		if (modal.isVisible() && m_clickedItem == filename)
		{
			modal.setImage(GetThumbnailPath(filename).c_str());
		}
	}

	void FilePresenter::onInit()
	{
		ZoneScoped;
		registerEventListener<EventType::ThumbnailData>(this, &FilePresenter::newThumbnailData);
		registerEventListener<EventType::PrinterUniqueId>([this]() { setFolder(""); });
		setSort(StorageHelper::getData(m_view->getStorageKeys().sortBy),
				StorageHelper::getData(m_view->getStorageKeys().sortDescending));
	}

	void FilePresenter::onActivate()
	{
		ZoneScoped;
		if (OM::GetPrinterUniqueId().empty())
		{
			return;
		}
		requestFiles();
	}

	void FilePresenter::onConnect()
	{
		ZoneScoped;
		// setFolder("");
	}

	void FilePresenter::onDisconnect()
	{
		ZoneScoped;
		LOG_DBG("Clearing files");
		UI_LOCK();
		m_items.clear();
		m_view->setFileCount(0);
	}
} // namespace UI

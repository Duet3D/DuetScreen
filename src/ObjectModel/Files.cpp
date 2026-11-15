/*
 * Files.cpp
 *
 *  Created on: 3 Jan 2024
 *      Author: Andy Everitt
 */

#include "Debug.h"

#include "Files.h"

#include "Comm/Communication.h"
#include "Hardware/Duet.h"
#include "Hardware/Usb.h"
#include "ObjectModel/Tree/Tree.h"
#include "UI/Core/Model.h"
#include "utils/UnitSystem.h"
#include <algorithm>
#include <fstream>
#include <mutex>

namespace OM::FileSystem
{
	static std::unordered_map<std::string, FileListRequestPtr> s_fileListRequests;

	static FileContentsPtr s_fileContents;
	static std::vector<std::string> s_filaments;

	std::string FileSystemItem::GetPath() const
	{
		ZoneScoped;
		if (m_path.empty())
			return m_name;

		std::string path = m_path;
		if (path.back() != '/')
		{
			path += '/';
		}
		return path + m_name;
	}

	std::string FileSystemItem::GetReadableSize() const
	{
		ZoneScoped;
		return Units::formatBytes(m_size);
	}

	void FileSystemItem::SetName(const std::string name)
	{
		ZoneScoped;
		m_name = name.c_str();
		switch (m_type)
		{
		case FileSystemItemType::file:
			LOG_VERBOSE("Files: set file name to {:s}", m_name.c_str());
			break;
		case FileSystemItemType::folder:
			LOG_VERBOSE("Files: set folder name to {:s}", m_name.c_str());
			break;
		}
	}

	FileSystemItem::~FileSystemItem()
	{
		ZoneScoped;
	}

	FileListRequest::FileListRequest(const std::string& path, request_files_cb_t callback, bool run_every_time)
		: m_path(path)
		, m_runEveryTime(run_every_time)
		, m_requestTime(TimeHelper::getRunningTime())
	{
		ZoneScoped;
		if (callback)
		{
			m_callbacks.push_back({std::move(callback), run_every_time});
		}
	}

	FileListRequest::~FileListRequest()
	{
		ZoneScoped;
	}

	ItemPtr FileListRequest::AddFolder()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		auto folder = std::make_shared<FileSystemItem>(FileSystemItemType::folder);
		m_items.push_back(folder);
		return folder;
	}

	ItemPtr FileListRequest::AddFile()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		auto file = std::make_shared<FileSystemItem>(FileSystemItemType::file);
		m_items.push_back(file);
		return file;
	}

	void FileListRequest::SortItems(const SortBy by, const bool descending)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		SortFilesBy(m_items, by, descending);
	}

	ItemList FileListRequest::GetItemsCopy() const
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		return m_items;
	}
	size_t FileListRequest::GetItemCount() const
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		return m_items.size();
	}

	ItemPtr FileListRequest::GetLastItem() const
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		if (m_items.empty())
		{
			return nullptr;
		}
		return m_items.back();
	}

	ItemPtr FileListRequest::GetItem(const size_t index) const
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(m_mutex);
		if (index >= m_items.size())
		{
			return nullptr;
		}
		return m_items.at(index);
	}

	void FileListRequest::RunCallback()
	{
		ZoneScoped;
		std::vector<Callback> callbacks;
		{
			std::lock_guard<LockableBase(std::mutex)> lk(m_mutex);
			callbacks = m_callbacks;
			m_done = m_next == 0;
		}
		for (const auto& [callback, everyTime] : callbacks)
		{
			if (m_next > 0 && !everyTime)
			{
				continue;
			}
			LOG_DBG("Running callback for file list request {:s} with {:d} items", m_path, m_items.size());
			callback(m_items);
		}
	}

	bool FileListRequest::IsPending() const
	{
		std::lock_guard<LockableBase(std::mutex)> lk(m_mutex);
		return !m_done;
	}

	void FileListRequest::TakeCallbacksFrom(const FileListRequest& pending)
	{
		std::scoped_lock lk(m_mutex, pending.m_mutex);
		m_callbacks.insert(m_callbacks.begin(), pending.m_callbacks.begin(), pending.m_callbacks.end());
	}

	bool FileListRequest::IsRequestExpired() const
	{
		ZoneScoped;
		const auto elapsed = TimeHelper::getTimeSince(m_requestTime);
		return elapsed > PRINTER_REQUEST_TIMEOUT;
	}

	static std::string GetLocalFilePath(std::string_view filename)
	{
		ZoneScoped;
		filename = filename.substr(filename.find_last_of('/') + 1); // Get the file name only
		return fmt::format("/tmp/files/{}", filename);
	}

	FileContents::FileContents(std::string_view filename, request_file_contents_cb_t callback, bool runEveryTime)
		: m_filename(filename)
		, m_callback(callback)
		, m_runEveryTime(runEveryTime)
	{
		ZoneScoped;
		ClearData();
	}

	int FileContents::AppendData(std::string_view data)
	{
		ZoneScoped;
		if (data.empty())
		{
			return 0;
		}

		const std::string fullPath = GetLocalFilePath(m_filename);
		std::ofstream file(fullPath, std::ios::app | std::ios::binary);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open file {} for appending data", fullPath);
			return -1;
		}

		if (!file.write(data.data(), data.size()))
		{
			LOG_ERROR("Failed to write data to file {}", fullPath);
			return -2;
		}

		return 0;
	}

	int FileContents::GetData(std::string& outData) const
	{
		ZoneScoped;
		const std::string fullPath = GetLocalFilePath(m_filename);
		std::ifstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open file {} for reading", fullPath);
			return -1;
		}

		outData.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (outData.empty() && !file.eof())
		{
			LOG_ERROR("Failed to read data from file {}", fullPath);
			return -2;
		}
		return 0;
	}

	int FileContents::ClearData()
	{
		ZoneScoped;
		const std::string fullPath = GetLocalFilePath(m_filename);
		std::ofstream file(fullPath, std::ios::trunc | std::ios::binary);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open file {} for clearing data", fullPath);
			return -1;
		}
		return 0;
	}

	void FileContents::RunCallback()
	{
		ZoneScoped;
		if (m_parseErr != 0)
		{
			LOG_ERROR("File {} has parse error: {}", m_filename, m_parseErr);
			return;
		}

		if (m_next != 0 && !m_runEveryTime)
		{
			LOG_DBG("File {} has next offset: {}, not running callback", m_filename, m_next);
			return;
		}

		if (m_callback)
		{
			std::string contents;
			if (GetData(contents) == 0)
			{
				m_callback(contents);
			}
			else
			{
				LOG_ERROR("Failed to get data for callback from file {}", m_filename);
			}
		}
	}

	FileListRequestPtr GetFileListRequest(const std::string& path)
	{
		ZoneScoped;
		MODEL_LOCK();
		auto it = s_fileListRequests.find(path);
		if (it != s_fileListRequests.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void SortFilesBy(ItemList& items, std::function<bool(const ItemPtr&, const ItemPtr&)> sortFunc)
	{
		ZoneScoped;
		auto first = items.begin();
		auto last = items.end();
		if (first != last)
		{			// Ensure the range is not empty
			--last; // Point to the last valid item
			while (std::distance(first, last) > 0)
			{
				auto temp = last;
				while (temp != first)
				{
					auto prev = std::prev(temp);
					if (*prev == nullptr)
						return;
					if (sortFunc(*temp, *prev))
					{
						std::iter_swap(temp, prev);
					}
					--temp;
				}
				++first;
			}
		}
	}

	void SortFilesBy(ItemList& items, const SortBy by, const bool descending)
	{
		ZoneScoped;
		switch (by)
		{
		case SortBy::NAME:
			SortFilesBy(items,
						[descending](const ItemPtr& L, const ItemPtr& R)
						{
							if (L->GetType() == R->GetType())
								return descending == (L->GetName() > R->GetName());
							return L->GetType() < R->GetType();
						});
			break;
		case SortBy::DATE:
			SortFilesBy(items,
						[descending](const ItemPtr& L, const ItemPtr& R)
						{
							if (L->GetType() == R->GetType())
								return descending == (L->GetDate() > R->GetDate());
							return L->GetType() < R->GetType();
						});
			break;
		case SortBy::SIZE:
			SortFilesBy(items,
						[descending](const ItemPtr& L, const ItemPtr& R)
						{
							if (L->GetType() == R->GetType())
								return descending == (L->GetSize() > R->GetSize());
							return L->GetType() < R->GetType();
						});
			break;
		default:
			break;
		}
	}

	void RequestFiles(OM::Directories::DirectoryType baseFolder,
					  const std::string& path,
					  request_files_cb_t callback,
					  bool runEveryTime)
	{
		ZoneScoped;
		std::string full_path = fmt::format("{}{}", OM::Directories::GetDirectory(baseFolder), path);

		FileListRequestPtr cached;
		FileListRequestPtr reqPtr;
		{
			MODEL_LOCK();
			auto it = s_fileListRequests.find(full_path);
			if (it != s_fileListRequests.end())
			{
				cached = it->second;
			}

			reqPtr = std::make_shared<FileListRequest>(full_path, callback, runEveryTime);
			// Another requester may be waiting for this folder too: this request answers it as well
			if (cached && cached->IsPending())
			{
				reqPtr->TakeCallbacksFrom(*cached);
			}
			s_fileListRequests[full_path] = reqPtr;
		}

		if (cached && callback)
		{
			LOG_DBG("Running callback with cached {:d} file items for '{:s}'", cached->GetItemCount(), full_path);
			callback(cached->GetItemsCopy());
		}
		LOG_INFO("Files: requesting files in {:s}", full_path);
		Comm::DUET.RequestFileList(full_path, reqPtr->GetFirst());
	}

	void RequestUsbFiles(const std::string& path)
	{
		ZoneScoped;
		LOG_DBG("Requesting USB files in path '{:s}'", path);
#if 0
		ClearFileSystem();
		s_usbFolder = true;
		s_currentDirPath = path;
		std::vector<USB::FileInfo> files = USB::ListEntriesInDirectory(std::string("/mnt/usb1/") + path);
		size_t index = 0;
		for (auto& fileInfo : files)
		{
			if (fileInfo.d_type == DT_DIR)
			{
				Folder* folder = AddFolderAt(index);
				folder->SetName(fileInfo.d_name);
			}
			else if (fileInfo.d_type == DT_REG)
			{
				File* file = AddFileAt(index);
				file->SetName(fileInfo.d_name);
				file->SetSize(fileInfo.st_size);
				file->SetDate(fmt::format("{:d}", fileInfo.st_mtim));
			}
			index++;
		}
#endif
	}

	void RunFile(const ItemPtr& file)
	{
		ZoneScoped;
		if (file == nullptr || file->GetType() != FileSystemItemType::file)
		{
			LOG_ERROR("Invalid file to run");
			return;
		}

		std::string path = file->GetPath();
		if (path.find(OM::Directories::GetGcodesDirectory()) == std::string::npos)
			RunMacro(file->GetPath());
		else
			StartPrint(file->GetPath());
	}

	void RunMacro(const std::string& path)
	{
		ZoneScoped;
		Comm::DUET.SendGcodef("M98 P\"{:s}\"\n", path);
	}

	void UploadFile(const ItemPtr& file)
	{
		ZoneScoped;
		// TODO upload file
		LOG_DBG("Uploading file '{:s}'", file ? file->GetPath() : "null");
#if 0
		if (file == nullptr || file->GetType() != FileSystemItemType::file)
		{
			LOG_ERROR("Invalid file to upload");
			return;
		}

		std::string contents;
		if (!USB::ReadUsbFileContents(file->GetPath(), contents))
			return;
		Comm::DUET.UploadFile(fmt::format("/gcodes/{:s}", file->GetName()).c_str(), contents);
#endif
	}

	void StartPrint(const std::string& path)
	{
		ZoneScoped;
		Comm::DUET.SendGcodef("M32 \"{:s}\"\n", path);
	}

	void ResumePrint()
	{
		ZoneScoped;
		Comm::DUET.SendGcode("M24\n");
	}

	void PausePrint()
	{
		ZoneScoped;
		Comm::DUET.SendGcode("M25\n");
	}

	void StopPrint()
	{
		ZoneScoped;
		Comm::DUET.SendGcode("M0\n");
	}

	void PrintAgain()
	{
		ZoneScoped;
		std::string lastFile;
		{
			std::lock_guard lock(mutexModel);
			lastFile = Tree::get().root()["job"]["lastFileName"].text();
		}
		Comm::DUET.SendGcodef("M23 \"{:s}\"\nM24\n", lastFile);
	}

	void ClearFileSystem()
	{
		ZoneScoped;
		LOG_DBG("Clearing all file list requests");
		MODEL_LOCK();
		s_fileListRequests.clear();
		s_fileContents.reset();
	}

	void ClearFileList(const std::string& path)
	{
		ZoneScoped;
		MODEL_LOCK();
		auto it = s_fileListRequests.find(path);
		if (it != s_fileListRequests.end())
		{
			FileListRequestPtr req = it->second;
			req->ClearItems();
		}
	}

	void RequestFilaments()
	{
		ZoneScoped;
		LOG_DBG("Requesting filaments");
		RequestFiles(OM::Directories::DirectoryType::FILAMENTS,
					 "",
					 [](OM::FileSystem::ItemList files)
					 {
						 {
							 ZoneScopedN("RequestFilaments Callback");
							 MODEL_LOCK();
							 SortFilesBy(files, SortBy::NAME, false);
							 s_filaments.clear();
							 s_filaments.reserve(files.size());
							 for (const auto& item : files)
							 {
								 if (!item)
								 {
									 continue;
								 }
								 s_filaments.emplace_back(item->GetName());
							 }
						 }
						 Model::get().post<EventType::Filaments>(s_filaments);
					 });
	}

	const std::vector<std::string>& GetFilamentList()
	{
		ZoneScoped;
		MODEL_LOCK();
		return s_filaments;
	}

	void RequestFileContents(const OM::Directories::DirectoryType baseFolder,
							 std::string_view path,
							 request_file_contents_cb_t callback,
							 bool runEveryTime)
	{
		ZoneScoped;
		LOG_INFO("Requesting file contents of {}", path);
		std::string fullPath = fmt::format("{}{}", OM::Directories::GetDirectory(baseFolder), path);
		if (Comm::DUET.GetCommunicationType() == Comm::CommunicationType::network)
		{
			std::string contents;
			if (!Comm::DUET.DownloadFile(fullPath, callback))
			{
				LOG_ERROR("Failed to download file contents of {}", fullPath);
				return;
			}
		}
		else
		{
			s_fileContents = std::make_shared<FileContents>(fullPath, callback, runEveryTime);
			Comm::DUET.SendGcodef("M36.2 P\"{:s}\" S0\n", fullPath);
		}
	}

	FileContentsPtr GetCurrentFileRequestContents()
	{
		ZoneScoped;
		return s_fileContents;
	}

	std::string_view GetFileExtension(std::string_view filename)
	{
		ZoneScoped;
		size_t dot = filename.find_last_of('.');
		if (dot != std::string::npos)
		{
			return filename.substr(dot + 1);
		}
		return "";
	}
} // namespace OM::FileSystem

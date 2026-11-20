/*
 * Files.cpp
 *
 *  Created on: 3 Jan 2024
 *      Author: Andy Everitt
 */

#include "Debug.h"

#include "Files.h"

#include "Comm/Connection.h"
#include "Comm/FileInfoCache.h"
#include "Comm/Gcode.h"
#include "Configuration.h"
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
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lk(m_mutex);
		return !m_done;
	}

	void FileListRequest::TakeCallbacksFrom(const FileListRequest& pending)
	{
		ZoneScoped;
		std::scoped_lock lk(m_mutex, pending.m_mutex);
		m_callbacks.insert(m_callbacks.begin(), pending.m_callbacks.begin(), pending.m_callbacks.end());
	}

	bool FileListRequest::IsRequestExpired() const
	{
		ZoneScoped;
		const auto elapsed = TimeHelper::getTimeSince(m_requestTime);
		return elapsed > PRINTER_REQUEST_TIMEOUT;
	}

	/** Ask for the page of the folder `dir` from item `first` on; it is handed to FileListArrived() */
	static void RequestPage(const std::string& dir, size_t first)
	{
		ZoneScoped;
		Comm::Connection::get().listFiles(
			dir, first, [dir](std::expected<Comm::FileList, Comm::ReplyError> reply) { FileListArrived(dir, reply); });
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
							ZoneScopedN("SortFilesBy_cb");
							if (L->GetType() == R->GetType())
								return descending == (L->GetName() > R->GetName());
							return L->GetType() < R->GetType();
						});
			break;
		case SortBy::DATE:
			SortFilesBy(items,
						[descending](const ItemPtr& L, const ItemPtr& R)
						{
							ZoneScopedN("SortFilesBy_cb");
							if (L->GetType() == R->GetType())
								return descending == (L->GetDate() > R->GetDate());
							return L->GetType() < R->GetType();
						});
			break;
		case SortBy::SIZE:
			SortFilesBy(items,
						[descending](const ItemPtr& L, const ItemPtr& R)
						{
							ZoneScopedN("SortFilesBy_cb");
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
		const std::string base = OM::Directories::GetDirectory(baseFolder);
		if (base.empty())
		{
			// The folders arrive after the board (whose id announces the printer): asking now would name no folder
			LOG_DBG("Files: not requesting files in '{:s}' until the printer's folders are known", path);
			return;
		}
		std::string full_path = fmt::format("{}{}", base, path);

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

		// What the folder held when last listed, while it is listed again; nothing yet if it is still being listed (its
		// requesters are answered with this request)
		if (cached && callback && !cached->IsPending())
		{
			LOG_DBG("Running callback with cached {:d} file items for '{:s}'", cached->GetItemCount(), full_path);
			callback(cached->GetItemsCopy());
		}
		LOG_INFO("Files: requesting files in {:s}", full_path);
		RequestPage(full_path, reqPtr->GetFirst());
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
		Comm::Connection::get().sendGcodef("M98 P{:s}\n", Comm::quoted(path));
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
		Comm::Connection::get().writeFile(fmt::format("/gcodes/{:s}", file->GetName()), std::move(contents));
#endif
	}

	void StartPrint(const std::string& path)
	{
		ZoneScoped;
		Comm::Connection::get().sendGcodef("M32 {:s}\n", Comm::quoted(path));
	}

	void ResumePrint()
	{
		ZoneScoped;
		Comm::Connection::get().sendGcode("M24\n");
	}

	void PausePrint()
	{
		ZoneScoped;
		Comm::Connection::get().sendGcode("M25\n");
	}

	void StopPrint()
	{
		ZoneScoped;
		Comm::Connection::get().sendGcode("M0\n");
	}

	void PrintAgain()
	{
		ZoneScoped;
		std::string lastFile;
		{
			std::lock_guard lock(mutexModel);
			lastFile = Tree::get().root()["job"]["lastFileName"].text();
		}
		Comm::Connection::get().sendGcodef("M23 {:s}\nM24\n", Comm::quoted(lastFile));
	}

	void ClearFileSystem()
	{
		ZoneScoped;
		LOG_DBG("Clearing all file list requests and the filaments");
		MODEL_LOCK();
		s_fileListRequests.clear();
		s_filaments.clear();
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
						 ZoneScopedN("RequestFilaments_cb");
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
							 request_file_contents_cb_t callback)
	{
		ZoneScoped;
		std::string fullPath = fmt::format("{}{}", OM::Directories::GetDirectory(baseFolder), path);
		LOG_INFO("Requesting file contents of {}", fullPath);
		Comm::Connection::get().readFile(
			fullPath,
			[fullPath, callback = std::move(callback)](std::expected<std::string, Comm::ReplyError> contents)
			{
				ZoneScopedN("lambda_cb");
				if (!contents)
				{
					LOG_ERROR("Couldn't read {:s}: {:s}", fullPath, contents.error().message);
					return;
				}
				callback(*contents);
			});
	}

	void FileListArrived(const std::string& dir, const std::expected<Comm::FileList, Comm::ReplyError>& reply)
	{
		ZoneScoped;
		FileListRequestPtr request = GetFileListRequest(dir);
		if (!request)
		{
			LOG_WARN("Not expecting to receive file list data for \"{:s}\", ignoring", dir);
			return;
		}
		if (!reply)
		{
			LOG_WARN("Couldn't list {:s}: {:s}", dir, reply.error().message);
			request->ClearItems();
			request->SetNext(0);
			request->RunCallback();
			return;
		}
		if (reply->first != request->GetNext())
		{
			LOG_WARN(
				"First index {:d} does not match expected next index {:d}, ignoring", reply->first, request->GetNext());
			return;
		}

		if (reply->first == 0)
		{
			request->ClearItems();
		}
		request->SetFirst(reply->first);
		for (const Comm::FileEntry& entry : reply->files)
		{
			ItemPtr item = entry.folder ? request->AddFolder() : request->AddFile();
			item->SetName(entry.name);
			item->SetPath(dir);
			item->SetSize(static_cast<size_t>(entry.size));
			item->SetDate(entry.date);
		}

		// The G-code files' information (and thumbnails) is kept; other folders' files have none
		if (dir.find(OM::Directories::GetGcodesDirectory()) != std::string::npos)
		{
			for (const ItemPtr& item : request->GetItemsCopy())
			{
				if (item->GetType() == FileSystemItemType::folder)
				{
					continue;
				}
				Comm::FileInfoCache::get().refresh(item->GetPath(), item->GetDate());
			}
		}

		request->SetNext(reply->next);
		request->RunCallback();
		if (reply->next != 0)
		{
			RequestPage(dir, reply->next);
		}
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

/*
 * Files.h
 *
 *  Created on: 3 Jan 2024
 *      Author: Andy Everitt
 */

#ifndef JNI_OBJECTMODEL_FILES_HPP_
#define JNI_OBJECTMODEL_FILES_HPP_

#include "ObjectModel/Directories.h"
#include "tracy/Tracy.hpp"
#include "utils/TimeHelper.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace OM::FileSystem
{
	enum class FileSystemItemType
	{
		folder,
		file,
	};

	class FileSystemItem
	{
	  public:
		FileSystemItem(const FileSystemItemType type)
			: m_type(type)
			, m_size(0)
		{
		}
		FileSystemItem(const FileSystemItemType type, const std::string& name)
			: m_name(name)
			, m_type(type)
			, m_size(0)
		{
		}

		~FileSystemItem();

		const std::string& GetName() const { return m_name; }
		FileSystemItemType GetType() const { return m_type; }
		void SetName(const std::string name);
		std::string GetPath() const;
		void SetPath(const std::string& path) { m_path = path; }
		const std::string& GetDate() const { return m_date; }
		void SetDate(const std::string& date) { m_date = date; }
		size_t GetSize() const { return m_size; }
		std::string GetReadableSize() const;
		void SetSize(const size_t size) { m_size = size; }

	  private:
		std::string m_path;
		std::string m_name;
		FileSystemItemType m_type;
		size_t m_size;
		std::string m_date;
	};

	class File : public FileSystemItem
	{
	  public:
		File()
			: FileSystemItem(FileSystemItemType::file)
		{
		}
		File(const std::string& name)
			: FileSystemItem(FileSystemItemType::file, name)
		{
		}
	};

	class Folder : public FileSystemItem
	{
	  public:
		Folder()
			: FileSystemItem(FileSystemItemType::folder)
		{
		}
		Folder(const std::string& name)
			: FileSystemItem(FileSystemItemType::folder, name)
		{
		}
	};

	// Elements are shared_ptrs; access to the owning container is synchronized in FileListRequest
	using ItemPtr = std::shared_ptr<FileSystemItem>;
	using ItemList = std::vector<ItemPtr>;

	using request_files_cb_t = std::function<void(ItemList items)>; // passing by ref caused rare issues
	using request_file_contents_cb_t = std::function<void(std::string_view contents)>;

	enum class SortBy
	{
		NAME,
		DATE,
		SIZE
	};

	class FileListRequest
	{
	  public:
		FileListRequest(const std::string& path, request_files_cb_t callback, bool run_every_time);
		~FileListRequest();

		void SetFirst(const size_t first) { m_first = first; }
		void SetNext(const size_t next) { m_next = next; }

		ItemPtr AddFolder();
		ItemPtr AddFile();
		void ClearItems()
		{
			std::lock_guard<LockableBase(std::mutex)> lk(m_mutex);
			m_items.clear();
		}
		void SortItems(const SortBy by, const bool descending);

		const std::string& GetDir() const { return m_path; }
		// Returns a snapshot copy of the items to avoid races
		ItemList GetItemsCopy() const;
		size_t GetItemCount() const;
		ItemPtr GetLastItem() const;
		ItemPtr GetItem(const size_t index) const;
		size_t GetFirst() const { return m_first; }
		size_t GetNext() const { return m_next; }
		bool RunEveryTime() const { return m_runEveryTime; }

		/** Hand the listing to each callback: after every page for those run every time, else after the last */
		void RunCallback();

		/** Whether the listing hasn't finished yet: its last page hasn't arrived */
		bool IsPending() const;

		/** Answer `pending`'s callbacks too, which were waiting for the same folder: a request replaces it */
		void TakeCallbacksFrom(const FileListRequest& pending);

		bool IsRequestExpired() const;

	  private:
		/** One requester's callback */
		struct Callback
		{
			request_files_cb_t callback; // Handed the listing
			bool everyTime;				 // Called after every page, not only the last
		};

		std::string m_path;
		std::vector<Callback> m_callbacks; // Every requester waiting for this folder
		bool m_runEveryTime;
		bool m_done = false; // The last page has arrived
		mutable TracyLockable(std::mutex, m_mutex);
		ItemList m_items;
		size_t m_first = 0;
		size_t m_next = 0;
		std::chrono::milliseconds m_requestTime;
	};

	using FileListRequestPtr = std::shared_ptr<FileListRequest>;
	using FileListRequestWeakPtr = std::weak_ptr<FileListRequest>;

	class FileContents
	{
	  public:
		FileContents(std::string_view filename,
					 request_file_contents_cb_t callback = nullptr,
					 bool runEveryTime = false);

		std::string_view GetFilename() const { return m_filename; }
		int AppendData(std::string_view data);
		int GetData(std::string& outData) const;
		int ClearData();

		void SetOffset(uint32_t offset) { m_offset = offset; }
		uint32_t GetOffset() const { return m_offset; }

		void SetNext(uint32_t next) { m_next = next; }
		uint32_t GetNext() const { return m_next; }

		void SetParseError(int16_t parseErr) { m_parseErr = parseErr; }
		int16_t GetParseError() const { return m_parseErr; }

		void RunCallback();

	  private:
		std::string m_filename; // Path to the file
		request_file_contents_cb_t m_callback;
		bool m_runEveryTime = false;
		uint32_t m_offset = 0;
		uint32_t m_next = 0;
		int16_t m_parseErr = 0;
	};

	using FileContentsPtr = std::shared_ptr<FileContents>;

	/* File List */

	FileListRequestPtr GetFileListRequest(const std::string& path);

	void SortFilesBy(ItemList& items, std::function<bool(const ItemPtr&, const ItemPtr&)> sortFunc);
	void SortFilesBy(ItemList& items, const SortBy sortBy, const bool descending);
	/**
	 * Ask the printer for the files in `path`, under the base folder `baseFolder`; `callback` is given them.
	 * Nothing is asked until the printer has said where its base folder is (`directories` in its object model):
	 * whoever wants files lists them again once it has.
	 */
	void RequestFiles(const OM::Directories::DirectoryType baseFolder,
					  const std::string& path,
					  request_files_cb_t callback,
					  bool runEveryTime = false);
	void RequestUsbFiles(const std::string& path);
	void RunFile(const ItemPtr& file);
	void RunMacro(const std::string& path);
	void UploadFile(const ItemPtr& file);
	void StartPrint(const std::string& path);
	void ResumePrint();
	void PausePrint();
	void StopPrint();
	void PrintAgain();
	void ClearFileSystem();
	void ClearFileList(const std::string& path);

	/* Filaments */
	void RequestFilaments();
	const std::vector<std::string>& GetFilamentList();

	/* File Contents */

	void RequestFileContents(const OM::Directories::DirectoryType baseFolder,
							 std::string_view path,
							 request_file_contents_cb_t callback,
							 bool runEveryTime = false);
	FileContentsPtr GetCurrentFileRequestContents();

	/* Misc */

	std::string_view GetFileExtension(std::string_view fileName);
} // namespace OM::FileSystem

#endif /* JNI_OBJECTMODEL_FILES_HPP_ */

/*
 * FileInfo.cpp
 *
 *  Created on: 12 Mar 2024
 *      Author: andy
 */

#include "Debug.h"
#include "DebugCommands.h"

#include "FileInfo.h"

#include "Configuration.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "nameof.hpp"
#include "utils/utils.h"
#include <fstream>
#include <sys/stat.h>
#include <utils/TimeHelper.h>

namespace Comm
{
	static TracyLockable(std::recursive_mutex, s_mutex);

	FileInfo::FileInfo()
	{
		LOG_VERBOSE("Created new fileinfo");
	}

	FileInfo::~FileInfo()
	{
		LOG_VERBOSE("Deleted fileinfo {:s}", filename.c_str());
	}

	std::shared_ptr<Thumbnail> FileInfo::GetThumbnail(size_t index) const
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		if (index >= m_thumbnails.size())
		{
			return nullptr;
		}

		return m_thumbnails[index];
	}

	std::shared_ptr<Thumbnail> FileInfo::GetOrCreateThumbnail(size_t index)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		if (index >= m_thumbnails.size())
		{
			m_thumbnails.resize(index + 1);
			m_thumbnails[index] = std::make_shared<Thumbnail>(filename.GetRef());
		}

		return m_thumbnails[index];
	}

	size_t FileInfo::ClearThumbnails(size_t fromIndex)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		size_t count = m_thumbnails.size() - fromIndex;
		m_thumbnails.resize(fromIndex);
		return count;
	}

	tm FileInfo::GetPrintTime() const
	{
		ZoneScoped;
		tm time = ParseSeconds(printTime);
		LOG_DBG("Print time ({:d}): {:d}:{:2d}:{:2d}", printTime, time.tm_hour, time.tm_min, time.tm_sec);
		return time;
	}

	void to_json(nlohmann::json& j, const FileInfo& c)
	{
		ZoneScoped;
		std::vector<nlohmann::json> thumbnails;
		for (const auto& thumb : c.GetThumbnails())
		{
			if (thumb)
				thumbnails.push_back(*thumb); // convert Thumbnail to json
		}

		j = nlohmann::json{{"filename", c.filename.c_str()},
						   {"filament", c.filament},
						   {"generatedBy", c.generatedBy.c_str()},
						   {"height", c.height},
						   {"lastModified", c.lastModified.c_str()},
						   {"layerHeight", c.layerHeight},
						   {"numLayers", c.numLayers},
						   {"printTime", c.printTime},
						   {"simulatedTime", c.simulatedTime},
						   {"size", c.size},
						   {"thumbnails", thumbnails}};
	}

	void from_json(const nlohmann::json& j, FileInfo& c)
	{
		ZoneScoped;
		if (j.contains("filename"))
			c.filename.copy(j.at("filename").get<std::string>().c_str());
		if (j.contains("filament"))
			j.at("filament").get_to(c.filament);
		if (j.contains("generatedBy"))
			c.generatedBy.copy(j.at("generatedBy").get<std::string>().c_str());
		if (j.contains("height"))
			j.at("height").get_to(c.height);
		if (j.contains("lastModified"))
			c.lastModified.copy(j.at("lastModified").get<std::string>().c_str());
		if (j.contains("layerHeight"))
			j.at("layerHeight").get_to(c.layerHeight);
		if (j.contains("numLayers"))
			j.at("numLayers").get_to(c.numLayers);
		if (j.contains("printTime"))
			j.at("printTime").get_to(c.printTime);
		if (j.contains("simulatedTime"))
			j.at("simulatedTime").get_to(c.simulatedTime);
		if (j.contains("size"))
			j.at("size").get_to(c.size);

		if (j.contains("thumbnails"))
		{
			const auto& thumbnails = j.at("thumbnails");
			for (size_t i = 0; i < thumbnails.size(); ++i)
			{
				auto thumb = c.GetOrCreateThumbnail(i);
				thumbnails[i].get_to(*thumb);
			}
			c.ClearThumbnails(thumbnails.size());
		}
	}

	FileInfoCache::FileInfoCache()
	{
		ZoneScoped;
		Model::get().addEventListener<EventType::PrinterUniqueId>(
			[this]()
			{
				if (!OM::GetPrinterUniqueId().empty())
					LoadCacheFromMemory();
			});
	}

	std::optional<std::filesystem::path> FileInfoCache::GetCachePath() const
	{
		ZoneScoped;
		std::string_view uid = OM::GetPrinterUniqueId();
		if (uid.empty())
		{
			LOG_DBG("Cannot get file info cache path: printer unique ID is empty");
			return std::nullopt;
		}

		return std::filesystem::path(NVS_FOLDER) / "fileinfo_cache" / uid;
	}

	bool FileInfoCache::LoadFileInfoFromFile(const std::filesystem::path& path, FileInfo& fileInfo)
	{
		ZoneScoped;
		nlohmann::json data;
		try
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				LOG_ERROR("Failed to open file info cache file: {:s}", path.string());
				return false;
			}
			file >> data;
			data.get_to(fileInfo);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Failed to load file info from {:s}: {:s}", path.string(), e.what());
			return false;
		}

		LOG_DBG("Loaded file info from {:s}", path.string());
		return true;
	}

	void FileInfoCache::LoadCacheFromMemory()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);

		ClearCache();

		GetCachePath().transform(
			[this](const std::filesystem::path& path)
			{
				if (!std::filesystem::exists(path))
				{
					LOG_INFO("Creating file info cache directory: {:s}", path.string());
					std::filesystem::create_directories(path);
				}

				LOG_INFO("Loading file info cache from: {:s}", path.string());
				LoadCacheFolder(path);
				return path;
			});
	}

	void FileInfoCache::LoadCacheFolder(const std::filesystem::path& folderPath)
	{
		ZoneScoped;
		for (const auto& entry : std::filesystem::directory_iterator(folderPath))
		{
			if (entry.is_regular_file())
			{
				if (entry.path().extension() != ".json")
				{
					continue;
				}
				LOG_DBG("Loading cached file info from {:s}", entry.path().string());
				auto fileInfo = std::make_shared<FileInfo>();

				if (LoadFileInfoFromFile(entry.path(), *fileInfo))
				{
					LOG_DBG("Loaded cached file info for {:s}", entry.path().string());
					m_cache[fileInfo->filename.c_str()] = fileInfo;

					if (!IsThumbnailCached(fileInfo->filename.c_str(), fileInfo->lastModified.c_str()))
					{
						FILEINFO_CACHE->QueueThumbnailRequest(fileInfo->filename.c_str());
					}
				}
				else
				{
					LOG_WARN("Failed to load cached file info from {:s}", entry.path().string());
				}
			}
			else if (entry.is_directory())
			{
				LoadCacheFolder(entry.path());
			}
		}
	}

	bool FileInfoCache::SaveCacheToFile(const FileInfo& fileInfo)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);

		bool ret = false;
		GetCachePath().transform(
			[&](const std::filesystem::path& path)
			{
				std::string file_path_str = fmt::format("{:s}/{:s}.json", path.string(), fileInfo.filename.c_str());
				utils::replaceSubstring(file_path_str, ":", "\\%3A");

				auto file_path = std::filesystem::path(file_path_str);

				nlohmann::json data = fileInfo;
				LOG_INFO("Saving file info cache to: {:s}", file_path_str);

				if (!std::filesystem::exists(file_path.parent_path()))
				{
					LOG_INFO("Creating file info cache directory: {:s}", file_path.parent_path().string());
					std::filesystem::create_directories(file_path.parent_path());
				}
				try
				{
					std::ofstream file(file_path);
					if (!file.is_open())
					{
						LOG_ERROR("Failed to open file info cache file for writing: {:s}", file_path_str);
						return path;
					}
					file << data.dump(4);
					LOG_DBG("Saved cached file info to {:s}", file_path_str);
					ret = true;
				}
				catch (const std::exception& e)
				{
					LOG_ERROR("Failed to save file info to {:s}: {:s}", file_path_str, e.what());
				}
				return path;
			});

		return ret;
	}

	bool FileInfoCache::SaveCache()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);

		bool ret = false;
		GetCachePath().transform(
			[this, &ret](const std::filesystem::path& path)
			{
				if (!std::filesystem::exists(path))
				{
					LOG_INFO("Creating file info cache directory: {:s}", path.string());
					std::filesystem::create_directories(path);
				}

				LOG_INFO("Saving file info cache to: {:s}", path.string());
				for (const auto& [filename, fileInfo] : m_cache)
				{
					std::filesystem::path filePath = path / (std::string(filename) + ".json");
					nlohmann::json data = *fileInfo;

					try
					{
						std::ofstream file(filePath);
						if (!file.is_open())
						{
							LOG_ERROR("Failed to open file info cache file for writing: {:s}", filePath.string());
							continue;
						}
						file << data.dump(4);
						LOG_DBG("Saved cached file info to {:s}", filePath.string());
						ret = true;
					}
					catch (const std::exception& e)
					{
						LOG_ERROR("Failed to save file info to {:s}: {:s}", filePath.string(), e.what());
					}
				}
				return path;
			});

		return ret;
	}

	void FileInfoCache::Spin()
	{
		ZoneScoped;
		auto now = TimeHelper::getRunningTime();

		// Timeout any request that hasn't received a response within the timeout period
		{
			std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
			for (auto it = m_fileInfoRequestQueue.begin(); it != m_fileInfoRequestQueue.end();)
			{
				FileInfoRequestPtr request = *it;
				it++;

				if (request->HasTimedOut(FILE_CACHE_REQUEST_TIMEOUT) || request->IsFailed())
				{
					LOG_WARN("File info request timed out for {:s}", request->GetData()->filename.c_str());
					request->Complete(true);
					m_fileInfoRequestQueue.remove(request);
#if 0
				LOG_WARN("Requeuing failed file info request for {:s}", request->GetData()->filename.c_str());
				QueueFileInfoRequest(request->GetData()->filename.c_str());
#endif
				}
			}
		}

		{
			std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
			for (auto it = m_thumbnailRequestQueue.begin(); it != m_thumbnailRequestQueue.end();)
			{
				ThumbnailRequestPtr request = *it;
				it++;

				if (!ThumbnailIsValid(*request->GetData()))
				{
					LOG_ERROR("Invalid thumbnail");
					m_thumbnailRequestQueue.remove(request);
					continue;
				}

				if (request->HasTimedOut(FILE_CACHE_REQUEST_TIMEOUT) || request->IsFailed())
				{
#if DEBUG
					ThumbnailPtr t = request->GetData();
#endif
					if (request->IsFailed())
					{
						LOG_WARN("Thumbnail request failed for {:s}", request->GetData()->filename.c_str());
					}
					else
					{
						LOG_WARN("Thumbnail request timed out for {:s}", request->GetData()->filename.c_str());
					}

					request->Complete(true);
					std::string_view filename = request->GetData()->filename.c_str();
					DeleteCachedThumbnail(filename);
					m_thumbnailRequestQueue.remove(request);
#if 0
				LOG_WARN("Requeuing thumbnail request for {:s}", request->GetData()->filename.c_str());
				QueueThumbnailRequest(filename);
#endif
				}
			}
		}

		if ((OM::PrintInProgress()) && (now - m_lastRequestTime < BACKGROUND_FILE_CACHE_POLL_INTERVAL))
		{
			LOG_VERBOSE("Skipping file info cache spin");
			return;
		}

		// Start a new request if there are no requests in progress
		size_t fileInfoRequested = 0;
		{
			std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
			for (auto it = m_fileInfoRequestQueue.begin(); it != m_fileInfoRequestQueue.end(); it++)
			{
				FileInfoRequestPtr request = *it;

				if (request->IsRequested())
				{
					fileInfoRequested++;
					continue;
				}

				if (fileInfoRequested >= MAX_FILEINFO_REQUESTS)
				{
					break;
				}

				request->RequestData();
				fileInfoRequested++;
			}
		}

		// Start a new thumbnail request if there are none in progress
		size_t thumbnailsRequested = 0;
		{
			std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
			for (auto it = m_thumbnailRequestQueue.begin(); it != m_thumbnailRequestQueue.end();)
			{
				ThumbnailRequestPtr request = *it;
				ThumbnailPtr thumbnail = request->GetData();
				if (thumbnail == nullptr)
				{
					// Should be impossible
					LOG_ERROR("Null thumbnail");
					continue;
				}

				++it;

				switch (thumbnail->context.state)
				{
				case ThumbnailState::Init:
				case ThumbnailState::DataRequest:
					if (!request->RequestData())
					{
					}
					thumbnailsRequested++;
					break;
				case ThumbnailState::Data:
				case ThumbnailState::DataWait:
					LOG_VERBOSE("Thumbnail request in progress for {:s}, state={:d}",
								thumbnail->filename.c_str(),
								(int)thumbnail->context.state);
					thumbnailsRequested++;
					break;
				case ThumbnailState::Cached:
					thumbnail->image.Close();
					LOG_DBG("Updating thumbnail {:s}", thumbnail->filename.c_str());
					ThumbnailRequestComplete(thumbnail->filename.c_str());
					Model::get().post<EventType::ThumbnailData>(std::string(thumbnail->filename.c_str()));
					break;
				}

				if (thumbnailsRequested >= MAX_THUMBNAIL_REQUESTS)
				{
					break;
				}
			}
		}
	}

	bool FileInfoCache::IsThumbnailCached(const std::string& filepath, const char* lastModified)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		// Does a thumbnail file exist in the file system?
		if (!::IsThumbnailCached(filepath, false))
		{
			LOG_DBG("Thumbnail file for {:s} does not exist", filepath.c_str());
			return false;
		}

		// Do we have a cache for the files meta data?
		if (m_cache.find(filepath) == m_cache.end())
		{
			LOG_DBG("No file info cached for {:s}", filepath.c_str());
			return false;
		}

		FileInfoPtr fileInfo = m_cache[filepath];

		// Is the last modified time the same?
		if (!fileInfo->lastModified.Equals(lastModified))
		{
			LOG_DBG("Last modified time for {:s} does not match", filepath.c_str());
			return false;
		}

		return true;
	}

	/**
	 * @brief Get file info for the given gcode file path
	 * @param filepath
	 * @return shared_ptr to FileInfo object
	 */
	FileInfoPtr FileInfoCache::GetFileInfo(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		if (m_cache.find(filepath) == m_cache.end())
		{
			return nullptr;
		}
		return m_cache[filepath];
	}

	bool FileInfoCache::IsFileInfoRequestInProgress()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		return std::find_if(m_fileInfoRequestQueue.begin(),
							m_fileInfoRequestQueue.end(),
							[](const FileInfoRequestPtr request)
							{ return request->IsInProgress(); }) != m_fileInfoRequestQueue.end();
	}

	void FileInfoCache::FileInfoArrived(const std::string& filepath,
										const std::expected<FileInfoReply, ReplyError>& reply)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		FileInfoRequestPtr request = GetFileInfoRequest(filepath);
		if (request == nullptr)
		{
			LOG_WARN("File info for {:s} wasn't asked for, ignoring it", filepath);
			return;
		}
		if (!reply)
		{
			LOG_WARN("No file info for {:s}: {:s}", filepath, reply.error().message);
			request->Complete(true);
			return;
		}

		FileInfo& fileInfo = *request->GetData();
		fileInfo.lastModified.copy(reply->lastModified.c_str());
		fileInfo.size = static_cast<uint32_t>(reply->size);
		fileInfo.height = static_cast<float>(reply->height);
		fileInfo.layerHeight = static_cast<float>(reply->layerHeight);
		fileInfo.numLayers = reply->numLayers;
		fileInfo.printTime = reply->printTime;
		fileInfo.simulatedTime = reply->simulatedTime;
		fileInfo.generatedBy.copy(reply->generatedBy.c_str());
		fileInfo.filament.assign(reply->filament.begin(), reply->filament.end());
		for (size_t i = 0; i < reply->thumbnails.size(); i++)
		{
			const ThumbnailInfo& sent = reply->thumbnails[i];
			ThumbnailPtr thumbnail = fileInfo.GetOrCreateThumbnail(i);
			if (!thumbnail->meta.SetImageFormat(sent.format))
			{
				LOG_WARN("Thumbnail {:d} of {:s} has a format the screen can't show: {:s}", i, filepath, sent.format);
			}
			thumbnail->meta.width = sent.width;
			thumbnail->meta.height = sent.height;
			thumbnail->meta.offset = sent.offset;
			thumbnail->meta.size = sent.size;
			thumbnail->context.next = sent.offset;
		}
		fileInfo.ClearThumbnails(reply->thumbnails.size()); // the file may have been replaced by one with fewer
		LOG_INFO("FileInfo: filename({:s}) thumbnails({:d})", filepath, fileInfo.GetThumbnailCount());

		FileInfoRequestComplete(filepath);
		if (!IsThumbnailCached(filepath, fileInfo.lastModified.c_str()))
		{
			QueueThumbnailRequest(filepath);
		}
	}

	void FileInfoCache::ThumbnailArrived(const std::string& filepath, const std::expected<FileChunk, ReplyError>& reply)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		ThumbnailRequestPtr request = GetThumbnailRequest(filepath);
		if (request == nullptr)
		{
			LOG_WARN("Not expecting to receive thumbnail data for \"{:s}\", ignoring", filepath);
			return;
		}
		ThumbnailPtr thumbnail = request->GetData();
		if (!reply)
		{
			LOG_ERROR("No thumbnail data for {:s}: {:s}", filepath, reply.error().message);
			thumbnail->context.err = reply.error().code;
			thumbnail->context.parseErr = ThumbnailContext::ParseErr::RrfError;
			request->Complete(true);
			return;
		}
		if (reply->offset != thumbnail->context.next)
		{
			LOG_WARN("thumbnail offset mismatch, expected {:d} got {:d}", thumbnail->context.next, reply->offset);
			thumbnail->context.parseErr = ThumbnailContext::ParseErr::MismatchOffset;
			request->Complete(true);
			return;
		}
		request->Receiving();
		thumbnail->context.offset = reply->offset;
		thumbnail->context.next = reply->next;

		const int ret = ThumbnailDecodeChunk(*thumbnail, reply->data);
		if (ret < 0)
		{
			LOG_ERROR("failed to decode thumbnail chunk {:d}.\n", ret);
			request->Complete(true);
			return;
		}
		thumbnail->context.state = reply->next == 0 ? ThumbnailState::Cached : ThumbnailState::DataRequest;
	}

	/**
	 * @brief Get the FileInfoRequest object for the given filepath
	 * @param filepath
	 * @return
	 */
	FileInfoCache::FileInfoRequestPtr FileInfoCache::GetFileInfoRequest(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		for (FileInfoRequestPtr request : m_fileInfoRequestQueue)
		{
			if (request->GetData()->filename.Equals(filepath.c_str()))
			{
				return request;
			}
		}
		return nullptr;
	}

#if 0
	bool FileInfoCache::IsFileInfoRequestQueued(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		return GetFileInfoRequest(filepath) != nullptr;
	}

	bool FileInfoCache::IsFileInfoRequestInProgress(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		FileInfoRequestPtr request = GetFileInfoRequest(filepath);
		if (request == nullptr)
		{
			return false;
		}

		return request->IsInProgress();
	}
#endif

	void FileInfoCache::FileInfoRequestComplete(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		LOG_DBG("File info request complete for {:s}", filepath.c_str());

		FileInfoRequestPtr request = GetFileInfoRequest(filepath);

		if (request == nullptr)
		{
			return;
		}

		request->Complete();
		m_cache[filepath] = request->GetData();

		m_fileInfoRequestQueue.remove(request);

		SaveCacheToFile(*request->GetData());
		// What is shown of the file (its print time, filament) can be now
		Model::get().post<EventType::FileInfo>(filepath);
	}

	bool FileInfoCache::FileInfoRequest::RequestDataInner()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		if (m_data == nullptr)
		{
			return false;
		}

		LOG_DBG("Requesting file info for \"{:s}\", ", m_data->filename.c_str());
		return DUET.RequestFileInfo(m_data->filename.c_str());
	}

	bool FileInfoCache::ThumbnailRequest::RequestDataInner()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		if (m_data == nullptr)
		{
			return false;
		}

		LOG_DBG("Requesting thumbnail for \"{:s}\", {:d}x{:d}",
				m_data->filename.c_str(),
				m_data->meta.width,
				m_data->meta.height);

		if (m_data->filename.IsEmpty() || m_data->meta.offset == 0)
		{
			LOG_WARN("Not enough information to request thumbnail for {:s}", m_data->filename.c_str());
			return false;
		}

		if (!ThumbnailIsValid(*m_data))
		{
			LOG_ERROR("thumbnail meta invalid.\n");
			return false;
		}

		if (m_data->context.state == ThumbnailState::DataRequest)
		{
			m_data->context.state = ThumbnailState::DataWait;
			return DUET.RequestThumbnail(m_data->filename.c_str(), m_data->context.next);
		}
		else
		{
			m_data->context.Init();

			std::string_view filename = m_data->filename.c_str();
			std::filesystem::path filepath =
				GetThumbnailPath(filename, true); // Use temp folder for in-progress thumbnails
			if (!m_data->image.New(m_data->meta, filepath))
			{
				LOG_ERROR("Failed to create thumbnail file {:s}.", filename);
				return false;
			}
			m_data->context.state = ThumbnailState::DataWait;
			m_data->context.next = m_data->meta.offset;
			return DUET.RequestThumbnail(m_data->filename.c_str(), m_data->meta.offset);
		}
	}

	void FileInfoCache::ClearCache()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		LOG_INFO("Clearing file info cache");

		m_cache.clear();
		m_fileInfoRequestQueue.clear();
		m_thumbnailRequestQueue.clear();
		ClearAllCachedThumbnails();
		LOG_DBG("Cache cleared");
	}

	/**
	 * @brief Queue a file info request for processing
	 * @param filepath The filepath to request information for
	 * @param next If true, try to queue it at the next position
	 * @return True if the request was successfully queued, false otherwise
	 */
	bool FileInfoCache::QueueFileInfoRequest(const std::string& filepath, bool next)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		LOG_DBG("Attempting to queue file info request for {:s}", filepath.c_str());
		for (auto it = m_fileInfoRequestQueue.begin(); it != m_fileInfoRequestQueue.end();)
		{
			FileInfoRequestPtr request = *it;
			it++;

			if (request->GetData()->filename.Equals(filepath.c_str()))
			{
				if (request->IsInProgress())
				{
					// Request already in progress so don't remove it from queue or add it again
					LOG_WARN("File info request for {:s} already in progress", filepath.c_str());
					return false;
				}

				if (!next && !request->IsFailed())
				{
					// Request is already in the queue but has not started
					LOG_DBG("File info request for {:s} already queued", filepath.c_str());
					return false;
				}

				// Request in the queue but not in progress or at the front
				m_fileInfoRequestQueue.remove(request);
				break;
			}
		}
		LOG_DBG("Queueing file info request for {:s}", filepath.c_str());
		if (next)
		{

			auto it = std::find_if(m_fileInfoRequestQueue.begin(),
								   m_fileInfoRequestQueue.end(),
								   [](const FileInfoRequestPtr request) { return !request->IsInProgress(); });
			m_fileInfoRequestQueue.insert(it, std::make_shared<FileInfoRequest>(filepath));
		}
		else
		{
			m_fileInfoRequestQueue.emplace_back(std::make_shared<FileInfoRequest>(filepath));
		}
		return true;
	}

	ThumbnailPtr FileInfoCache::GetLargestValidThumbnail(const FileInfo& fileInfo, size_t width, size_t height)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		ThumbnailPtr largestValidThumbnail;
		size_t largestSize = 0;
		for (size_t i = 0; i < fileInfo.GetThumbnailCount(); i++)
		{
			ThumbnailPtr thumbnail = fileInfo.GetThumbnail(i);
			if (thumbnail == nullptr)
				continue;

			if ((ThumbnailIsValid(*thumbnail)) && (thumbnail->meta.width <= width) &&
				(thumbnail->meta.height <= height) && (thumbnail->meta.width * thumbnail->meta.height > largestSize))
			{
				largestValidThumbnail = thumbnail;
				largestSize = thumbnail->meta.width * thumbnail->meta.height;
			}
		}
		return largestValidThumbnail;
	}

	bool FileInfoCache::QueueThumbnailRequest(const std::string& filepath, bool next)
	{
		ZoneScoped;
		return QueueThumbnailRequestInner(filepath, MAX_THUMBNAIL_CACHE_PIXELS, MAX_THUMBNAIL_CACHE_PIXELS, next);
	}

	bool FileInfoCache::QueueLargeThumbnailRequest(const std::string& filepath)
	{
		ZoneScoped;
		return QueueThumbnailRequestInner(filepath, 400, 400, true);
	}

	bool FileInfoCache::QueueThumbnailRequestInner(const std::string& filepath, size_t width, size_t height, bool next)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		LOG_DBG("Attempting to queue thumbnail request for {:s}, max size {:d}x{:d}", filepath.c_str(), width, height);
		for (auto it = m_thumbnailRequestQueue.begin(); it != m_thumbnailRequestQueue.end(); it++)
		{
			ThumbnailRequestPtr request = *it;

			if (request->GetData()->filename.Equals(filepath.c_str()))
			{
				if (request->IsInProgress())
				{
					// Request already in progress so don't remove it from queue or add it again
					LOG_DBG("Thumbnail request for {:s} already in progress", filepath.c_str());
					return false;
				}

				if (!next && !request->IsFailed())
				{
					// Request is already in the queue but has not started
					LOG_DBG("Thumbnail request for {:s} already queued", filepath.c_str());
					return false;
				}

				// Request in the queue but not in progress or at the front
				m_thumbnailRequestQueue.remove(request);
				std::filesystem::remove(
					GetThumbnailPath(filepath, true)); // Remove any temp thumbnail file (none should exist)
				break;
			}
		}

		FileInfoPtr fileInfo = GetFileInfo(filepath);
		if (fileInfo == nullptr)
		{
			LOG_DBG("No file info found for {:s}", filepath.c_str());
			FileInfoRequestPtr request = GetFileInfoRequest(filepath);
			if (request == nullptr)
			{
				LOG_WARN("Request not in progress for \"{:s}\", queuing file info request", filepath.c_str());
				QueueFileInfoRequest(filepath, next);
				return false;
			}

			if (!request->IsReceiving())
			{
				LOG_WARN("FileInfo request for \"{:s}\" has not been received yet, not queuing thumbnail request",
						 filepath.c_str());
				return false;
			}

			fileInfo = request->GetData();
		}

		ThumbnailPtr largestValidThumbnail = GetLargestValidThumbnail(*fileInfo, width, height);
		if (largestValidThumbnail == nullptr)
		{
			LOG_DBG("No valid thumbnail found for {:s}", filepath.c_str());
			return false;
		}
		return QueueThumbnailRequestInner(largestValidThumbnail, next);
	}

	bool FileInfoCache::QueueThumbnailRequestInner(const ThumbnailPtr& thumbnail, bool next)
	{
		ZoneScoped;
		if (thumbnail == nullptr)
		{
			return false;
		}

		thumbnail->context.Init();
		if (next)
		{
			auto it = std::find_if(m_thumbnailRequestQueue.begin(),
								   m_thumbnailRequestQueue.end(),
								   [](const ThumbnailRequestPtr request) { return !request->IsInProgress(); });
			m_thumbnailRequestQueue.insert(it, std::make_shared<ThumbnailRequest>(thumbnail));
		}
		else
		{
			m_thumbnailRequestQueue.emplace_back(std::make_shared<ThumbnailRequest>(thumbnail));
		}
		LOG_DBG("Queued thumbnail request for \"{:s}\", {:d}x{:d}",
				thumbnail->filename.c_str(),
				thumbnail->meta.width,
				thumbnail->meta.height);
		return true;
	}

#if 1
	ThumbnailPtr FileInfoCache::GetRequestedThumbnail(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		auto it = std::find_if(m_thumbnailRequestQueue.begin(),
							   m_thumbnailRequestQueue.end(),
							   [filepath](const ThumbnailRequestPtr request)
							   { return request->GetData()->filename.Equals(filepath.c_str()); });
		if (it != m_thumbnailRequestQueue.end())
		{
			ThumbnailRequestPtr request = *it;
			return request->GetData();
		}
		return nullptr;
	}
#endif

	bool FileInfoCache::IsThumbnailRequestInProgress()
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		return std::find_if(m_thumbnailRequestQueue.begin(),
							m_thumbnailRequestQueue.end(),
							[](const ThumbnailRequestPtr request)
							{ return request->IsInProgress(); }) != m_thumbnailRequestQueue.end();
	}

	void FileInfoCache::ThumbnailRequestComplete(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		ThumbnailRequestPtr request = GetThumbnailRequest(filepath);
		if (request == nullptr)
		{
			return;
		}

		auto temp_file = GetThumbnailPath(filepath, true);
		auto cached_file = GetThumbnailPath(filepath, false);

		if (temp_file != cached_file && std::filesystem::exists(temp_file))
		{
			std::filesystem::create_directories(cached_file.parent_path());
			std::filesystem::remove(cached_file);
			std::filesystem::copy(temp_file, cached_file);
			std::filesystem::remove(temp_file);
			LOG_DBG("Moved thumbnail from {:s} to {:s}", temp_file.string(), cached_file.string());
		}

		request->Complete();
		m_thumbnailRequestQueue.remove(request);
	}

	FileInfoCache::ThumbnailRequestPtr FileInfoCache::GetThumbnailRequest(const std::string& filepath)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		auto it = std::find_if(m_thumbnailRequestQueue.begin(),
							   m_thumbnailRequestQueue.end(),
							   [&filepath](const ThumbnailRequestPtr& request)
							   { return request->GetData()->filename.Equals(filepath.c_str()); });

		if (it == m_thumbnailRequestQueue.end())
		{
			return nullptr;
		}
		return *it;
	}

	/**
	 * @brief Stops the current thumbnail request. Will not stop a thumbnail request if it is for the current print job.
	 * @param largeOnly If true, only stops the request if the current thumbnail is above the cache limit.
	 * @return True if the thumbnail request was stopped, false otherwise.
	 */
	bool FileInfoCache::StopThumbnailRequest(bool largeOnly)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		UNUSED(largeOnly);
		return true;
	}

	void FileInfoCache::Debug()
	{
		std::lock_guard<LockableBase(std::recursive_mutex)> lock(s_mutex);
		LOG_DBG("File info cache debug");
		LOG_INFO("File info cache:");
		for (auto& it : get()->m_cache)
		{
			FileInfoPtr fileInfo = it.second;
			if (fileInfo == nullptr)
				continue;
			LOG_INFO("  File {:s}:", fileInfo->filename.c_str());
			LOG_INFO("    size: {:d}", fileInfo->size);
			LOG_INFO("    lastModified: {:s}", fileInfo->lastModified.c_str());
			LOG_INFO("    height: {:0.3f}", fileInfo->height);
			LOG_INFO("    layerHeight: {:0.3f}", fileInfo->layerHeight);
			LOG_INFO("    thumbnails: {:d}", fileInfo->GetThumbnailCount());

			for (size_t i = 0; i < fileInfo->GetThumbnailCount(); i++)
			{
				ThumbnailPtr thumbnail = fileInfo->GetThumbnail(i);
				if (thumbnail == nullptr)
					continue;
				LOG_INFO("    Thumbnail {:d}:", i);
				LOG_INFO("      meta:");
				LOG_INFO("        filename: {:s}", thumbnail->filename.c_str());
				LOG_INFO("        width({:d}), height({:d}), format({:s}), offset({:d}), size({:d})",
						 thumbnail->meta.width,
						 thumbnail->meta.height,
						 nameof::nameof_enum(thumbnail->meta.imageFormat),
						 thumbnail->meta.offset,
						 thumbnail->meta.size);
				LOG_INFO("      context:");
				LOG_INFO("        err({:d}), parseErr({:s}), size({:d}), offset({:d}), next({:d}), state({:s})",
						 thumbnail->context.err,
						 nameof::nameof_enum(thumbnail->context.parseErr),
						 thumbnail->context.size,
						 thumbnail->context.offset,
						 thumbnail->context.next,
						 nameof::nameof_enum(thumbnail->context.state));
			}
		}

		LOG_INFO("  File info request queue:");
		for (FileInfoRequestPtr request : m_fileInfoRequestQueue)
		{
			LOG_INFO("    {:s}", request->GetData()->filename.c_str());
		}

		LOG_INFO("  Thumbnail request queue:");
		for (ThumbnailRequestPtr request : m_thumbnailRequestQueue)
		{
			LOG_INFO("    {:d}x{:d} {:s}",
					 request->GetData()->meta.width,
					 request->GetData()->meta.height,
					 request->GetData()->filename.c_str());
		}
	}

	tm ParseSeconds(uint32_t seconds)
	{
		ZoneScoped;
		tm time;
		time.tm_hour = static_cast<int>(seconds / 3600);
		time.tm_min = static_cast<int>((seconds - time.tm_hour * 3600) / 60);
		time.tm_sec = static_cast<int>(seconds - time.tm_hour * 3600 - time.tm_min * 60);
		return time;
	}

	static Debug::DebugCommand s_dbgFileInfoCache("dbg_file_info_cache", []() { FileInfoCache::get()->Debug(); });
} // namespace Comm

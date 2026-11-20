/*
 * FileInfo.h
 *
 *  Created on: 12 Mar 2024
 *      Author: andy
 */

#ifndef JNI_COMM_FILEINFO_H_
#define JNI_COMM_FILEINFO_H_

#include "Comm/Replies.h"
#include "Thumbnail.h"

#include "Configuration.h"
#include "Duet3D/General/String.h"
#include "Duet3D/General/StringRef.h"
#include "utils/TimeHelper.h"
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <unordered_map>
#include <vector>

namespace Comm
{
	using ThumbnailPtr = std::shared_ptr<Thumbnail>;

	struct FileInfo
	{
	  public:
		FileInfo();
		~FileInfo();

		String<MAX_FILENAME_LENGTH> filename; // full path of the file on the Duet
		std::vector<float> filament;
		String<64> generatedBy;
		float height = 0;
		String<19> lastModified;
		float layerHeight = 0;
		uint32_t numLayers = 0;
		uint32_t printTime = 0;
		uint32_t simulatedTime = 0;
		uint32_t size = 0;

		ThumbnailPtr GetThumbnail(size_t index) const;
		ThumbnailPtr GetOrCreateThumbnail(size_t index);
		size_t GetThumbnailCount() const { return m_thumbnails.size(); }
		size_t ClearThumbnails(size_t fromIndex);
		const auto& GetThumbnails() const { return m_thumbnails; }

		tm GetPrintTime() const;

	  private:
		std::vector<ThumbnailPtr> m_thumbnails;
	};

	void to_json(nlohmann::json& j, const Comm::FileInfo& c);
	void from_json(const nlohmann::json& j, Comm::FileInfo& c);

	using FileInfoPtr = std::shared_ptr<FileInfo>;

	enum class RequestState
	{
		UNKNOWN = 0,
		QUEUED,
		REQUESTED,
		RECEIVING,
		COMPLETE,
		FAILED
	};

	class FileInfoCache
	{
	  public:
		template <typename T>
		struct Request
		{

			Request()
				: m_data(std::make_shared<T>())
			{
			}

			Request(const std::shared_ptr<T>& data)
				: m_data(data)
			{
			}

			virtual ~Request() = default;
			Request(const Request& other) = default;
			Request& operator=(const Request& other) = default;
			Request(Request&& other) noexcept = default;
			Request& operator=(Request&& other) noexcept = default;

			std::shared_ptr<T> GetData() const { return m_data; }
			RequestState GetState() const { return m_state; }

			bool IsRequested() const { return m_state >= RequestState::REQUESTED; }
			bool IsInProgress() const
			{
				return m_state == RequestState::REQUESTED || m_state == RequestState::RECEIVING;
			}
			bool IsReceiving() const { return m_state == RequestState::RECEIVING; }
			bool IsComplete() const { return m_state == RequestState::COMPLETE || m_state == RequestState::FAILED; }
			bool IsFailed() const { return m_state == RequestState::FAILED; }

			void Receiving()
			{
				m_receiveTime = TimeHelper::getRunningTime();
				m_state = RequestState::RECEIVING;
			}
			void Complete(bool failed = false) { m_state = failed ? RequestState::FAILED : RequestState::COMPLETE; }

			std::chrono::milliseconds GetRequestTime() const { return m_requestTime; }
			bool HasTimedOut(std::chrono::milliseconds timeout) const
			{
				const bool requestTimedOut =
					m_state == RequestState::REQUESTED && TimeHelper::getTimeSince(m_requestTime) > timeout;
				const bool receiveTimedOut =
					m_state == RequestState::RECEIVING && TimeHelper::getTimeSince(m_receiveTime) > timeout;
				return requestTimedOut || receiveTimedOut;
			}

			bool RequestData()
			{
				m_requestTime = TimeHelper::getRunningTime();
				if (!RequestDataInner())
				{
					return false;
				}
				m_state = RequestState::REQUESTED;
				return true;
			}

			bool operator==(const Request& other) { return m_data == other.m_data; }

		  protected:
			virtual bool RequestDataInner() = 0;
			void SetState(RequestState state) { m_state = state; }

			std::shared_ptr<T> m_data;
			RequestState m_state = RequestState::UNKNOWN;
			std::chrono::milliseconds m_requestTime = 0ms;
			std::chrono::milliseconds m_receiveTime = 0ms;
		};

		struct FileInfoRequest : public Request<FileInfo>
		{
			FileInfoRequest(const std::string& filepath)
				: Request<FileInfo>()
			{
				m_data->filename.copy(filepath.c_str());
			}

			virtual ~FileInfoRequest() = default;
			FileInfoRequest(const FileInfoRequest& other) = default;
			FileInfoRequest& operator=(const FileInfoRequest& other) = default;
			FileInfoRequest(FileInfoRequest&& other) noexcept = default;
			FileInfoRequest& operator=(FileInfoRequest&& other) noexcept = default;

		  protected:
			bool RequestDataInner() override;
		};

		struct ThumbnailRequest : public Request<Thumbnail>
		{
			using Request<Thumbnail>::Request;

			virtual ~ThumbnailRequest() = default;
			ThumbnailRequest(const ThumbnailRequest& other) = default;
			ThumbnailRequest& operator=(const ThumbnailRequest& other) = default;
			ThumbnailRequest(ThumbnailRequest&& other) noexcept = default;
			ThumbnailRequest& operator=(ThumbnailRequest&& other) noexcept = default;

		  protected:
			bool RequestDataInner() override;
		};

		using FileInfoRequestPtr = std::shared_ptr<FileInfoRequest>;
		using ThumbnailRequestPtr = std::shared_ptr<ThumbnailRequest>;

		static FileInfoCache* get()
		{
			static FileInfoCache instance;
			return &instance;
		}

		std::optional<std::filesystem::path> GetCachePath() const;
		void LoadCacheFromMemory();
		bool SaveCache();

		void Spin(); // processes thumbnail requests

		bool IsThumbnailCached(const std::string& filepath,
							   const char* lastModified); // returns true if a thumbnail for the file is in cache
		// and lastModified is the same

		FileInfoPtr GetFileInfo(const std::string& filepath);
		/**
		 * The firmware's answer to a file info request (M36, rr_fileinfo): the file's information, kept, and its
		 * thumbnail asked for; or why there is none, which ends the request
		 * @param filepath The file asked about
		 */
		void FileInfoArrived(const std::string& filepath, const std::expected<FileInfoReply, ReplyError>& reply);
		/**
		 * The firmware's answer to a thumbnail request (M36.1, rr_thumbnail): the next piece of the thumbnail, decoded,
		 * and the one after it asked for; or why there is none, which ends the request
		 * @param filepath The file whose thumbnail was asked for
		 */
		void ThumbnailArrived(const std::string& filepath, const std::expected<FileChunk, ReplyError>& reply);
		FileInfoRequestPtr GetFileInfoRequest(const std::string& filepath);
		void FileInfoRequestComplete(const std::string& filepath); // called when the file info request is complete

		ThumbnailPtr GetRequestedThumbnail(const std::string& filepath);
		bool CancelThumbnailRequest(const std::string& filepath);
		ThumbnailRequestPtr GetThumbnailRequest(const std::string& filepath);
		void ThumbnailRequestComplete(const std::string& filepath);

		void ClearCache(); // clears the cache

		bool QueueFileInfoRequest(const std::string& filepath, bool next = false);
		bool QueueThumbnailRequest(const std::string& filepath,
								   bool next = false);				  // returns true if the request was queued
		bool QueueLargeThumbnailRequest(const std::string& filepath); // returns true if a thumbnail request was started

		bool StopThumbnailRequest(bool largeOnly = false);

		void Debug(); // prints debug info

	  private:
		FileInfoCache();

		void LoadCacheFolder(const std::filesystem::path& folderPath);
		bool LoadFileInfoFromFile(const std::filesystem::path& path, FileInfo& fileInfo);
		bool SaveCacheToFile(const FileInfo& fileInfo);

		bool IsFileInfoRequestInProgress();
		bool IsThumbnailRequestInProgress();

		bool IsFileInfoRequestQueued(const std::string& filepath);
		bool IsFileInfoRequestInProgress(const std::string& filepath);

		bool IsThumbnailRequestQueued(const std::string& filepath);
		bool IsThumbnailRequestInProgress(const std::string& filepath);

		ThumbnailPtr GetLargestValidThumbnail(const FileInfo& fileInfo, size_t width, size_t height);
		bool QueueThumbnailRequestInner(const std::string& filepath, size_t width, size_t height, bool next);
		bool QueueThumbnailRequestInner(const ThumbnailPtr& thumbnail,
										bool next = false); // returns true if the request was queued

		std::unordered_map<std::string, FileInfoPtr> m_cache; // cache of file path and their associated file info
		std::list<FileInfoRequestPtr> m_fileInfoRequestQueue;
		std::list<ThumbnailRequestPtr> m_thumbnailRequestQueue;
		std::chrono::milliseconds m_lastRequestTime = 0ms;
	};

	tm ParseSeconds(uint32_t seconds);
} // namespace Comm
#define FILEINFO_CACHE Comm::FileInfoCache::get()

#endif /* JNI_COMM_FILEINFO_H_ */

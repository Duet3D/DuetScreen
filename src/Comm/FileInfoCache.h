/*
 * FileInfoCache.h
 *
 * What the screen knows about the printer's G-code files: their information (M36, rr_fileinfo, DSF's file info:
 * Comm::FileInfo) and their thumbnails, asked for as they are needed and kept, per printer, so they aren't asked for
 * again while the files don't change.
 *
 *   - refresh(): a file listed (Files.h) with its date; its information is asked for if it isn't known or the file
 *     has changed since, and then its small thumbnail if there isn't one kept
 *   - requestThumbnail(): a thumbnail wanted now (the job running, the file chosen), ahead of the rest
 *
 * One request is made at a time, the next as soon as the last is answered (a little later while printing, so the
 * printer isn't kept busy); every request is answered, if only with why there is no answer (Connection.h), so nothing
 * here waits or times out by itself. FileInfo and ThumbnailData events say what has arrived.
 *
 * Kept per printer (its board's unique id), in NVS_FOLDER/fileinfo_cache/<id>/: each file's information as JSON,
 * written as RepRapFirmware sends it (read back with FileInfo::read()), and its thumbnail, a PNG or BMP, beside it.
 * A thumbnail being fetched is written to a temporary folder and moved there once whole.
 */

#pragma once

#include "Comm/Replies.h"
#include "tracy/Tracy.hpp"
#include <chrono>
#include <cstdint>
#include <deque>
#include <expected>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Comm
{
	class ThumbnailWriter;

	class FileInfoCache
	{
	  public:
		/** Which of a file's thumbnails is wanted */
		enum class ThumbnailSize
		{
			small, // The largest no bigger than MAX_THUMBNAIL_CACHE_PIXELS square: for lists
			large, // The largest no bigger than largeThumbnailPixels square: for a job's status, a file chosen
		};

		/** The biggest a large thumbnail may be, pixels square */
		static constexpr uint32_t largeThumbnailPixels = 400;

		/** The cache */
		static FileInfoCache& get();

		/** What is known about `path` (a full path, e.g. "0:/gcodes/a.gcode"), if anything */
		std::optional<FileInfo> find(const std::string& path) const;

		/**
		 * A file has been listed: ask for its information if it isn't known, or is older than `lastModified`, then for
		 * its small thumbnail if there isn't one kept
		 */
		void refresh(const std::string& path, std::string_view lastModified);

		/** Ask for `path`'s thumbnail of `size` now, ahead of everything else waiting */
		void requestThumbnail(const std::string& path, ThumbnailSize size);

		/** Whether a thumbnail of `path` is kept */
		bool hasThumbnail(const std::string& path) const;

		/** Where `path`'s thumbnail is kept (there may be none yet) */
		std::filesystem::path thumbnailPath(const std::string& path) const;

		/** The printer has gone: forget what is waiting, and what is known (what is kept stays, for when it's back) */
		void forget();

		/** Forget everything, what is kept included */
		void clear();

		/** Keep files under `kept` (one folder per printer) and fetch thumbnails into `temporary` (tests) */
		void setFolders(std::filesystem::path kept, std::filesystem::path temporary);

		/** Log what is known */
		void log() const;

	  private:
		/** Something to ask the printer */
		struct Job
		{
			enum class Kind
			{
				info,	   // The file's information
				thumbnail, // One of its thumbnails
			};
			Kind kind = Kind::info;					   // What is asked for
			std::string path;						   // The file
			ThumbnailSize size = ThumbnailSize::small; // Which thumbnail
			bool infoAsked = false;					   // For a thumbnail: its file's information was asked for first
		};

		FileInfoCache();

		/** Load what is kept about the printer now connected */
		void load();

		/** The folder kept for the printer now connected, if it is known */
		std::optional<std::filesystem::path> printerFolder() const;

		/** `path` as a path under a folder: ':' can't be in a file name everywhere */
		static std::string relative(const std::string& path);

		/** Whether a job like `job` is waiting or running (the lock held) */
		bool queued(Job::Kind kind, const std::string& path) const;

		/** Start the next job, unless one is running (or, while printing, it isn't time yet) */
		void startNext();

		/** Ask the printer what `job` wants */
		void run(const Job& job);

		/** The information `job` asked for arrived, or didn't */
		void infoArrived(const Job& job, const std::expected<FileInfo, ReplyError>& info);

		/** Ask for the piece of the thumbnail `job` wants at `offset`, adding it to `writer` until it is whole */
		void fetchPiece(const Job& job,
						const ThumbnailInfo& thumbnail,
						std::shared_ptr<ThumbnailWriter> writer,
						uint32_t offset);

		/** The running job is done: start the next */
		void jobDone();

		/** Keep `info` with the printer's files */
		void save(const FileInfo& info) const;

		mutable TracyLockable(std::mutex, m_mutex);			 // Guards everything below
		std::unordered_map<std::string, FileInfo> m_infos;	 // What is known, by full path
		std::deque<Job> m_jobs;								 // Waiting, next first
		std::optional<Job> m_running;						 // Asked for and not yet answered
		bool m_startScheduled = false;						 // startNext() is due later (while printing)
		std::chrono::steady_clock::time_point m_lastStarted; // When the last job started
		uint64_t m_generation = 0;			   // Counts forget()s: answers to what was forgotten are dropped
		std::filesystem::path m_keptRoot;	   // Where each printer's folder is
		std::filesystem::path m_temporaryRoot; // Where thumbnails are fetched into
	};
} // namespace Comm

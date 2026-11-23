/*
 * FileInfoCache.cpp
 */

#include "FileInfoCache.h"
#include "Comm/Connection.h"
#include "Comm/ThumbnailWriter.h"
#include "Configuration.h"
#include "Debug.h"
#include "DebugCommands.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Json.h"
#include "UI/Core/Model.h"
#include "utils/utils.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <utility>

namespace Comm
{
	namespace
	{
		/** The biggest a thumbnail of `size` may be, pixels square */
		uint32_t limitOf(FileInfoCache::ThumbnailSize size)
		{
			return size == FileInfoCache::ThumbnailSize::small ? static_cast<uint32_t>(MAX_THUMBNAIL_CACHE_PIXELS)
															   : FileInfoCache::largeThumbnailPixels;
		}

		/** Whether `thumbnail` is no bigger than `size` allows */
		bool fits(const ThumbnailInfo& thumbnail, FileInfoCache::ThumbnailSize size)
		{
			return thumbnail.width <= limitOf(size) && thumbnail.height <= limitOf(size);
		}

		/**
		 * The thumbnails of `info` that can be shown, nearest to `size` first: those that fit, the largest first, then
		 * those too big, the smallest first
		 */
		std::vector<ThumbnailInfo> ranked(const FileInfo& info, FileInfoCache::ThumbnailSize size)
		{
			std::vector<ThumbnailInfo> thumbnails;
			std::ranges::copy_if(info.thumbnails, std::back_inserter(thumbnails), &ThumbnailWriter::canWrite);
			std::ranges::sort(thumbnails,
							  {},
							  [size](const ThumbnailInfo& thumbnail)
							  {
								  const auto area = static_cast<int64_t>(thumbnail.width) * thumbnail.height;
								  return fits(thumbnail, size) ? std::pair(0, -area) : std::pair(1, area);
							  });
			return thumbnails;
		}

		/** The thumbnail of `info` to fetch for `size`: the largest that fits, of a format that can be written */
		std::optional<ThumbnailInfo> choose(const FileInfo& info, FileInfoCache::ThumbnailSize size)
		{
			const std::vector<ThumbnailInfo> thumbnails = ranked(info, size);
			if (thumbnails.empty() || !fits(thumbnails.front(), size))
			{
				return std::nullopt;
			}
			return thumbnails.front();
		}

		/** `lastModified` ("2026-06-17T09:30:10") as part of a file name: its digits ("20260617093010") */
		std::string version(std::string_view lastModified)
		{
			std::string digits;
			std::ranges::copy_if(lastModified, std::back_inserter(digits), [](char c) { return c >= '0' && c <= '9'; });
			return digits;
		}

		/** `info` as RepRapFirmware sends it, so it is read back with FileInfo::read() */
		nlohmann::ordered_json toJson(const FileInfo& info)
		{
			nlohmann::ordered_json thumbnails = nlohmann::ordered_json::array();
			for (const ThumbnailInfo& thumbnail : info.thumbnails)
			{
				thumbnails.push_back({{"format", thumbnail.format},
									  {"width", thumbnail.width},
									  {"height", thumbnail.height},
									  {"offset", thumbnail.offset},
									  {"size", thumbnail.size}});
			}
			return {{"err", 0},
					{"fileName", info.fileName},
					{"size", info.size},
					{"lastModified", info.lastModified},
					{"height", info.height},
					{"layerHeight", info.layerHeight},
					{"numLayers", info.numLayers},
					{"printTime", info.printTime},
					{"simulatedTime", info.simulatedTime},
					{"filament", info.filament},
					{"generatedBy", info.generatedBy},
					{"thumbnails", thumbnails}};
		}

		/** What is kept in `folder`: each file's information, by full path */
		std::unordered_map<std::string, FileInfo> readKept(const std::filesystem::path& folder)
		{
			ZoneScoped;
			std::unordered_map<std::string, FileInfo> infos;
			std::error_code error;
			for (auto it = std::filesystem::recursive_directory_iterator(folder, error);
				 !error && it != std::filesystem::recursive_directory_iterator();
				 it.increment(error))
			{
				if (!it->is_regular_file() || it->path().extension() != ".json")
				{
					continue;
				}
				std::ifstream file(it->path());
				const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				auto info = OM::parseJson(text)
								.transform_error([](const OM::JsonError& error)
												 { return ReplyError{.message = error.message}; })
								.and_then([](const OM::Node& json) { return FileInfo::read(json); });
				if (info)
				{
					infos.emplace(info->fileName, std::move(*info));
				}
				else
				{
					LOG_WARN("Ignoring {:s}: {:s}", it->path().string(), info.error().message);
				}
			}
			return infos;
		}

		/** Move `from` to `to`, replacing it; they may be on different file systems (/tmp and the kept folder) */
		bool move(const std::filesystem::path& from, const std::filesystem::path& to)
		{
			std::error_code error;
			std::filesystem::create_directories(to.parent_path(), error);
			std::filesystem::rename(from, to, error);
			if (error)
			{
				error.clear();
				std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, error);
				std::filesystem::remove(from);
			}
			return !error;
		}

		/** Log what the cache knows */
		const Debug::DebugCommand s_logCommand("dbg_file_info_cache", [] { FileInfoCache::get().log(); });
	} // namespace

	FileInfoCache& FileInfoCache::get()
	{
		static FileInfoCache s_cache;
		return s_cache;
	}

	FileInfoCache::FileInfoCache()
		: m_keptRoot(std::filesystem::path(NVS_FOLDER) / "fileinfo_cache")
		, m_temporaryRoot("/tmp/thumbnails")
	{
	}

	std::optional<FileInfo> FileInfoCache::find(const std::string& path)
	{
		ZoneScoped;
		loadPrinter();
		std::lock_guard lock(m_mutex);
		const auto it = m_infos.find(path);
		return it != m_infos.end() ? std::optional(it->second) : std::nullopt;
	}

	void FileInfoCache::refresh(const std::string& path, std::string_view lastModified)
	{
		ZoneScoped;
		loadPrinter();
		{
			std::lock_guard lock(m_mutex);
			const auto it = m_infos.find(path);
			if (it != m_infos.end() && it->second.lastModified == lastModified)
			{
				if (wanted(it->second, ThumbnailSize::small) && !queued(Job::Kind::thumbnail, path))
				{
					m_jobs.push_back({.kind = Job::Kind::thumbnail, .path = path, .size = ThumbnailSize::small});
				}
			}
			else if (!queued(Job::Kind::info, path))
			{
				m_jobs.push_back({.kind = Job::Kind::info, .path = path, .size = ThumbnailSize::small});
			}
		}
		startNext();
	}

	void FileInfoCache::requestThumbnail(const std::string& path, ThumbnailSize size)
	{
		ZoneScoped;
		loadPrinter();
		{
			std::lock_guard lock(m_mutex);
			if (const auto it = m_infos.find(path); it != m_infos.end() && !wanted(it->second, size))
			{
				return; // kept, or not to be had
			}
			if (m_running && m_running->kind == Job::Kind::thumbnail && m_running->path == path &&
				m_running->size == size)
			{
				return; // on its way
			}
			std::erase_if(m_jobs,
						  [&](const Job& job)
						  { return job.kind == Job::Kind::thumbnail && job.path == path && job.size == size; });
			m_jobs.push_front({.kind = Job::Kind::thumbnail, .path = path, .size = size});
		}
		startNext();
	}

	std::optional<std::filesystem::path> FileInfoCache::thumbnail(const std::string& path, ThumbnailSize size)
	{
		ZoneScoped;
		const std::optional<FileInfo> info = find(path);
		if (!info)
		{
			return std::nullopt;
		}
		for (const ThumbnailInfo& thumbnail : ranked(*info, size))
		{
			std::filesystem::path image = imagePath(*info, thumbnail);
			if (kept(image))
			{
				return image;
			}
		}
		return std::nullopt;
	}

	void FileInfoCache::forget()
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_mutex);
			m_generation++;
			m_jobs.clear();
			m_running.reset();
			m_infos.clear();
			m_loadedFor.clear();
			m_failed.clear();
		}
		std::error_code error;
		std::filesystem::remove_all(m_temporaryRoot, error);
	}

	void FileInfoCache::clear()
	{
		ZoneScoped;
		forget();
		std::error_code error;
		std::filesystem::remove_all(m_keptRoot, error);
	}

	void FileInfoCache::setFolders(std::filesystem::path kept, std::filesystem::path temporary)
	{
		std::lock_guard lock(m_mutex);
		m_keptRoot = std::move(kept);
		m_temporaryRoot = std::move(temporary);
	}

	void FileInfoCache::log()
	{
		loadPrinter();
		std::lock_guard lock(m_mutex);
		LOG_INFO("File info cache: {:d} files known, {:d} requests waiting{:s}",
				 m_infos.size(),
				 m_jobs.size(),
				 m_running ? ", one running" : "");
		for (const auto& [path, info] : m_infos)
		{
			const auto images = std::ranges::count_if(
				info.thumbnails, [&](const ThumbnailInfo& thumbnail) { return kept(imagePath(info, thumbnail)); });
			LOG_INFO("  {:s}: {:s}, {:d} bytes, {:d} s, {:d} thumbnails, {:d} kept",
					 path,
					 info.lastModified,
					 info.size,
					 info.printTime,
					 info.thumbnails.size(),
					 images);
		}
	}

	void FileInfoCache::loadPrinter()
	{
		const std::string id = OM::GetPrinterUniqueId();
		std::filesystem::path folder;
		{
			std::lock_guard lock(m_mutex);
			if (id == m_loadedFor)
			{
				return;
			}
			folder = m_keptRoot / id;
		}
		ZoneScoped;
		std::unordered_map<std::string, FileInfo> infos;
		if (!id.empty())
		{
			infos = readKept(folder);
			LOG_INFO("Loaded what is known about {:d} files", infos.size());
		}
		std::lock_guard lock(m_mutex);
		if (id != m_loadedFor)
		{
			m_infos.merge(infos); // what arrived meanwhile stays: it is newer than what was kept
			m_loadedFor = id;
		}
	}

	std::optional<std::filesystem::path> FileInfoCache::printerFolder() const
	{
		const std::string& id = OM::GetPrinterUniqueId();
		if (id.empty())
		{
			return std::nullopt;
		}
		return m_keptRoot / id;
	}

	std::filesystem::path FileInfoCache::imageFolder() const
	{
		return printerFolder().value_or(m_temporaryRoot);
	}

	std::string FileInfoCache::relative(const std::string& path)
	{
		std::string relative = path;
		utils::replaceSubstring(relative, ":", "\\%3A");
		return relative;
	}

	std::filesystem::path FileInfoCache::imagePath(const FileInfo& info, const ThumbnailInfo& thumbnail) const
	{
		// No .png or .bmp: LVGL's BMP decoder would take a .bmp, and it can't scale; FFmpeg's (lv_conf.h) and libpng's
		// tell the format from what is in the file
		return imageFolder() / fmt::format("{:s}.{:s}.{:d}x{:d}",
										   relative(info.fileName),
										   version(info.lastModified),
										   thumbnail.width,
										   thumbnail.height);
	}

	bool FileInfoCache::kept(const std::filesystem::path& image)
	{
		struct stat status;
		return stat(image.c_str(), &status) == 0 && S_ISREG(status.st_mode) && status.st_size > 0;
	}

	bool FileInfoCache::wanted(const FileInfo& info, ThumbnailSize size) const
	{
		const std::optional<ThumbnailInfo> thumbnail = choose(info, size);
		if (!thumbnail)
		{
			return false;
		}
		const std::filesystem::path image = imagePath(info, *thumbnail);
		return !kept(image) && !m_failed.contains(image.string());
	}

	bool FileInfoCache::queued(Job::Kind kind, const std::string& path) const
	{
		const auto same = [&](const Job& job) { return job.kind == kind && job.path == path; };
		return (m_running && same(*m_running)) || std::ranges::any_of(m_jobs, same);
	}

	void FileInfoCache::startNext()
	{
		ZoneScoped;
		const bool printing = OM::PrintInProgress(); // read before taking the lock: it reads the object model
		Job job;
		{
			std::lock_guard lock(m_mutex);
			if (m_running || m_jobs.empty() || m_startScheduled)
			{
				return;
			}
			const auto sinceLast = std::chrono::steady_clock::now() - m_lastStarted;
			if (printing && sinceLast < BACKGROUND_FILE_CACHE_POLL_INTERVAL)
			{
				// While printing, a request now and then, so the printer isn't kept busy
				m_startScheduled = true;
				Connection::get().after(std::chrono::duration_cast<std::chrono::milliseconds>(
											BACKGROUND_FILE_CACHE_POLL_INTERVAL - sinceLast),
										[this]
										{
											ZoneScopedN("startNext_cb");
											{
												std::lock_guard lock(m_mutex);
												m_startScheduled = false;
											}
											startNext();
										});
				return;
			}
			job = std::move(m_jobs.front());
			m_jobs.pop_front();
			m_running = job;
			m_lastStarted = std::chrono::steady_clock::now();
		}
		run(job);
	}

	void FileInfoCache::run(const Job& job)
	{
		ZoneScoped;
		ZoneText(job.path.data(), job.path.size());
		uint64_t generation = 0;
		{
			std::lock_guard lock(m_mutex);
			generation = m_generation;
		}
		if (job.kind == Job::Kind::info)
		{
			Connection::get().fileInfo(job.path,
									   [this, job, generation](std::expected<FileInfo, ReplyError> info)
									   {
										   ZoneScopedN("fileInfo_cb");
										   std::unique_lock lock(m_mutex);
										   if (generation != m_generation)
										   {
											   return; // forgotten
										   }
										   lock.unlock();
										   infoArrived(job, info);
									   });
			return;
		}

		const std::optional<FileInfo> info = find(job.path);
		if (!info)
		{
			if (!job.infoAsked)
			{
				// The thumbnails are in the file's information: ask for that first, then for the thumbnail again
				std::lock_guard lock(m_mutex);
				Job thumbnail = job;
				thumbnail.infoAsked = true;
				m_jobs.push_front(std::move(thumbnail));
				m_jobs.push_front({.kind = Job::Kind::info, .path = job.path, .size = job.size});
			}
			jobDone();
			return;
		}
		const std::optional<ThumbnailInfo> thumbnail = choose(*info, job.size);
		if (!thumbnail)
		{
			LOG_DBG("{:s} has no thumbnail the screen can show", job.path);
			jobDone();
			return;
		}
		auto fetch = std::make_shared<Fetch>(Fetch{.job = job,
												   .thumbnail = *thumbnail,
												   .temporary = m_temporaryRoot / (relative(job.path) + ".part"),
												   .image = imagePath(*info, *thumbnail),
												   .writer = std::make_shared<ThumbnailWriter>(),
												   .generation = generation});
		if (kept(fetch->image))
		{
			jobDone(); // fetched since it was asked for
			return;
		}
		if (!fetch->writer->open(*thumbnail, fetch->temporary))
		{
			LOG_WARN("Couldn't write the thumbnail of {:s} to {:s}", job.path, fetch->temporary.string());
			jobDone();
			return;
		}
		fetchPiece(std::move(fetch), thumbnail->offset);
	}

	void FileInfoCache::infoArrived(const Job& job, const std::expected<FileInfo, ReplyError>& reply)
	{
		ZoneScoped;
		if (!reply)
		{
			LOG_WARN("No file information for {:s}: {:s}", job.path, reply.error().message);
			jobDone();
			return;
		}
		FileInfo info = *reply;
		info.fileName = job.path;
		std::optional<FileInfo> previous;
		{
			std::lock_guard lock(m_mutex);
			if (const auto it = m_infos.find(job.path); it != m_infos.end())
			{
				previous = std::exchange(it->second, info);
			}
			else
			{
				m_infos.emplace(job.path, info);
			}
		}
		if (previous && previous->lastModified != info.lastModified)
		{
			// The file has changed: its old thumbnails won't be shown again (the new ones have other names)
			for (const ThumbnailInfo& thumbnail : previous->thumbnails)
			{
				std::error_code error;
				std::filesystem::remove(imagePath(*previous, thumbnail), error);
			}
		}
		save(info);
		Model::get().post<EventType::FileInfo>(job.path);
		{
			std::lock_guard lock(m_mutex);
			if (wanted(info, ThumbnailSize::small) && !queued(Job::Kind::thumbnail, job.path))
			{
				m_jobs.push_back({.kind = Job::Kind::thumbnail, .path = job.path, .size = ThumbnailSize::small});
			}
		}
		jobDone();
	}

	void FileInfoCache::fetchPiece(std::shared_ptr<Fetch> fetch, uint32_t offset)
	{
		ZoneScoped;
		const std::string path = fetch->job.path;
		Connection::get().thumbnail(path,
									offset,
									[this, fetch = std::move(fetch), offset](std::expected<FileChunk, ReplyError> piece)
									{
										ZoneScopedN("fetchPiece_cb");
										{
											std::lock_guard lock(m_mutex);
											if (fetch->generation != m_generation)
											{
												return; // forgotten
											}
										}
										const auto fail = [&](std::string_view why)
										{
											LOG_WARN("No thumbnail for {:s}: {:s}", fetch->job.path, why);
											fetch->writer->finish();
											std::error_code error;
											std::filesystem::remove(fetch->temporary, error);
											{
												std::lock_guard lock(m_mutex);
												m_failed.insert(fetch->image.string());
											}
											jobDone();
										};
										if (!piece)
										{
											fail(piece.error().message);
											return;
										}
										if (piece->offset != offset)
										{
											fail("a piece from somewhere else in the file came");
											return;
										}
										if (!fetch->writer->add(piece->data))
										{
											fail("a piece couldn't be decoded");
											return;
										}
										if (piece->next != 0)
										{
											fetchPiece(fetch, piece->next);
											return;
										}
										if (!fetch->writer->finish())
										{
											fail("it ended short");
											return;
										}
										if (!move(fetch->temporary, fetch->image))
										{
											fail("it couldn't be kept");
											return;
										}
										Model::get().post<EventType::ThumbnailData>(fetch->job.path);
										jobDone();
									});
	}

	void FileInfoCache::jobDone()
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_mutex);
			m_running.reset();
		}
		startNext();
	}

	void FileInfoCache::save(const FileInfo& info) const
	{
		ZoneScoped;
		const auto folder = printerFolder();
		if (!folder)
		{
			return; // the printer isn't known yet
		}
		const std::filesystem::path path = *folder / (relative(info.fileName) + ".json");
		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);
		std::ofstream file(path);
		if (!file)
		{
			LOG_ERROR("Couldn't keep the information about {:s} in {:s}", info.fileName, path.string());
			return;
		}
		file << toJson(info).dump(4);
	}
} // namespace Comm

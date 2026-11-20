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

namespace Comm
{
	namespace
	{
		/** The thumbnail of `info` to fetch for `size`: the largest that fits, of a format that can be written */
		std::optional<ThumbnailInfo> choose(const FileInfo& info, FileInfoCache::ThumbnailSize size)
		{
			const uint32_t limit = size == FileInfoCache::ThumbnailSize::small
									   ? static_cast<uint32_t>(MAX_THUMBNAIL_CACHE_PIXELS)
									   : FileInfoCache::largeThumbnailPixels;
			std::optional<ThumbnailInfo> chosen;
			for (const ThumbnailInfo& thumbnail : info.thumbnails)
			{
				if (ThumbnailWriter::canWrite(thumbnail) && thumbnail.width <= limit && thumbnail.height <= limit &&
					(!chosen || thumbnail.width * thumbnail.height > chosen->width * chosen->height))
				{
					chosen = thumbnail;
				}
			}
			return chosen;
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

		/** Move `from` to `to`, replacing it; they may be on different file systems (/tmp and the kept folder) */
		bool move(const std::filesystem::path& from, const std::filesystem::path& to)
		{
			std::error_code error;
			std::filesystem::create_directories(to.parent_path(), error);
			std::filesystem::remove(to, error);
			std::filesystem::rename(from, to, error);
			if (error)
			{
				error.clear();
				std::filesystem::copy_file(from, to, error);
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
		ZoneScoped;
		// Another printer has other files: what is kept about it is loaded once it is known
		Model::get().addEventListener<EventType::PrinterUniqueId>(
			[this]
			{
				ZoneScopedN("FileInfoCache_cb");
				load();
			});
	}

	std::optional<FileInfo> FileInfoCache::find(const std::string& path) const
	{
		ZoneScoped;
		std::lock_guard lock(m_mutex);
		const auto it = m_infos.find(path);
		return it != m_infos.end() ? std::optional(it->second) : std::nullopt;
	}

	void FileInfoCache::refresh(const std::string& path, std::string_view lastModified)
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_mutex);
			const auto it = m_infos.find(path);
			if (it != m_infos.end() && it->second.lastModified == lastModified)
			{
				if (!hasThumbnail(path) && !queued(Job::Kind::thumbnail, path))
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
		{
			std::lock_guard lock(m_mutex);
			if (m_running && m_running->kind == Job::Kind::thumbnail && m_running->path == path &&
				m_running->size == size)
			{
				return; // on its way
			}
			std::erase_if(m_jobs,
						  [&path](const Job& job) { return job.kind == Job::Kind::thumbnail && job.path == path; });
			m_jobs.push_front({.kind = Job::Kind::thumbnail, .path = path, .size = size});
		}
		startNext();
	}

	bool FileInfoCache::hasThumbnail(const std::string& path) const
	{
		struct stat status;
		// More than a byte: an empty file marks a thumbnail that couldn't be had
		return stat(thumbnailPath(path).c_str(), &status) == 0 && S_ISREG(status.st_mode) && status.st_size > 1;
	}

	std::filesystem::path FileInfoCache::thumbnailPath(const std::string& path) const
	{
		return printerFolder().value_or(m_temporaryRoot) / relative(path);
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

	void FileInfoCache::log() const
	{
		std::lock_guard lock(m_mutex);
		LOG_INFO("File info cache: {:d} files known, {:d} requests waiting{:s}",
				 m_infos.size(),
				 m_jobs.size(),
				 m_running ? ", one running" : "");
		for (const auto& [path, info] : m_infos)
		{
			LOG_INFO("  {:s}: {:s}, {:d} bytes, {:d} s, {:d} thumbnails{:s}",
					 path,
					 info.lastModified,
					 info.size,
					 info.printTime,
					 info.thumbnails.size(),
					 hasThumbnail(path) ? ", one kept" : "");
		}
	}

	void FileInfoCache::load()
	{
		ZoneScoped;
		std::unordered_map<std::string, FileInfo> infos;
		if (const auto folder = printerFolder(); folder && std::filesystem::exists(*folder))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(*folder))
			{
				if (!entry.is_regular_file() || entry.path().extension() != ".json")
				{
					continue;
				}
				std::ifstream file(entry.path());
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
					LOG_WARN("Ignoring {:s}: {:s}", entry.path().string(), info.error().message);
				}
			}
		}
		LOG_INFO("Loaded what is known about {:d} files", infos.size());
		std::lock_guard lock(m_mutex);
		m_infos = std::move(infos);
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

	std::string FileInfoCache::relative(const std::string& path)
	{
		std::string relative = path;
		utils::replaceSubstring(relative, ":", "\\%3A");
		return relative;
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
		auto writer = std::make_shared<ThumbnailWriter>();
		if (!thumbnail || !writer->open(*thumbnail, m_temporaryRoot / relative(job.path)))
		{
			LOG_DBG("{:s} has no thumbnail the screen can show", job.path);
			jobDone();
			return;
		}
		fetchPiece(job, *thumbnail, std::move(writer), thumbnail->offset);
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
		bool changed = true;
		{
			std::lock_guard lock(m_mutex);
			const auto it = m_infos.find(job.path);
			changed = it == m_infos.end() || it->second.lastModified != info.lastModified;
			m_infos[job.path] = info;
		}
		if (changed)
		{
			std::error_code error;
			std::filesystem::remove(thumbnailPath(job.path), error); // the file has changed: so may its thumbnail
		}
		save(info);
		Model::get().post<EventType::FileInfo>(job.path);
		if (!hasThumbnail(job.path))
		{
			std::lock_guard lock(m_mutex);
			if (!std::ranges::any_of(m_jobs,
									 [&](const Job& waiting)
									 { return waiting.kind == Job::Kind::thumbnail && waiting.path == job.path; }))
			{
				m_jobs.push_back({.kind = Job::Kind::thumbnail, .path = job.path, .size = ThumbnailSize::small});
			}
		}
		jobDone();
	}

	void FileInfoCache::fetchPiece(const Job& job,
								   const ThumbnailInfo& thumbnail,
								   std::shared_ptr<ThumbnailWriter> writer,
								   uint32_t offset)
	{
		ZoneScoped;
		uint64_t generation = 0;
		{
			std::lock_guard lock(m_mutex);
			generation = m_generation;
		}
		Connection::get().thumbnail(
			job.path,
			offset,
			[this, job, thumbnail, writer, offset, generation](std::expected<FileChunk, ReplyError> piece)
			{
				ZoneScopedN("fetchPiece_cb");
				{
					std::lock_guard lock(m_mutex);
					if (generation != m_generation)
					{
						return; // forgotten
					}
				}
				const std::filesystem::path temporary = m_temporaryRoot / relative(job.path);
				const auto fail = [&](std::string_view why)
				{
					LOG_WARN("No thumbnail for {:s}: {:s}", job.path, why);
					writer->finish();
					std::error_code error;
					std::filesystem::remove(temporary, error);
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
				if (!writer->add(piece->data))
				{
					fail("a piece couldn't be decoded");
					return;
				}
				if (piece->next != 0)
				{
					fetchPiece(job, thumbnail, writer, piece->next);
					return;
				}
				if (!writer->finish())
				{
					fail("it ended short");
					return;
				}
				if (move(temporary, thumbnailPath(job.path)))
				{
					Model::get().post<EventType::ThumbnailData>(job.path);
				}
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

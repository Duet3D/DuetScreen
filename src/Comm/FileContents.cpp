/*
 * FileContents.cpp
 */

#include "FileContents.h"
#include "Duet3D/General/CRC32.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <fstream>
#include <mutex>

namespace Comm
{
	struct FileContents::Source
	{
		std::string bytes;			// The bytes, when held in memory
		std::filesystem::path path; // Else the file they are read from
		size_t size = 0;			// How many there are
		uint32_t crc = 0;			// Their CRC-32
		CancelFlag cancelled;		// Set once they are cancelled
		std::mutex mutex;			// Guards `stream`: a link may read on a thread of its own
		std::ifstream stream;		// The file, opened on the first read
	};

	FileContents::FileContents(std::shared_ptr<Source> source)
		: m_source(std::move(source))
	{
	}

	FileContents FileContents::bytes(std::string bytes)
	{
		ZoneScoped;
		auto source = std::make_shared<Source>();
		source->cancelled = std::make_shared<std::atomic_bool>(false);
		CRC32 crc;
		crc.Update(bytes.data(), bytes.size());
		source->crc = crc.Get();
		source->size = bytes.size();
		source->bytes = std::move(bytes);
		return FileContents(std::move(source));
	}

	std::expected<FileContents, ReplyError> FileContents::file(const std::filesystem::path& path, CancelFlag cancelFlag)
	{
		ZoneScoped;
		std::ifstream stream(path, std::ios::binary);
		if (!stream.is_open())
		{
			return std::unexpected(ReplyError{.message = fmt::format("{:s} can't be opened", path.string())});
		}
		auto source = std::make_shared<Source>();
		source->path = path;
		source->cancelled = cancelFlag ? std::move(cancelFlag) : std::make_shared<std::atomic_bool>(false);
		CRC32 crc;
		std::array<char, 64 * 1024> buffer{};
		while (stream)
		{
			if (source->cancelled->load())
			{
				return std::unexpected(cancelledError());
			}
			stream.read(buffer.data(), buffer.size());
			const auto read = static_cast<size_t>(stream.gcount());
			crc.Update(buffer.data(), read);
			source->size += read;
		}
		if (stream.bad())
		{
			return std::unexpected(ReplyError{.message = fmt::format("{:s} can't be read", path.string())});
		}
		source->crc = crc.Get();
		return FileContents(std::move(source));
	}

	ReplyError FileContents::cancelledError()
	{
		return ReplyError{.message = "cancelled"};
	}

	void FileContents::cancel() const
	{
		m_source->cancelled->store(true);
	}

	bool FileContents::cancelled() const
	{
		return m_source->cancelled->load();
	}

	size_t FileContents::size() const
	{
		return m_source->size;
	}

	uint32_t FileContents::crc32() const
	{
		return m_source->crc;
	}

	std::expected<std::string, ReplyError> FileContents::read(size_t offset, size_t length) const
	{
		ZoneScoped;
		Source& source = *m_source;
		if (source.cancelled->load())
		{
			return std::unexpected(cancelledError());
		}
		length = std::min(length, source.size - std::min(offset, source.size));
		if (source.path.empty())
		{
			return source.bytes.substr(std::min(offset, source.size), length);
		}

		std::lock_guard lock(source.mutex);
		if (!source.stream.is_open())
		{
			source.stream.open(source.path, std::ios::binary);
		}
		std::string piece(length, '\0');
		source.stream.clear();
		source.stream.seekg(static_cast<std::streamoff>(offset));
		source.stream.read(piece.data(), static_cast<std::streamsize>(length));
		if (static_cast<size_t>(source.stream.gcount()) != length)
		{
			return std::unexpected(
				ReplyError{.message = fmt::format("{:s} can't be read any more (was its drive taken out?)",
												  source.path.string())});
		}
		return piece;
	}
} // namespace Comm

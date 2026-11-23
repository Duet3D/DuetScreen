/*
 * FileContents.h
 *
 * What is written to a file on the printer (Connection::writeFile()): bytes held in memory (the screen's settings, a
 * log), or a file on the screen (one on a USB drive), read a piece at a time as it is sent, so a large file is never
 * held whole. Its size and CRC-32 are known before anything is sent, as M559 needs them first.
 *
 * Copies share what they read: a link sends the pieces in order from one of them.
 */

#pragma once

#include "Comm/Replies.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace Comm
{
	/** Told how much of a file has been sent: `sent` of `total` bytes */
	using Progress = std::function<void(size_t sent, size_t total)>;

	class FileContents
	{
	  public:
		/** `bytes`, held in memory */
		static FileContents bytes(std::string bytes);

		/**
		 * The file at `path` on the screen, or why it can't be read. It is read through once, for its size and
		 * CRC-32, so for a large file call it away from the UI's and the connection's threads.
		 */
		static std::expected<FileContents, ReplyError> file(const std::filesystem::path& path);

		/** How many bytes there are */
		size_t size() const;

		/** Their CRC-32, as RepRapFirmware checks it (M559 C) */
		uint32_t crc32() const;

		/**
		 * Up to `length` bytes from `offset` on: fewer only at the end. Or why they can't be read, e.g. the USB drive
		 * the file is on was taken out, or the file is shorter than it was.
		 */
		std::expected<std::string, ReplyError> read(size_t offset, size_t length) const;

	  private:
		struct Source; // The bytes, or the file and where it is read from

		/** Contents read from `source` */
		explicit FileContents(std::shared_ptr<Source> source);

		std::shared_ptr<Source> m_source; // Shared by copies
	};
} // namespace Comm

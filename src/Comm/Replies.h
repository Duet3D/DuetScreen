/*
 * Replies.h
 *
 * What the printer's replies that aren't the object model say, read from the reply's JSON (OM::parseJson()). Each kind
 * is read in one place, the same whether it came over USB or UART (M20, M36, M36.1, M36.2) or over the network
 * (rr_filelist, rr_fileinfo, rr_thumbnail, and DSF's own API, which uses the same field names).
 *
 * A reply the firmware marks as failed ("err" other than 0) reads as a ReplyError, so whoever asked hears why.
 */

#pragma once

#include "ObjectModel/Tree/Node.h"
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Comm
{
	/** Why a reply can't be used */
	struct ReplyError
	{
		int32_t code = 0;	 // The firmware's "err", or 0 if the reply wasn't what was expected
		std::string message; // What was wrong, for the log
	};

	/** One item of a folder */
	struct FileEntry
	{
		bool folder = false; // A folder, not a file
		std::string name;	 // Its name, without the folder
		uint64_t size = 0;	 // Bytes, for a file
		std::string date;	 // When it last changed, e.g. "2026-08-27T13:54:52"; may be empty
	};

	/** One page of a folder listing: M20 S3, rr_filelist or DSF's directory listing */
	struct FileList
	{
		std::string dir;			  // The folder, e.g. "0:/gcodes"
		size_t first = 0;			  // Index of the first item of this page in the whole listing
		std::vector<FileEntry> files; // The items of this page
		size_t next = 0;			  // Index to ask for the next page from, or 0 if this is the last

		/** Read a listing; DSF's API sends a bare array of items, which is read as the whole of folder `dir` */
		static std::expected<FileList, ReplyError> read(const OM::Node& reply, std::string_view dir = {});
	};

	/** One of the thumbnails a G-code file holds */
	struct ThumbnailInfo
	{
		std::string format;	 // "png", "qoi" or "jpeg"
		uint32_t width = 0;	 // Pixels
		uint32_t height = 0; // Pixels
		uint32_t offset = 0; // Where its data starts in the file
		uint32_t size = 0;	 // Bytes of encoded data
	};

	/** What the firmware knows about a G-code file: M36, rr_fileinfo or DSF's file info */
	struct FileInfo
	{
		std::string fileName;				   // Full path on the printer
		uint64_t size = 0;					   // Bytes
		std::string lastModified;			   // When it last changed; may be empty
		double height = 0;					   // Height of the object, mm
		double layerHeight = 0;				   // mm
		uint32_t numLayers = 0;				   // 0 if unknown
		uint32_t printTime = 0;				   // The slicer's estimate, s; 0 if unknown
		uint32_t simulatedTime = 0;			   // The firmware's simulation, s; 0 if never simulated
		std::vector<double> filament;		   // Filament per extruder, mm
		std::string generatedBy;			   // The slicer; may be empty
		std::vector<ThumbnailInfo> thumbnails; // The thumbnails it holds

		/** Read file information */
		static std::expected<FileInfo, ReplyError> read(const OM::Node& reply);
	};

	/**
	 * One piece of a file: a thumbnail's (M36.1 or rr_thumbnail) or the file's contents (M36.2). Over USB and UART the
	 * fields are wrapped in an object ("thumbnail" or "fragment"); over the network they aren't.
	 */
	struct FileChunk
	{
		std::string fileName; // Full path on the printer
		uint32_t offset = 0;  // Where in the file `data` starts
		std::string data;	  // The piece: base64 for a thumbnail, text for contents
		uint32_t next = 0;	  // Where the next piece starts, or 0 if this is the last

		/**
		 * Read a piece
		 * @param reply The reply
		 * @param wrapper The object the fields are wrapped in over USB and UART ("thumbnail" or "fragment"); fields
		 * not wrapped are read from the reply itself
		 */
		static std::expected<FileChunk, ReplyError> read(const OM::Node& reply, std::string_view wrapper);
	};

	/** The firmware's "err" of `reply` as a ReplyError, if it is set and not 0 */
	std::optional<ReplyError> firmwareError(const OM::Node& reply);
} // namespace Comm

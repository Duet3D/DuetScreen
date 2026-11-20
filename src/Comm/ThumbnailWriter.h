/*
 * ThumbnailWriter.h
 *
 * Writes a G-code file's thumbnail to an image file as its pieces arrive (base64, a piece at a time over USB, UART
 * and a standalone Duet's network, whole through DSF): a PNG is written as it was sent, a QOI is decoded to a BMP, as
 * the screen shows PNG and BMP.
 */

#pragma once

#include "Comm/Replies.h"
#include "image/bmp.h"
#include "image/png.h"
#include "image/qoi.h"
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace Comm
{
	class ThumbnailWriter
	{
	  public:
		/** Whether a thumbnail described as `info` can be written: a PNG or QOI with a size */
		static bool canWrite(const ThumbnailInfo& info);

		/** Close the image if it is open */
		~ThumbnailWriter();

		/**
		 * Start writing the thumbnail `info` describes to `path`
		 * @return whether the image could be started
		 */
		bool open(const ThumbnailInfo& info, const std::filesystem::path& path);

		/**
		 * Add the next piece of the thumbnail
		 * @param base64 The piece, as sent
		 * @return whether it could be decoded and written
		 */
		bool add(std::string_view base64);

		/** Close the image; @return whether it is whole (a QOI decoded to its last pixel, a PNG ended) */
		bool finish();

	  private:
		/** Decode a piece of a QOI thumbnail to the BMP */
		bool addQoi(std::span<unsigned char> data);

		bool m_qoiFormat = false; // A QOI decoded to a BMP, rather than a PNG written as sent
		uint32_t m_width = 0;	  // Pixels
		uint32_t m_height = 0;	  // Pixels
		PNG m_png;				  // The PNG written
		BMP m_bmp;				  // The BMP written
		qoi_desc m_qoi{};		  // The QOI decoder's state
	};
} // namespace Comm

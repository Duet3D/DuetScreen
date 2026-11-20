#include "Debug.h"

#include "Thumbnail.h"
#include "nameof.hpp"

extern "C"
{
#include "image/base64.h"
#include <span>
#include <vector>
}

#include "sys/param.h"
#include <filesystem>
#include <sys/stat.h>

#include "Comm/FileInfo.h"
#include "utils/utils.h"

#define TEMP_THUMBNAIL_FOLDER "/tmp/thumbnails/"

std::filesystem::path GetThumbnailPath(const std::filesystem::path& filepath, const bool temp)
{
	std::string sanitisedFilename(filepath);
	utils::replaceSubstring(sanitisedFilename, ":", "\\%3A");
	// utils::replaceSubstring(sanitisedFilename, "/", "\%2F");

	std::filesystem::path folder;
	if (temp)
	{
		folder = std::filesystem::path(TEMP_THUMBNAIL_FOLDER);
	}
	else
	{
		folder = FILEINFO_CACHE->GetCachePath().value_or(TEMP_THUMBNAIL_FOLDER);
	}
	if (sanitisedFilename.rfind(folder) == 0)
	{
		return sanitisedFilename;
	}
	return folder / sanitisedFilename;
}

namespace Comm
{
	bool ThumbnailImage::New(const ThumbnailMeta& meta, const std::filesystem::path& filepath)
	{
		Close();
		qoi.decoder_state = qoi_decoder_state::qoi_decoder_header;
		imageFilename = filepath;
		std::filesystem::create_directories(imageFilename.parent_path());
		switch (meta.imageFormat)
		{
		case ThumbnailMeta::ImageFormat::Png:
			return png.New(imageFilename.c_str());
		case ThumbnailMeta::ImageFormat::Qoi:
			return bmp.New(meta.width, meta.height, imageFilename.c_str());
		default:
			return false;
		}
	}

	bool ThumbnailMeta::SetImageFormat(std::string_view format)
	{
		if (format == "qoi")
		{
			imageFormat = ImageFormat::Qoi;
			return true;
		}
		else if (format == "png")
		{
			imageFormat = ImageFormat::Png;
			return true;
		}
		else
		{
			imageFormat = ImageFormat::Invalid;
			return false;
		}
	}

	bool ThumbnailImage::IsOpen() const
	{
		return png.IsOpen() || bmp.IsOpen();
	}

	bool ThumbnailImage::Close()
	{
		return png.Close() && bmp.Close();
	}

	uint32_t Thumbnail::GetProgress() const
	{
		return std::min(100u, (100 * (std::max(meta.offset, context.offset) - meta.offset)) / meta.size);
	}

	bool Thumbnail::AboveCacheLimit() const
	{
		return meta.width > MAX_THUMBNAIL_CACHE_PIXELS || meta.height > MAX_THUMBNAIL_CACHE_PIXELS;
	}

	std::filesystem::path Thumbnail::GetPath() const
	{
		return GetThumbnailPath(filename.c_str());
	}

	void to_json(nlohmann::json& j, const ThumbnailMeta& m)
	{
		j = nlohmann::json{{"width", m.width},
						   {"height", m.height},
						   {"format",
							m.imageFormat == ThumbnailMeta::ImageFormat::Qoi
								? "qoi"
								: (m.imageFormat == ThumbnailMeta::ImageFormat::Png ? "png" : "invalid")},
						   {"offset", m.offset},
						   {"size", m.size}};
	}

	void from_json(const nlohmann::json& j, ThumbnailMeta& m)
	{
		if (j.contains("width"))
			j.at("width").get_to(m.width);
		if (j.contains("height"))
			j.at("height").get_to(m.height);
		if (j.contains("format"))
		{
			std::string_view format = j.at("format").get<std::string_view>();
			m.SetImageFormat(format);
		}
		if (j.contains("offset"))
			j.at("offset").get_to(m.offset);
		if (j.contains("size"))
			j.at("size").get_to(m.size);
	}

	void to_json(nlohmann::json& j, const Thumbnail& t)
	{
		j = nlohmann::json{
			{"filename", t.filename.c_str()},
			{"meta", t.meta},
		};
	}

	void from_json(const nlohmann::json& j, Thumbnail& t)
	{
		if (j.contains("meta"))
			j.at("meta").get_to(t.meta);
	}
} // namespace Comm

bool ThumbnailIsValid(const Comm::Thumbnail& thumbnail)
{
	if (thumbnail.filename.IsEmpty())
	{
		LOG_WARN("Thumbnail filename is empty");
		return false;
	}

	switch (thumbnail.meta.imageFormat)
	{
	case Comm::ThumbnailMeta::ImageFormat::Qoi:
	case Comm::ThumbnailMeta::ImageFormat::Png:
		break;
	default:
		LOG_WARN("Thumbnail {:s} format invalid", thumbnail.filename.c_str());
		return false;
	}

	if (thumbnail.meta.height == 0 || thumbnail.meta.width == 0)
	{
		LOG_WARN("Thumbnail invalid because heigh={:d}, width={:d}", thumbnail.meta.height, thumbnail.meta.width);
		return false;
	}

	return true;
}

int ThumbnailInit(Comm::Thumbnail& thumbnail)
{
	thumbnail.meta.width = 0;
	thumbnail.meta.height = 0;
	thumbnail.image.pixel_count = 0;
	thumbnail.meta.imageFormat = Comm::ThumbnailMeta::ImageFormat::Invalid;

	return qoi_decode_init(&thumbnail.image.qoi);
}

/** Add a decoded piece of a PNG thumbnail */
static int ThumbnailDecodeChunkPng(Comm::Thumbnail& thumbnail, std::span<unsigned char> data)
{
	size_t ret = thumbnail.image.png.appendData(data.data(), static_cast<int>(data.size()));
	LOG_DBG("done {:d}/{:d} {:s}\n", ret, data.size(), thumbnail.filename.c_str());
	return 0;
}

/** Decode a piece of a QOI thumbnail into its bitmap */
static int ThumbnailDecodeChunkQoi(Comm::Thumbnail& thumbnail, std::span<unsigned char> data)
{
	int ret;
	int size_done = 0;
	int pixel_decoded = 0;
	rgba_t rgba_buffer[1024];

	do
	{
		LOG_DBG("buffer {:p} (size {:d}, done {:d}) pixbuf {:p} (size {:d}, decoded {:d})\n",
				static_cast<const void*>(data.data()),
				data.size(),
				size_done,
				static_cast<const void*>(rgba_buffer),
				sizeof(rgba_buffer),
				pixel_decoded);
		ret = qoi_decode_chunked(&thumbnail.image.qoi,
								 data.data() + size_done,
								 static_cast<int>(data.size()) - size_done,
								 rgba_buffer,
								 sizeof(rgba_buffer),
								 &pixel_decoded);
		if (ret < 0)
		{
			LOG_ERROR("failed qoi decoding state {:s} {:d}.\n",
					  nameof::nameof_enum(qoi_decode_state_get(&thumbnail.image.qoi)),
					  ret);
			return -6;
		}

		if (thumbnail.image.qoi.height != thumbnail.meta.height || thumbnail.image.qoi.width != thumbnail.meta.width)
		{
			LOG_ERROR("thumbnail height {:d}, qoi height {:d}", thumbnail.meta.height, thumbnail.image.qoi.height);
			LOG_ERROR("thumbnail width {:d}, qoi width {:d}", thumbnail.meta.width, thumbnail.image.qoi.width);
			return -7;
		}

		size_done += ret;

		thumbnail.image.pixel_count += pixel_decoded;

		LOG_VERBOSE("decoded {:d} bytes, done {:d}/{:d}; decoded {:d} missing {:d}({:#02x}) count {:d}/{:d}/{:d}\n",
					ret,
					size_done,
					data.size(),
					pixel_decoded,
					thumbnail.image.qoi.last_bytes_size,
					thumbnail.image.qoi.last_bytes[0] & 0xc0,
					thumbnail.image.qoi.pixels_count,
					thumbnail.image.pixel_count,
					thumbnail.meta.height * thumbnail.meta.width);

		thumbnail.image.bmp.appendPixels(rgba_buffer, pixel_decoded);
	} while (size_done < static_cast<int>(data.size()) &&
			 qoi_decode_state_get(&thumbnail.image.qoi) == qoi_decoder_body);

	LOG_DBG("done {:d}/{:d} pixels {:d}/{:d} {:s}",
			size_done,
			data.size(),
			thumbnail.image.pixel_count,
			thumbnail.meta.height * thumbnail.meta.width,
			thumbnail.filename.c_str());

	return qoi_decode_state_get(&thumbnail.image.qoi) != qoi_decoder_done;
}

int ThumbnailDecodeChunk(Comm::Thumbnail& thumbnail, std::string_view base64)
{
	if (!ThumbnailIsValid(thumbnail))
	{
		LOG_ERROR("meta invalid.\n");
		return -1;
	}

	if (base64.empty())
	{
		LOG_ERROR("no data.\n");
		return -2;
	}

	std::vector<unsigned char> decoded(base64.size() / 4 * 3 + 3);
	int ret = base64_decode(base64.data(), static_cast<unsigned int>(base64.size()), decoded.data());
	if (ret < 0)
	{
		LOG_ERROR("decode error {:d} size {:d} data\n{:s}\n", ret, base64.size(), base64);
		return -4;
	}

	LOG_DBG("*** received size {:d}, base64 decoded size {:d}\n", base64.size(), ret);
	const std::span<unsigned char> data(decoded.data(), static_cast<size_t>(ret));

	switch (thumbnail.meta.imageFormat)
	{
	case Comm::ThumbnailMeta::ImageFormat::Png:
		return ThumbnailDecodeChunkPng(thumbnail, data);
	case Comm::ThumbnailMeta::ImageFormat::Qoi:
		return ThumbnailDecodeChunkQoi(thumbnail, data);
	default:
		// Shouldn't get here
		return -5;
	}
}

bool IsThumbnailCached(const std::filesystem::path& filepath, bool includeBlank)
{
	std::filesystem::path thumbnailPath = GetThumbnailPath(filepath);
	struct stat sb;
	// Use stat directly to avoid spawning a shell
	if (stat(thumbnailPath.c_str(), &sb) == 0)
	{
		// Ensure it's a regular file
		if (!S_ISREG(sb.st_mode))
		{
			return false;
		}
		if (!includeBlank && sb.st_size <= 1)
		{
			// File exists but is effectively empty
			return false;
		}
		return true;
	}
	return false;
}

bool ClearAllCachedThumbnails()
{
	LOG_INFO("Clearing all temp cached thumbnails");
	return std::filesystem::remove_all("/tmp/thumbnails") > 0;
}

bool DeleteCachedThumbnail(std::string_view filepath)
{
	LOG_INFO("Deleting thumbnail for {:s}", filepath);
	bool ret = true;
	// ret &= std::filesystem::remove(GetThumbnailPath(filepath, false));
	ret &= std::filesystem::remove(GetThumbnailPath(filepath, true));
	return ret;
}

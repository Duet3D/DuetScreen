#ifndef THUMBNAIL_HPP
#  define THUMBNAIL_HPP 1

#  include <cstddef>
#  include <sys/types.h>

#  include "Configuration.h"
#  include "Duet3D/General/String.h"
#  include "image/bmp.h"
#  include "image/png.h"
#  include "image/qoi.h"
#  include <nlohmann/json.hpp>
#  include <string>

namespace Comm
{
	enum ThumbnailState
	{
		Init = 0,
		DataRequest,
		DataWait,
		Data,
		Cached
	};

	struct ThumbnailMeta
	{
		uint32_t width;
		uint32_t height;
		enum ImageFormat
		{
			Invalid = 0,
			Qoi,
			Png,
		} imageFormat;
		uint32_t offset;
		uint32_t size;

		bool SetImageFormat(std::string_view format);
	};

	void to_json(nlohmann::json& j, const ThumbnailMeta& m);
	void from_json(const nlohmann::json& j, ThumbnailMeta& m);

	struct ThumbnailContext
	{
		enum class ParseErr
		{
			NoError = 0,
			InvalidOffset = -1,
			MismatchOffset = -2,
			InvalidNext = -3,
			InvalidSize = -4,
			RrfError = -5,
			Unknown = -6,
		};

		enum ThumbnailState state;
		ParseErr parseErr;
		int32_t err;
		uint32_t size;
		uint32_t offset;
		uint32_t next;

		ThumbnailContext() { Init(); }

		void Init()
		{
			state = ThumbnailState::Init;
			parseErr = ParseErr::NoError;
			err = 0;
			size = 0;
			offset = 0;
			next = 0;
		};
	};

	struct ThumbnailImage
	{
		uint32_t pixel_count;

		qoi_desc qoi;
		PNG png;
		BMP bmp;
		std::filesystem::path imageFilename; // Path to the image file on disk

		bool New(const ThumbnailMeta& meta, const std::filesystem::path& filepath);
		bool IsOpen() const;
		bool Close();
	};

	struct Thumbnail
	{
		StringRef filename; // full path of the file on the Duet
		ThumbnailMeta meta;
		ThumbnailContext context;
		ThumbnailImage image;

		Thumbnail(StringRef name)
			: filename(name)
		{
		}
		uint32_t GetProgress() const;
		bool AboveCacheLimit() const;
		std::filesystem::path GetPath() const;
	};

	void to_json(nlohmann::json& j, const Thumbnail& t);
	void from_json(const nlohmann::json& j, Thumbnail& t);

} // namespace Comm

typedef bool (*ThumbnailProcessCb)(const struct Thumbnail& thumbnail,
								   uint32_t pixels_offset,
								   const rgba_t* pixels,
								   size_t pixels_count);

bool ThumbnailIsValid(const Comm::Thumbnail& thumbnail);

int ThumbnailInit(Comm::Thumbnail& thumbnail);
/** Decode a piece of `thumbnail`'s data, sent as base64, into its image; 0 or more once it is done, less on error */
int ThumbnailDecodeChunk(Comm::Thumbnail& thumbnail, std::string_view base64);

std::filesystem::path GetThumbnailPath(const std::filesystem::path& filepath, const bool temp = false);
bool IsThumbnailCached(const std::filesystem::path& filepath, bool includeBlank = false);
bool ClearAllCachedThumbnails();
bool DeleteCachedThumbnail(std::string_view filepath);

#endif /* ifndef THUMBNAIL_HPP */

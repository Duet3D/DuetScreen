/*
 * ThumbnailWriter.cpp
 */

#include "ThumbnailWriter.h"
#include "Debug.h"
extern "C"
{
#include "image/base64.h"
}
#include "tracy/Tracy.hpp"
#include <array>
#include <vector>

namespace Comm
{
	bool ThumbnailWriter::canWrite(const ThumbnailInfo& info)
	{
		return (info.format == "png" || info.format == "qoi") && info.width > 0 && info.height > 0 && info.size > 0;
	}

	ThumbnailWriter::~ThumbnailWriter()
	{
		m_png.Close();
		m_bmp.Close();
	}

	bool ThumbnailWriter::open(const ThumbnailInfo& info, const std::filesystem::path& path)
	{
		ZoneScoped;
		if (!canWrite(info))
		{
			return false;
		}
		std::filesystem::create_directories(path.parent_path());
		m_qoiFormat = info.format == "qoi";
		m_width = info.width;
		m_height = info.height;
		if (m_qoiFormat)
		{
			qoi_decode_init(&m_qoi);
			return m_bmp.New(static_cast<int>(info.width), static_cast<int>(info.height), path.c_str());
		}
		return m_png.New(path.c_str());
	}

	bool ThumbnailWriter::add(std::string_view base64)
	{
		ZoneScoped;
		ZoneValue(base64.size());
		if (base64.empty())
		{
			return true;
		}
		std::vector<unsigned char> decoded(base64.size() / 4 * 3 + 3);
		const int size = base64_decode(base64.data(), static_cast<unsigned int>(base64.size()), decoded.data());
		if (size < 0)
		{
			LOG_ERROR("A thumbnail piece isn't base64 ({:d})", size);
			return false;
		}
		const std::span<unsigned char> data(decoded.data(), static_cast<size_t>(size));
		if (m_qoiFormat)
		{
			return addQoi(data);
		}
		m_png.appendData(data.data(), static_cast<int>(data.size()));
		return true;
	}

	bool ThumbnailWriter::addQoi(std::span<unsigned char> data)
	{
		ZoneScoped;
		std::array<rgba_t, 1024> pixels;
		int used = 0;
		while (used < static_cast<int>(data.size()) && qoi_decode_state_get(&m_qoi) != qoi_decoder_done)
		{
			int decoded = 0;
			const int taken = qoi_decode_chunked(&m_qoi,
												 data.data() + used,
												 static_cast<int>(data.size()) - used,
												 pixels.data(),
												 static_cast<int>(sizeof(pixels)),
												 &decoded);
			if (taken < 0)
			{
				LOG_ERROR("A QOI thumbnail couldn't be decoded ({:d})", taken);
				return false;
			}
			if (m_qoi.width != m_width || m_qoi.height != m_height)
			{
				LOG_ERROR("A QOI thumbnail is {:d}x{:d}, not {:d}x{:d} as its file says",
						  m_qoi.width,
						  m_qoi.height,
						  m_width,
						  m_height);
				return false;
			}
			m_bmp.appendPixels(pixels.data(), decoded);
			used += taken;
			if (taken == 0 && decoded == 0)
			{
				break; // waiting for more data
			}
		}
		return true;
	}

	bool ThumbnailWriter::finish()
	{
		ZoneScoped;
		const bool whole = !m_qoiFormat || qoi_decode_state_get(&m_qoi) == qoi_decoder_done;
		m_png.Close();
		m_bmp.Close();
		return whole;
	}
} // namespace Comm

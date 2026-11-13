/*
 * utils.cpp
 */

#include "utils.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>
#include <vector>

// Only lodepng's C functions: LVGL builds it as C, so its C++ API isn't there to link to
#define LODEPNG_NO_COMPILE_CPP
#include "src/libs/lodepng/lodepng.h"

namespace
{
	/** The width and height of the PNG file at `path`, from its header; nullopt if there is none */
	std::optional<std::pair<uint32_t, uint32_t>> pngSize(const std::filesystem::path& path)
	{
		std::array<unsigned char, 24> header{};
		std::ifstream file(path, std::ios::binary);
		if (!file.read(reinterpret_cast<char*>(header.data()), header.size()))
		{
			return std::nullopt;
		}
		const auto bigEndian = [&](size_t at)
		{
			return (uint32_t{header[at]} << 24) | (uint32_t{header[at + 1]} << 16) | (uint32_t{header[at + 2]} << 8) |
				   header[at + 3];
		};
		return std::pair{bigEndian(16), bigEndian(20)};
	}

	/** Draw the screen and save the display as the PNG file `path` */
	void saveDisplay(const std::filesystem::path& path)
	{
		lv_obj_invalidate(lv_screen_active());
		lv_refr_now(nullptr);
		const lv_draw_buf_t* buffer = lv_display_get_buf_active(nullptr);
		std::vector<unsigned char> rgba;
		rgba.reserve(size_t{buffer->header.w} * buffer->header.h * 4);
		for (uint32_t y = 0; y < buffer->header.h; y++)
		{
			const uint8_t* row = buffer->data + size_t{y} * buffer->header.stride;
			for (uint32_t x = 0; x < buffer->header.w; x++)
			{
				const uint8_t* pixel = row + size_t{x} * 4; // Blue, green, red, unused
				rgba.insert(rgba.end(), {pixel[2], pixel[1], pixel[0], 0xFF});
			}
		}
		lodepng_encode32_file(path.c_str(), rgba.data(), buffer->header.w, buffer->header.h);
	}
} // namespace

bool comparePicture(const char* path)
{
	// LVGL's comparer fails a picture whose size changed without saving it, so run_tests.py would have nothing to
	// show; save it as `_err`, as LVGL does a picture whose pixels changed
	const std::filesystem::path reference = std::filesystem::path(REF_IMGS_PATH) / path;
	const auto size = pngSize(reference);
	const auto width = static_cast<uint32_t>(lv_display_get_horizontal_resolution(nullptr));
	const auto height = static_cast<uint32_t>(lv_display_get_vertical_resolution(nullptr));
	if (size && *size != std::pair{width, height})
	{
		std::filesystem::path err = reference;
		err.replace_filename(reference.stem().string() + "_err.png");
		saveDisplay(err);
		LOG_ERROR(
			"{:s} is {:d}x{:d}, the display {:d}x{:d}", reference.string(), size->first, size->second, width, height);
		return false;
	}
	return lv_test_screenshot_compare(path) == LV_TEST_SCREENSHOT_RESULT_PASSED;
}

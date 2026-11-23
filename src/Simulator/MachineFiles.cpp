/*
 * MachineFiles.cpp
 *
 * The simulated machine's SD card: its jobs (with thumbnails, but for those set without), a few macros and
 * filaments, heightmaps made for its bed, and the files written to it: the replies to M20, M36, M36.1 and M36.2, and
 * files written with M559 (read back with M36.2). Probing the bed for a heightmap (G29) is in MachineGcode.cpp.
 */

#include "Duet3D/General/CRC32.h"
#include "Machine.h"
#include "image/qoi.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <fmt/format.h>
#include <functional>
#include <numbers>
#include <optional>
#include <span>

extern "C"
{
#include "image/base64.h"
}

namespace Sim
{
	namespace
	{
		/** The heightmap G29 probes, written over each time */
		constexpr std::string_view PROBED = "heightmap.csv";

		/** Points along each axis of a heightmap */
		constexpr size_t MESH_POINTS = 11;

		/** How far in from the bed's edges a heightmap's grid starts, mm */
		constexpr float MESH_MARGIN = 10.0f;

		/** Thumbnail sizes a job has, small then large, as slicers embed them, pixels */
		constexpr std::array<int32_t, 2> THUMBNAIL_SIZES = {48, 160};

		/** About what a job's thumbnails add to its size, bytes */
		constexpr int64_t THUMBNAILS_SIZE = 30000;

		/** When a job was last changed at the earliest (2026-11-01T12:00:00), seconds since 1970 */
		constexpr std::time_t FILE_TIME = 1793534400;

		/** Characters of a thumbnail's base64 text sent in each reply to M36.1 */
		constexpr size_t THUMBNAIL_CHUNK = 1024;

		/** What ends a file written with M559 without a size, as RepRapFirmware's EOF_STRING */
		constexpr std::string_view END_OF_FILE = "<!-- **EoF** -->";

		/** `dir` without a trailing slash */
		std::string_view withoutSlash(std::string_view dir)
		{
			while (dir.size() > 3 && dir.back() == '/')
			{
				dir.remove_suffix(1);
			}
			return dir;
		}

		/** Whether (x, y) is inside the convex polygon `corners`, in order around it */
		bool inside(float x, float y, std::span<const std::array<float, 2>> corners)
		{
			bool positive = false;
			bool negative = false;
			for (size_t i = 0; i < corners.size(); i++)
			{
				const auto& a = corners[i];
				const auto& b = corners[(i + 1) % corners.size()];
				const float cross = (b[0] - a[0]) * (y - a[1]) - (b[1] - a[1]) * (x - a[0]);
				positive = positive || cross > 0.0f;
				negative = negative || cross < 0.0f;
			}
			return !(positive && negative);
		}

		/** The colour of `hue` (degrees) at `value` (0-1), quite saturated, as 8-bit RGB */
		std::array<uint8_t, 3> colour(float hue, float value)
		{
			const float chroma = value * 0.65f;
			const float h = std::fmod(hue, 360.0f) / 60.0f;
			const float x = chroma * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
			float rgb[3] = {0.0f, 0.0f, 0.0f};
			const int sector = static_cast<int>(h);
			const float order[6][3] = {
				{chroma, x, 0}, {x, chroma, 0}, {0, chroma, x}, {0, x, chroma}, {x, 0, chroma}, {chroma, 0, x}};
			for (int c = 0; c < 3; c++)
			{
				rgb[c] = order[std::clamp(sector, 0, 5)][c] + (value - chroma);
			}
			return {static_cast<uint8_t>(std::lround(rgb[0] * 255.0f)),
					static_cast<uint8_t>(std::lround(rgb[1] * 255.0f)),
					static_cast<uint8_t>(std::lround(rgb[2] * 255.0f))};
		}

		/**
		 * A thumbnail of `name`, `size` pixels square: a cube seen from above a corner, shaded, in a colour of
		 * its own, on a clear background; as a QOI image, base64 encoded
		 */
		std::string drawThumbnail(std::string_view name, int32_t size)
		{
			const float hue = static_cast<float>(std::hash<std::string_view>{}(name) % 360);
			const float c = static_cast<float>(size) / 2.0f;
			const float r = static_cast<float>(size) * 0.42f;
			const float w = r * std::numbers::sqrt3_v<float> / 2.0f;
			const std::array<std::array<float, 2>, 4> top = {
				{{c, c - r}, {c + w, c - r / 2}, {c, c}, {c - w, c - r / 2}}};
			const std::array<std::array<float, 2>, 4> left = {
				{{c - w, c - r / 2}, {c, c}, {c, c + r}, {c - w, c + r / 2}}};
			const std::array<std::array<float, 2>, 4> right = {
				{{c, c}, {c + w, c - r / 2}, {c + w, c + r / 2}, {c, c + r}}};
			const std::array faces = {std::make_pair(std::span<const std::array<float, 2>>(top), colour(hue, 0.95f)),
									  std::make_pair(std::span<const std::array<float, 2>>(left), colour(hue, 0.75f)),
									  std::make_pair(std::span<const std::array<float, 2>>(right), colour(hue, 0.55f))};

			std::vector<uint8_t> rgba(static_cast<size_t>(size * size) * 4, 0);
			for (int32_t y = 0; y < size; y++)
			{
				for (int32_t x = 0; x < size; x++)
				{
					const float px = static_cast<float>(x) + 0.5f;
					const float py = static_cast<float>(y) + 0.5f;
					for (const auto& [corners, rgb] : faces)
					{
						if (inside(px, py, corners))
						{
							uint8_t* pixel = &rgba[static_cast<size_t>(y * size + x) * 4];
							std::copy(rgb.begin(), rgb.end(), pixel);
							pixel[3] = 255;
							break;
						}
					}
				}
			}

			qoi_desc desc{};
			desc.width = static_cast<unsigned int>(size);
			desc.height = static_cast<unsigned int>(size);
			desc.channels = 4;
			desc.colorspace = QOI_SRGB;
			int length = 0;
			void* qoi = qoi_encode(rgba.data(), &desc, &length);
			if (qoi == nullptr)
			{
				return "";
			}
			std::string base64(BASE64_ENCODE_OUT_SIZE(static_cast<unsigned int>(length)), '\0');
			base64.resize(base64_encode(
				static_cast<const unsigned char*>(qoi), static_cast<unsigned int>(length), base64.data()));
			free(qoi);
			return base64;
		}
	} // namespace

	int64_t Machine::fileSize(const JobFile& file)
	{
		return 1000 + static_cast<int64_t>(file.printTime) * 250 + (file.thumbnail ? THUMBNAILS_SIZE : 0);
	}

	std::string Machine::fileDate(const JobFile& file)
	{
		const size_t contents = std::hash<std::string>{}(
			fmt::format("{:s}/{:d}/{:d}/{:d}", file.name, file.printTime, file.layers, file.thumbnail));
		const std::time_t when = FILE_TIME + static_cast<std::time_t>(contents % (30 * 24 * 3600));
		std::tm utc{};
		gmtime_r(&when, &utc);
		return fmt::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}",
						   utc.tm_year + 1900,
						   utc.tm_mon + 1,
						   utc.tm_mday,
						   utc.tm_hour,
						   utc.tm_min,
						   utc.tm_sec);
	}

	float Machine::jobHeight(const JobFile& file)
	{
		return static_cast<float>(file.layers) * 0.2f;
	}

	std::vector<Machine::Thumbnail> Machine::thumbnails(const JobFile& file)
	{
		std::vector<Thumbnail> all;
		if (!file.thumbnail)
		{
			return all;
		}
		uint32_t offset = 200; // after the slicer's first comments
		for (const int32_t size : THUMBNAIL_SIZES)
		{
			Thumbnail thumbnail{
				.width = size, .height = size, .offset = offset, .base64 = drawThumbnail(file.name, size)};
			offset += static_cast<uint32_t>(thumbnail.base64.size()) + 100;
			all.push_back(std::move(thumbnail));
		}
		return all;
	}

	const std::vector<std::string_view>& Machine::heightmapNames()
	{
		static const std::vector<std::string_view> names = {PROBED, "tilted.csv", "warped.csv", "partial.csv"};
		return names;
	}

	Machine::Mesh Machine::mesh(std::string_view name) const
	{
		Mesh mesh;
		for (size_t a = 0; a < 2; a++)
		{
			const float low = a < m_config.axes.size() ? m_config.axes[a].min : 0.0f;
			const float high = a < m_config.axes.size() ? m_config.axes[a].max : 200.0f;
			const float margin = high - low > 4.0f * MESH_MARGIN ? MESH_MARGIN : 0.0f;
			mesh.min[a] = low + margin;
			mesh.max[a] = std::max(mesh.min[a] + 1.0f, high - margin);
			mesh.points[a] = MESH_POINTS;
			mesh.spacing[a] = (mesh.max[a] - mesh.min[a]) / static_cast<float>(MESH_POINTS - 1);
		}
		// Each from (u, v), 0-1 across the grid: a dish (varying a little with each probe), a slope, a twist, and a
		// dish probed only within a circle
		const float ripple = 0.01f * static_cast<float>(m_probes % 5);
		for (size_t row = 0; row < MESH_POINTS; row++)
		{
			for (size_t col = 0; col < MESH_POINTS; col++)
			{
				const float u = static_cast<float>(col) / static_cast<float>(MESH_POINTS - 1);
				const float v = static_cast<float>(row) / static_cast<float>(MESH_POINTS - 1);
				const float du = u - 0.5f;
				const float dv = v - 0.5f;
				const float dish = 0.5f * (du * du + dv * dv) - 0.06f + 0.02f * std::sin(9.0f * u + 5.0f * v) + ripple;
				std::optional<float> z;
				if (name == "tilted.csv")
				{
					z = 0.3f * du + 0.1f * dv + 0.003f;
				}
				else if (name == "warped.csv")
				{
					z = 0.8f * du * dv + 0.03f * std::cos(7.0f * u) + 0.002f;
				}
				else if (name == "partial.csv")
				{
					if (du * du + dv * dv <= 0.3f)
					{
						z = dish;
					}
				}
				else
				{
					z = dish;
				}
				// RepRapFirmware writes a point not probed as 0: a probed one is never quite 0
				if (z && std::fabs(*z) < 0.0005f)
				{
					*z = 0.001f;
				}
				mesh.z.push_back(z);
			}
		}
		return mesh;
	}

	Machine::MeshStats Machine::meshStats(const Mesh& mesh)
	{
		MeshStats stats;
		double sum = 0.0;
		double squares = 0.0;
		size_t count = 0;
		for (const auto& z : mesh.z)
		{
			if (!z)
			{
				continue;
			}
			stats.low = count == 0 ? *z : std::min(stats.low, *z);
			stats.high = count == 0 ? *z : std::max(stats.high, *z);
			sum += static_cast<double>(*z);
			squares += static_cast<double>(*z) * static_cast<double>(*z);
			count++;
		}
		if (count > 0)
		{
			stats.mean = sum / static_cast<double>(count);
			stats.deviation = std::sqrt(std::max(0.0, squares / static_cast<double>(count) - stats.mean * stats.mean));
		}
		return stats;
	}

	std::string Machine::meshCsv(const Mesh& mesh)
	{
		const MeshStats stats = meshStats(mesh);
		std::string csv =
			fmt::format("RepRapFirmware height map file v2 generated at 2026-11-15 12:00, min error {:.3f}, "
						"max error {:.3f}, mean {:.3f}, deviation {:.3f}\n",
						stats.low,
						stats.high,
						stats.mean,
						stats.deviation);
		csv += "axis0,axis1,min0,max0,min1,max1,radius,spacing0,spacing1,num0,num1\n";
		csv += fmt::format("X,Y,{:.2f},{:.2f},{:.2f},{:.2f},-1.00,{:.2f},{:.2f},{:d},{:d}\n",
						   mesh.min[0],
						   mesh.max[0],
						   mesh.min[1],
						   mesh.max[1],
						   mesh.spacing[0],
						   mesh.spacing[1],
						   mesh.points[0],
						   mesh.points[1]);
		for (size_t row = 0; row < mesh.points[1]; row++)
		{
			for (size_t col = 0; col < mesh.points[0]; col++)
			{
				const auto& z = mesh.z[row * mesh.points[0] + col];
				csv += col > 0 ? "," : "";
				csv += z ? fmt::format("{:7.3f}", *z) : "      0";
			}
			csv += "\n";
		}
		return csv;
	}

	Json Machine::compensationModel() const
	{
		if (m_heightmap.empty())
		{
			return Json{{"fadeHeight", nullptr}, {"file", nullptr}, {"meshDeviation", nullptr}, {"type", "none"}};
		}
		const Mesh grid = mesh(m_heightmap);
		const MeshStats stats = meshStats(grid);
		return Json{
			{"fadeHeight", nullptr},
			{"file", "0:/sys/" + m_heightmap},
			{"liveGrid",
			 Json{{"axes", Json::array({"X", "Y"})},
				  {"maxs", Json::array({grid.max[0], grid.max[1]})},
				  {"mins", Json::array({grid.min[0], grid.min[1]})},
				  {"radius", -1.0},
				  {"spacings", Json::array({grid.spacing[0], grid.spacing[1]})}}},
			{"meshDeviation", Json{{"deviation", rounded(stats.deviation, 3)}, {"mean", rounded(stats.mean, 3)}}},
			{"type", "mesh"}};
	}

	Json Machine::fileListReply(std::string_view dir, int32_t first) const
	{
		const std::string_view folder = withoutSlash(dir);
		Json files = Json::array();
		const auto add =
			[&files](std::string_view type, std::string_view name, int64_t size, std::string_view date = FILE_DATE)
		{ files.push_back(Json{{"type", type}, {"name", name}, {"size", size}, {"date", date}}); };
		if (folder == "0:/gcodes")
		{
			for (const JobFile& file : m_config.files)
			{
				add("f", file.name, fileSize(file), fileDate(file));
			}
		}
		else if (folder == "0:/macros")
		{
			add("d", "Calibration", 0);
			add("f", "Clean nozzle.g", 120);
			add("f", "Lights on.g", 40);
			add("f", "Park.g", 80);
		}
		else if (folder == "0:/macros/Calibration")
		{
			add("f", "Bed level.g", 200);
		}
		else if (folder == "0:/filaments")
		{
			for (const char* filament : {"ABS", "PETG", "PLA", "TPU"})
			{
				add("d", filament, 0);
			}
		}
		else if (folder == "0:/sys")
		{
			add("f", "config.g", 2048);
			add("f", "homeall.g", 256);
			for (const std::string_view name : heightmapNames())
			{
				add("f", name, static_cast<int64_t>(meshCsv(mesh(name)).size()));
			}
		}
		Json page = Json::array();
		for (size_t i = static_cast<size_t>(std::max(0, first)); i < files.size(); i++)
		{
			page.push_back(files[i]);
		}
		return Json{{"dir", dir}, {"first", first}, {"files", page}, {"next", 0}};
	}

	Json Machine::fileInfoReply(std::string_view path) const
	{
		std::string_view wanted = path;
		if (wanted.empty() && m_job.running)
		{
			wanted = m_config.files[m_job.file].name;
		}
		const JobFile* file = findFile(wanted);
		if (file == nullptr)
		{
			return Json{{"err", 1}};
		}
		Json thumbnailsInfo = Json::array();
		for (const Thumbnail& thumbnail : thumbnails(*file))
		{
			thumbnailsInfo.push_back(Json{{"format", "qoi"},
										  {"height", thumbnail.height},
										  {"offset", thumbnail.offset},
										  {"size", thumbnail.base64.size()},
										  {"width", thumbnail.width}});
		}
		return Json{{"err", 0},
					{"fileName", "0:/gcodes/" + file->name},
					{"size", fileSize(*file)},
					{"lastModified", fileDate(*file)},
					{"height", rounded(jobHeight(*file))},
					{"layerHeight", 0.2},
					{"numLayers", file->layers},
					{"printTime", file->printTime},
					{"filament", Json::array({rounded(file->printTime * 1.5, 1)})},
					{"generatedBy", "DuetScreen simulator"},
					{"thumbnails", thumbnailsInfo}};
	}

	Json Machine::thumbnailReply(std::string_view path, uint32_t offset) const
	{
		const JobFile* file = findFile(path);
		for (const Thumbnail& thumbnail : file != nullptr ? thumbnails(*file) : std::vector<Thumbnail>())
		{
			const uint32_t end = thumbnail.offset + static_cast<uint32_t>(thumbnail.base64.size());
			if (offset < thumbnail.offset || offset >= end)
			{
				continue;
			}
			const std::string data = thumbnail.base64.substr(offset - thumbnail.offset, THUMBNAIL_CHUNK);
			const uint32_t next = offset + static_cast<uint32_t>(data.size());
			return Json{{"thumbnail",
						 Json{{"fileName", path},
							  {"offset", offset},
							  {"data", data},
							  {"next", next < end ? next : 0},
							  {"err", 0}}}};
		}
		return Json{{"thumbnail", Json{{"fileName", path}, {"offset", offset}, {"data", ""}, {"next", 0}, {"err", 1}}}};
	}

	Json Machine::fileContentsReply(std::string_view path, uint32_t offset) const
	{
		std::string text;
		if (const auto written = writtenFile(path))
		{
			text = *written;
		}
		else if (const JobFile* file = findFile(path);
				 file != nullptr && path.find("/gcodes") != std::string_view::npos)
		{
			text = fmt::format("; {:s}, a job for the simulated Duet\nG28\nM190 S60\nM109 S210\nG1 Z0.2 F600\n",
							   file->name);
		}
		else if (const size_t slash = path.find_last_of('/');
				 path.find("/sys") != std::string_view::npos &&
				 std::ranges::find(heightmapNames(), path.substr(slash + 1)) != heightmapNames().end())
		{
			text = meshCsv(mesh(path.substr(slash + 1)));
		}
		else if (path.find("/macros") != std::string_view::npos)
		{
			text = "; A macro on the simulated Duet\nM117 \"Macro run\"\n";
		}
		else
		{
			return Json{{"fragment", Json{{"fileName", path}, {"offset", offset}, {"next", 0}, {"err", 1}}}};
		}
		const std::string part = offset < text.size() ? text.substr(offset) : std::string();
		return Json{
			{"fragment", Json{{"fileName", path}, {"offset", offset}, {"data", part}, {"next", 0}, {"err", 0}}}};
	}

	std::string Machine::sdPath(std::string_view path)
	{
		if (path.starts_with("0:"))
		{
			return std::string(path);
		}
		if (path.starts_with('/'))
		{
			return fmt::format("0:{:s}", path);
		}
		return fmt::format("0:/sys/{:s}", path);
	}

	std::vector<Json> Machine::receiveFile(std::string& input)
	{
		Upload& upload = *m_upload;
		if (upload.size > 0)
		{
			const size_t take = std::min(upload.size - upload.contents.size(), input.size());
			upload.contents.append(input, 0, take);
			input.erase(0, take);
			if (upload.contents.size() < upload.size)
			{
				return {};
			}
		}
		else
		{
			// Without a size, the file ends with the end-of-file marker, which isn't kept
			upload.contents += input;
			input.clear();
			const size_t end = upload.contents.find(END_OF_FILE);
			if (end == std::string::npos)
			{
				return {};
			}
			input = upload.contents.substr(end + END_OF_FILE.size());
			upload.contents.resize(end);
		}
		Upload done = std::move(upload);
		m_upload.reset();
		CRC32 crc;
		crc.Update(done.contents.data(), done.contents.size());
		if (done.crc && *done.crc != crc.Get())
		{
			return {Json{{"resp", "Error: CRC32 checksum doesn't match\n"}}};
		}
		m_writtenFiles[done.path] = std::move(done.contents);
		return {};
	}

	std::optional<std::string> Machine::writtenFile(std::string_view path) const
	{
		const auto it = m_writtenFiles.find(sdPath(path));
		return it != m_writtenFiles.end() ? std::optional(it->second) : std::nullopt;
	}
} // namespace Sim

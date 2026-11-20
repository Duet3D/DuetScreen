/*
 * Heightmap.cpp
 *
 *  Created on: 26 Mar 2024
 *      Author: andy
 */

#include "Debug.h"

#include "Heightmap.h"
#include "LockWrapper.h"
#include "Tree/Tree.h"
#include <mutex>

#include "Comm/Connection.h"
#include "Comm/Gcode.h"
#include "Configuration.h"
#include "ObjectModel/Directories.h"
#include "utils/csv.h"
#include <cmath>
#include <map>
#include <sstream>

namespace OM
{
	static std::vector<HeightmapPtr> s_heightmapCache;
	static std::string s_emptyStr = "";

	HeightmapMeta::HeightmapMeta()
	{
		ZoneScoped;
		Reset();
	}

	HeightmapMeta::~HeightmapMeta() {}

	void HeightmapMeta::Reset()
	{
		ZoneScoped;
		m_isValid = false;
		m_axis[0] = "X";
		m_axis[1] = "Y";
		m_min[0] = 0.0;
		m_min[1] = 0.0;
		m_max[0] = 0.0;
		m_max[1] = 0.0;
		m_radius = 0.0;
		m_spacing[0] = 0.0;
		m_spacing[1] = 0.0;
		m_samples[0] = 0;
		m_samples[1] = 0;
		m_recipSpacing[0] = 0.0;
		m_recipSpacing[1] = 0.0;
	}

	void HeightmapMeta::Parse(std::string_view meta)
	{
		ZoneScoped;
		utils::CSV doc(meta, true);
		const std::vector<std::string>& headers = doc.GetHeaders();
		for (std::string_view header : headers)
		{
			LOG_DBG("Header: \"{:s}\"", header);
		}

		doc.GetCell("axis0", 0, m_axis[0]);
		doc.GetCell("axis1", 0, m_axis[1]);

		doc.GetCell("min0", 0, m_min[0]);
		doc.GetCell("min1", 0, m_min[1]);
		doc.GetCell("max0", 0, m_max[0]);
		doc.GetCell("max1", 0, m_max[1]);

		doc.GetCell("radius", 0, m_radius);

		doc.GetCell("spacing0", 0, m_spacing[0]);
		doc.GetCell("spacing1", 0, m_spacing[1]);

		doc.GetCell("num0", 0, m_samples[0]);
		doc.GetCell("num1", 0, m_samples[1]);

		CheckValidity();

		if (IsValid())
		{
			LOG_DBG("Axes: {:s}, {:s}", m_axis[0].c_str(), m_axis[1].c_str());
			LOG_DBG("Min: {:g}, {:g}", m_min[0], m_min[1]);
			LOG_DBG("Max: {:g}, {:g}", m_max[0], m_max[1]);
			LOG_DBG("Radius: {:g}", m_radius);
			LOG_DBG("Spacing: {:g}, {:g}", m_spacing[0], m_spacing[1]);
			LOG_DBG("Samples: {:d}, {:d}", m_samples[0], m_samples[1]);
		}
		else
		{
			LOG_WARN("Heightmap meta data is invalid");
		}
	}

	char HeightmapMeta::GetAxisLetter(size_t index) const
	{
		ZoneScoped;
		const std::string& label = m_axis[index];
		return label.empty() ? '\0' : label[0];
	}

	void HeightmapMeta::CheckValidity()
	{
		ZoneScoped;
		if (m_max[0] - m_min[0] < MinRange || m_spacing[0] < MinSpacing || m_max[1] - m_min[1] < MinRange ||
			m_spacing[1] < MinSpacing)
		{
			m_isValid = false;
		}
		else
		{
			m_isValid = GetNumSamples() != 0 && (m_radius < 0.0 || m_radius >= 1.0) && m_axis[0] != m_axis[1];

			if (m_isValid)
			{
				m_recipSpacing[0] = 1.0 / m_spacing[0];
				m_recipSpacing[1] = 1.0 / m_spacing[1];
			}
		}
	}

	Heightmap::Heightmap(std::string_view filename)
		: m_fileName(filename)
	{
		ZoneScoped;
	}

	void Heightmap::Reset()
	{
		ZoneScoped;
		m_heightmap.clear();
		meta.Reset();
		m_minError = 0.0f;
		m_maxError = 0.0f;
		m_meanError = 0.0f;
		m_stdDev = 0.0f;
		m_area = 0.0f;
	}

	bool Heightmap::LoadFromDuet(Heightmap::load_cb_t callback)
	{
		ZoneScoped;
		Reset();
		OM::FileSystem::RequestFileContents(
			OM::Directories::DirectoryType::SYSTEM,
			m_fileName,
			[this, callback](std::string_view csvContents)
			{
				if (csvContents.find("RepRapFirmware height map") == std::string::npos)
				{
					LOG_WARN("CSV file \"{:s}\" not a heightmap", m_fileName);
					return;
				}

				if (!ParseMeta(csvContents))
				{
					LOG_ERROR("Failed to parse meta data for heightmap {:s}", m_fileName.c_str());
					return;
				}

				if (!ParseData(csvContents))
				{
					LOG_ERROR("Failed to parse data for heightmap {:s}", m_fileName.c_str());
					return;
				}

				callback(*this);
			});

		return true;
	}

	const Heightmap::Point* Heightmap::GetPoint(size_t x, size_t y) const
	{
		ZoneScoped;
		if (x >= GetWidth() || y >= GetHeight())
		{
			LOG_ERROR("Invalid point {:d}, {:d}, heightmap size ({:d}, {:d})", x, y, GetWidth(), GetHeight());
			return nullptr;
		}
		return &m_heightmap[y * GetWidth() + x];
	}

	bool Heightmap::ParseMeta(std::string_view csvContents)
	{
		ZoneScoped;
		LOG_INFO("Parsing meta data for heightmap {:s}", m_fileName);
		size_t metaStart = utils::findInstance(csvContents, "\n", 1);
		size_t metaEnd = utils::findInstance(csvContents, "\n", 3);
		if (metaStart == std::string::npos || metaEnd == std::string::npos)
		{
			LOG_ERROR("Corrupt heightmap file {:s}", m_fileName);
			return false;
		}

		std::string_view metaStr = csvContents.substr(metaStart, metaEnd - metaStart);
		LOG_DBG("Meta:\n{:s}", metaStr);

		meta.Parse(metaStr);
		return true;
	}

	bool Heightmap::ParseData(std::string_view csvContents)
	{
		ZoneScoped;
		LOG_INFO("Parsing data for heightmap {:s}", m_fileName);
		size_t dataStart = utils::findInstance(csvContents, "\n", 3) + 1;
		if (dataStart == std::string::npos)
		{
			LOG_ERROR("Corrupt heightmap file {:s}", m_fileName);
			return false;
		}

		bool parseError = false;
		std::string_view dataStr = csvContents.substr(dataStart);
		LOG_DBG("Data:\n{:s}", dataStr);
		utils::CSV doc(dataStr, false);

		m_heightmap.clear();
		m_minError = 9999.9f;
		m_maxError = -9999.9f;

		size_t rows = doc.GetRowCount();
		size_t cols = doc.GetColumnCount();

		if (rows != GetHeight() || cols != GetWidth())
		{
			LOG_ERROR("Heightmap size mismatch: {:d} != {:d} or {:d} != {:d}", rows, GetHeight(), cols, GetWidth());
			return false;
		}

		// Resize the 1D vector to hold all points
		m_heightmap.resize(rows * cols);

		double errorSum = 0.0f;
		double errorSqrSum = 0.0f;
		double xMin = 9999.9f;	// Used to calculate area
		double yMin = 9999.9f;	// Used to calculate area
		double xMax = -9999.9f; // Used to calculate area
		double yMax = -9999.9f; // Used to calculate area

		for (size_t rowIdx = 0; rowIdx < rows; rowIdx++)
		{
			for (size_t colIdx = 0; colIdx < cols; colIdx++)
			{
				size_t index = rowIdx * cols + colIdx;
				Point& point = m_heightmap[index];
				std::string val;
				point.x = meta.GetMin(0) + static_cast<double>(colIdx) * meta.GetSpacing(0);
				point.y = meta.GetMin(1) + static_cast<double>(rowIdx) * meta.GetSpacing(1);
				if (point.x < xMin)
				{
					LOG_DBG("New xMin: {:g}", point.x);
					xMin = point.x;
				}
				if (point.x > xMax)
				{
					LOG_DBG("New xMax: {:g}", point.x);
					xMax = point.x;
				}
				if (point.y < yMin)
				{
					LOG_DBG("New yMin: {:g}", point.y);
					yMin = point.y;
				}
				if (point.y > yMax)
				{
					LOG_DBG("New yMax: {:g}", point.y);
					yMax = point.y;
				}
				if (!doc.GetCell(colIdx, rowIdx, val))
				{
					LOG_ERROR("Failed to get cell {:d}, {:d}", colIdx, rowIdx);
					point.isNull = true;
					parseError = true;
				}
				utils::removeCharFromString(val, ' ');
				if (val == "0")
				{
					point.isNull = true;
				}
				point.z = strtod(val.c_str(), NULL);

				LOG_DBG("Cell {:d}, {:d}{:s}: ({:g}, {:g}, {:g}) raw=\"{:s}\"",
						colIdx,
						rowIdx,
						point.isNull ? "[INVALID]" : "",
						point.x,
						point.y,
						point.z,
						val.c_str());
				if (point.z < m_minError)
				{
					m_minError = point.z;
				}
				if (point.z > m_maxError)
				{
					m_maxError = point.z;
				}
				errorSum += point.z;
				errorSqrSum += point.z * point.z;
			}
		}

		LOG_DBG("xMin={:g}, xMax={:g}, yMin={:g}, yMax={:g}", xMin, xMax, yMin, yMax);
		m_area =
			meta.GetRadius() > 0 ? meta.GetRadius() * meta.GetRadius() * M_PI : std::abs((xMax - xMin) * (yMax - yMin));
		m_meanError = errorSum / static_cast<double>(rows * cols);
		const double pointCount = static_cast<double>(GetPointCount());
		m_stdDev = sqrt(errorSqrSum * pointCount - errorSum * errorSum) / pointCount;

		if (rows * cols != m_heightmap.size())
		{
			LOG_WARN("Heightmap size mismatch: {:d} != {:d}", rows * cols, m_heightmap.size());
			parseError = true;
		}

		LOG_DBG("Heightmap: {:d} rows, {:d} cols, area={:g} mm^2, minError={:g} mm, maxError={:g} mm, meanError={:g} "
				"mm, stdDev=%.3f mm",
				rows,
				cols,
				m_area,
				m_minError,
				m_maxError,
				m_meanError,
				m_stdDev);

		return !parseError;
	}

	double Heightmap::GetInterpolatedPoint(double axis0, double axis1, bool extrapolate) const
	{
		ZoneScoped;
		// Last grid point
		const double xLast = meta.GetMin(0) + (static_cast<double>(meta.GetSamples(0)) - 1) * meta.GetSpacing(0);
		const double yLast = meta.GetMin(1) + (static_cast<double>(meta.GetSamples(1)) - 1) * meta.GetSpacing(1);

		// Clamp to rectangle so InterpolateXY will always have valid parameters
		const double fEPSILON = 0.01;
		bool outOfBounds = false;
		if (axis0 < meta.GetMin(0))
		{
			outOfBounds = true;
			axis0 = meta.GetMin(0);
		}
		if (axis1 < meta.GetMin(1))
		{
			outOfBounds = true;
			axis1 = meta.GetMin(1);
		}
		if (axis0 > xLast - fEPSILON)
		{
			outOfBounds = true;
			axis0 = xLast - fEPSILON;
		}
		if (axis1 > yLast - fEPSILON)
		{
			outOfBounds = true;
			axis1 = yLast - fEPSILON;
		}

		if (!extrapolate && outOfBounds)
			return std::numeric_limits<double>::quiet_NaN();

		const double xf = (axis0 - meta.GetMin(0)) * meta.GetRecipSpacing(0);
		const double xFloor = floor(xf);
		const int32_t xIndex = (int32_t)xFloor;
		const double yf = (axis1 - meta.GetMin(1)) * meta.GetRecipSpacing(1);
		const double yFloor = floor(yf);
		const int32_t yIndex = (int32_t)yFloor;

		double value;
		if (!InterpolateAxis0Axis1(xIndex, yIndex, xf - xFloor, yf - yFloor, value))
		{
			return std::numeric_limits<double>::quiet_NaN();
		}
		return value;
	}

	bool Heightmap::InterpolateAxis0Axis1(
		size_t axis0Index, size_t axis1Index, double axis0Frac, double axis1Frac, double& result) const
	{
		ZoneScoped;
		const size_t indexX0Y0 = GetMapIndex(axis0Index, axis1Index); // (X0,Y0)
		const size_t indexX1Y0 = indexX0Y0 + 1;						  // (X1,Y0)
		const size_t indexX0Y1 = indexX0Y0 + meta.GetSamples(0);	  // (X0 Y1)
		const size_t indexX1Y1 = indexX0Y1 + 1;						  // (X1,Y1)

		const double xyFrac = axis0Frac * axis1Frac;

		if (indexX1Y1 >= m_heightmap.size())
		{
			LOG_ERROR(
				"Invalid heightmap index {:d}, {:d}. Max index = {:d}", axis0Index, axis1Index, m_heightmap.size() - 1);
			return 0.0;
		}

		Point points[4] = {
			m_heightmap[indexX0Y0], m_heightmap[indexX1Y0], m_heightmap[indexX0Y1], m_heightmap[indexX1Y1]};

		for (Point& point : points)
		{
			if (point.isNull)
			{
				result = 0.0;
				return false;
			}
		}

		result = (points[0].z * (1.0 - axis0Frac - axis1Frac + xyFrac)) + (points[1].z * (axis0Frac - xyFrac)) +
				 (points[2].z * (axis1Frac - xyFrac)) + (points[3].z * xyFrac);
		return true;
	}

	/* Sends command to Duet to use the heightmap called `filename` */
	void LoadHeightmap(std::string_view filename)
	{
		ZoneScoped;
		LOG_INFO("Loading heightmap {:s}", filename);
		Comm::Connection::get().sendGcodef("G29 S1 P{:s}\n", Comm::quoted(filename));
	}

	/* Sends command to Duet to unload the heightmap */
	void UnloadHeightmap()
	{
		ZoneScoped;
		LOG_INFO("Unloading heightmap");
		Comm::Connection::get().sendGcode("G29 S2\n");
	}

	void ToggleHeightmap(std::string_view filename)
	{
		ZoneScoped;
		if (GetCurrentHeightmap() != filename)
		{
			LoadHeightmap(filename);
			return;
		}
		UnloadHeightmap();
	}

	std::string GetCurrentHeightmap()
	{
		ZoneScoped;
		std::lock_guard lock(mutexModel);
		const std::string_view path = Tree::get().root().at("move.compensation.file").text();
		const size_t slash = path.find_last_of('/');
		return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
	}

	HeightmapPtr GetHeightmapData(std::string_view filename)
	{
		ZoneScoped;
		for (const auto& cachedHeightmap : s_heightmapCache)
		{
			if (cachedHeightmap->GetFileName() == filename)
			{
				return cachedHeightmap;
			}
		}

		// Not found in cache, create a new one, add it, and return it.
		auto heightmap = std::make_shared<Heightmap>(std::string(filename));
		s_heightmapCache.push_back(heightmap);
		return heightmap;
	}

	size_t ClearHeightmapCache()
	{
		ZoneScoped;
		size_t count = s_heightmapCache.size();
		s_heightmapCache.clear();
		return count;
	}

	void RequestHeightmapFiles(FileSystem::request_files_cb_t callback)
	{
		ZoneScoped;
		LOG_DBG("Requesting heightmap files from Duet");
		FileSystem::RequestFiles(Directories::DirectoryType::SYSTEM, "", callback);
		ClearHeightmapCache();
	}

	FileSystem::ItemList GetHeightmapFiles(const FileSystem::ItemList& files)
	{
		ZoneScoped;
		FileSystem::ItemList csvFiles;

		for (const auto& item : files)
		{
			if (item->GetType() != FileSystem::FileSystemItemType::file)
				continue;

			// Ignore non CSV files
			if (item->GetName().rfind(".csv\0") == std::string::npos)
				continue;

			if (item->GetName().find(DEFAULT_FILAMENTS_FILE) != std::string::npos)
				continue;

			csvFiles.push_back(item);
		}

		return csvFiles;
	}
} // namespace OM

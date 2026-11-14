/*
 * Heightmap.h
 *
 *  Created on: 26 Mar 2024
 *      Author: andy
 */

#ifndef JNI_OBJECTMODEL_HEIGHTMAP_H_
#define JNI_OBJECTMODEL_HEIGHTMAP_H_

#include "Files.h"

#include <memory>
#include <string>
#include <vector>

namespace OM
{
	class Heightmap;
	using HeightmapPtr = std::shared_ptr<Heightmap>;

	class HeightmapMeta
	{
	  public:
		HeightmapMeta();
		~HeightmapMeta();

		void Reset();
		void Parse(std::string_view meta);

		/** The letter of the map's axis `index` (0 or 1), e.g. 'X'; '\0' if the map has none */
		char GetAxisLetter(size_t index) const;
		double GetMin(size_t index) const { return m_min[index]; }
		double GetMax(size_t index) const { return m_max[index]; }
		double GetSpacing(size_t index) const { return m_spacing[index]; }
		double GetRecipSpacing(size_t index) const { return m_recipSpacing[index]; }
		size_t GetSamples(size_t index) const { return m_samples[index]; }
		size_t GetNumSamples() const { return GetSamples(0) * GetSamples(1); }
		double GetRadius() const { return m_radius; }
		bool IsValid() const { return m_isValid; }

	  private:
		void CheckValidity();

		static constexpr double MinSpacing = 0.1; // The minimum point spacing allowed
		static constexpr double MinRange = 1.0;	  // The minimum X and Y range allowed

		std::string m_axis[2];
		double m_min[2];
		double m_max[2];
		double m_radius;
		double m_spacing[2];
		size_t m_samples[2];
		double m_recipSpacing[2];
		bool m_isValid = false;
	};

	class Heightmap
	{
	  public:
		using load_cb_t = std::function<void(Heightmap& heightmap)>;

		Heightmap(std::string_view filename);

		struct Point
		{
			double x;
			double y;
			double z = 0.0f;
			bool isNull = false;
		};

		void Reset();

		bool LoadFromDuet(load_cb_t callback);
		bool IsValid() const { return meta.IsValid(); }

		std::string_view GetFileName() const { return m_fileName; }
		size_t GetHeight() const { return meta.GetSamples(1); }
		size_t GetWidth() const { return meta.GetSamples(0); }
		const std::vector<Point>& GetPoints() const { return m_heightmap; }
		const Point* GetPoint(size_t x, size_t y) const;
		size_t GetPointCount() const { return m_heightmap.size(); }
		double GetArea() const { return m_area; }
		double GetMinError() const { return m_minError; }
		double GetMaxError() const { return m_maxError; }
		double GetMeanError() const { return m_meanError; }
		double GetStdDev() const { return m_stdDev; }

		double GetInterpolatedPoint(double axis0, double axis1, bool extrapolate = false) const;
		bool ParseMeta(std::string_view csvContents);
		bool ParseData(std::string_view csvContents);

		HeightmapMeta meta;

	  private:
		bool InterpolateAxis0Axis1(
			size_t axis0Index, size_t axis1Index, double axis0Frac, double axis1Frac, double& result) const;
		size_t GetMapIndex(size_t axis0Index, size_t axis1Index) const
		{
			return (axis1Index * meta.GetSamples(0)) + axis0Index;
		}

		std::string m_fileName;
		double m_minError = 0.0;
		double m_maxError = 0.0;
		double m_meanError = 0.0;
		double m_stdDev = 0.0;
		double m_area = 0.0;
		std::vector<Point> m_heightmap;
	};

	/** The height map the printer uses (move.compensation.file, without its folder); empty if none */
	std::string GetCurrentHeightmap();

	void LoadHeightmap(std::string_view filename);
	void UnloadHeightmap();
	void ToggleHeightmap(std::string_view filename);

	HeightmapPtr GetHeightmapData(std::string_view filename);
	size_t ClearHeightmapCache();

	void RequestHeightmapFiles(FileSystem::request_files_cb_t callback = nullptr);
	FileSystem::ItemList GetHeightmapFiles(const FileSystem::ItemList& files);
} // namespace OM

#endif /* JNI_OBJECTMODEL_HEIGHTMAP_H_ */

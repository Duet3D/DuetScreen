#include "HeightmapPresenter.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "HeightmapView.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "i18n/i18n.h"
#include <cmath>
#include <span>

#define RENDER_MEASUREMENT_POINTS 0

namespace UI
{
	namespace
	{
		HeightmapPresenter::AxisRange getAdjustedAxisRange(const OM::Move::AxisPtr& axis)
		{
			if (axis == nullptr)
			{
				return {};
			}

			const float toolOffset = OM::GetCurrentToolAxisOffset(axis);
			return {axis->minPosition + toolOffset, axis->maxPosition + toolOffset};
		}
	} // namespace

	void HeightmapPresenter::setRenderMode(HeightmapRenderMode mode)
	{
		ZoneScoped;
		m_mode = mode;
		render();
	}

	void HeightmapPresenter::setHeightmap(const std::shared_ptr<OM::Heightmap>& heightmap)
	{
		ZoneScoped;
		{
			std::unique_lock lock(m_heightmapMutex);
			m_heightmap = heightmap;
		}
		if (heightmap == nullptr)
		{
			getView()->clear();
		}
	}

	void HeightmapPresenter::render()
	{
		ZoneScoped;

		std::shared_ptr<OM::Heightmap> heightmap;
		{
			std::shared_lock lock(m_heightmapMutex);
			heightmap = m_heightmap;
		}

		if (heightmap == nullptr)
		{
			getView()->clear();
			return;
		}

		if (!heightmap->IsValid())
		{
			LOG_WARN("Heightmap is not valid");
			getView()->clear();
			return;
		}

		auto axis0 = heightmap->meta.GetAxis(0);
		auto axis1 = heightmap->meta.GetAxis(1);

		if (axis0 == nullptr || axis1 == nullptr)
		{
			LOG_WARN("Heightmap axes are not valid");
			return;
		}

		LOG_DBG("Rendering heightmap {:s}", heightmap->GetFileName());

		const auto axis0Range = getAdjustedAxisRange(axis0);
		const auto axis1Range = getAdjustedAxisRange(axis1);

		getView()->setShownHeightmapName(heightmap->GetFileName());
		getView()->setXRange({static_cast<int32_t>(axis0Range.min), static_cast<int32_t>(axis0Range.max)});
		getView()->setYRange({static_cast<int32_t>(axis1Range.min), static_cast<int32_t>(axis1Range.max)});

		m_axis0Range = axis0Range;
		m_axis1Range = axis1Range;

		switch (m_mode)
		{
		case HeightmapRenderMode::Fixed:
			getView()->setValueRange(-0.25f, 0.25f);
			break;
		case HeightmapRenderMode::Auto:
			getView()->setValueRange(static_cast<float>(heightmap->GetMinError()),
									 static_cast<float>(heightmap->GetMaxError()));
			break;
		}

		getView()->drawGrid();

		uint32_t width, height;
		getView()->getResolution(width, height);

		{
			ZoneScopedN("Heightmap Render Loop");
			float x_min, y_min, x_max, y_max;
			getView()->pxToPos(0, 0, x_min, y_min);
			getView()->pxToPos(width - 1, height - 1, x_max, y_max);
			float xStep = (x_max - x_min) / static_cast<float>(width - 1);
			float yStep = (y_max - y_min) / static_cast<float>(height - 1);

			/**
			 * Can improve performance by preventing UI updates until the whole heightmap finishes rendering
			 * (`UI_LOCK`) but since a large heightmap might take ~1-2 seconds and the UI would be frozen during that
			 * time, I think it is better to have a slightly slower but more responsive UI.
			 */
			for (uint32_t px = 0; px < width; px++)
			{
				ZoneScoped;
#ifdef TRACY_ENABLE
				auto name = fmt::format("px = {:d}", px);
				ZoneName(name.c_str(), name.size());
#endif
				for (uint32_t py = 0; py < height; py++)
				{
					float x = x_min + (static_cast<float>(px) * xStep);
					float y = y_min + (static_cast<float>(py) * yStep);
					double value = heightmap->GetInterpolatedPoint(x, y);
					if (std::isnan(value))
					{
						continue;
					}
					getView()->setPx(px, height - py - 1, static_cast<float>(value));
				}
			}
		}
		getView()->getHeightmap().getCanvas().invalidate();
		getView()->renderColorBar();

#if RENDER_MEASUREMENT_POINTS
		const auto& measurements = heightmap->GetPoints();
		for (size_t i = 0; i < measurements.size(); i++)
		{
			const auto& point = measurements[i];
			getView()->addMeasurementPoint(point.x, point.y);
		}
#endif

		getView()->setStatistics(heightmap->GetPointCount(),
								 heightmap->GetArea() / 100,
								 heightmap->GetMinError(),
								 heightmap->GetMaxError(),
								 heightmap->GetMeanError(),
								 heightmap->GetStdDev());
	}

	void HeightmapPresenter::setActiveHeightmap(const size_t index)
	{
		ZoneScoped;
		if (index >= m_heightmapFiles.size())
		{
			LOG_ERROR("Invalid heightmap index {:d}", index);
			return;
		}

		const std::string& name = m_heightmapFiles[index]->GetName();
		LOG_INFO("Loading heightmap {:s}", name);
		auto map = OM::GetHeightmapData(name);
		{
			std::unique_lock lock(m_heightmapMutex);
			m_heightmap = map;
		}
		if (map)
		{
			map->LoadFromDuet([this](OM::Heightmap& /* heightmap */) { render(); });
		}
	}

	void HeightmapPresenter::toggleHeightmap(const size_t index)
	{
		ZoneScoped;
		if (index >= m_heightmapFiles.size())
		{
			LOG_ERROR("Invalid heightmap index {:d}", index);
			return;
		}

		const std::string& name = m_heightmapFiles[index]->GetName();
		LOG_INFO("Toggling heightmap {:s}", name);
		OM::ToggleHeightmap(name.c_str());
	}

	void HeightmapPresenter::trueBedLevel()
	{
		ZoneScoped;
		Comm::DUET.SendGcode("G32\n");
	}

	void HeightmapPresenter::meshBedLevel()
	{
		ZoneScoped;
		Comm::DUET.SendGcode("G29\n");
	}

	void HeightmapPresenter::newCompensationFile()
	{
		ZoneScoped;
		LOG_INFO("New compensation file");
		updateHeightmapList();
		OM::RequestHeightmapFiles(
			[this](OM::FileSystem::ItemList files)
			{
				m_heightmapFiles = OM::GetHeightmapFiles(files);
				updateHeightmapList();
			});
	}

	void HeightmapPresenter::newDirectories()
	{
		ZoneScoped;
		LOG_DBG("New directories");
		OM::RequestHeightmapFiles(
			[this](OM::FileSystem::ItemList files)
			{
				m_heightmapFiles = OM::GetHeightmapFiles(files);
				updateHeightmapList();
			});
	}

	void HeightmapPresenter::newAxesData()
	{
		ZoneScoped;
		LOG_DBG("New axes data");
		std::shared_ptr<OM::Heightmap> heightmap;
		{
			std::shared_lock lock(m_heightmapMutex);
			heightmap = m_heightmap;
		}
		if (heightmap == nullptr)
		{
			LOG_DBG("Heightmap is not set, skipping axis range update");
			return;
		}

		auto axis0 = heightmap->meta.GetAxis(0);
		auto axis1 = heightmap->meta.GetAxis(1);

		if (axis0 == nullptr || axis1 == nullptr)
		{
			LOG_WARN("Heightmap axes are not valid");
			return;
		}

		const auto axis0Range = getAdjustedAxisRange(axis0);
		const auto axis1Range = getAdjustedAxisRange(axis1);

		if (m_axis0Range == axis0Range && m_axis1Range == axis1Range)
		{
			LOG_VERBOSE("No change in axis range");
			return;
		}

		if (heightmap != nullptr && heightmap->IsValid())
		{
			render();
		}
	}

	void HeightmapPresenter::newStatus(OM::PrinterStatus status)
	{
		ZoneScoped;
		getView()->getControlButtons().setState(
			LV_STATE_DISABLED, OM::IsPrintingStatus(status) || !OM::IsConnected(), true);
	}

	void HeightmapPresenter::requestHeightmaps()
	{
		ZoneScoped;
		OM::RequestHeightmapFiles(
			[this](const OM::FileSystem::ItemList& files)
			{
				m_heightmapFiles = OM::GetHeightmapFiles(files);
				updateHeightmapList();
			});
		std::string_view currentHeightmap = OM::GetCurrentHeightmap();
		std::shared_ptr<OM::Heightmap> map =
			currentHeightmap.empty() ? nullptr : OM::GetHeightmapData(currentHeightmap);
		if (map)
		{
			map->LoadFromDuet([this](OM::Heightmap& /* heightmap */) { render(); });
		}
		setHeightmap(map);
	}

	void HeightmapPresenter::updateHeightmapList()
	{
		ZoneScoped;
		OM::FileSystem::SortFilesBy(m_heightmapFiles, OM::FileSystem::SortBy::NAME, false);
		getView()->setHeightmapCount(m_heightmapFiles.size());

		bool selected = false;
		for (size_t i = 0; i < m_heightmapFiles.size(); i++)
		{
			getView()->setHeightmapName(i, m_heightmapFiles[i]->GetName());
			if (m_heightmapFiles[i]->GetName() == OM::GetCurrentHeightmap())
			{
				selected = true;
				getView()->setSelectedHeightmap(i);
			}
		}
		if (!selected)
		{
			getView()->setSelectedHeightmap(-1);
		}
	}

	void HeightmapPresenter::onActivate()
	{
		ZoneScoped;
		LOG_DBG("activate");
		requestHeightmaps();

		newStatus(OM::GetStatus());
	}

	void HeightmapPresenter::onConnect()
	{
		ZoneScoped;
		LOG_DBG("Connected");
		requestHeightmaps();
	}

	void HeightmapPresenter::onDisconnect()
	{
		ZoneScoped;
		LOG_DBG("Disconnect");
		{
			std::unique_lock lock(m_heightmapMutex);
			if (m_heightmap != nullptr)
			{
				m_heightmap = nullptr;
			}
		}
		getView()->clear();
		getView()->setHeightmapCount(0);
	}
} // namespace UI

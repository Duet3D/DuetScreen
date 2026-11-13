/*
 * HeightmapPresenter.cpp
 */

#include "HeightmapPresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <cmath>
#include <limits>

namespace UI
{
	/** Samples along each side of the map: one per pixel of a <ds_heatmap> at its default resolution */
	static constexpr size_t SAMPLES = 200;

	/** The values the fixed render mode colours, mm either side of 0 */
	static constexpr float FIXED_SCALE = 0.25f;

	void HeightmapFileRow::fields(Xml::FieldVisitor& f)
	{
		f.field("name", name, "File name");
		f.field("loaded", loaded, "1 for the one the printer is using");
		f.field("load_text", loadText, "\"Load\", or \"Unload\" for the one in use");
	}

	HeightmapPresenter::HeightmapPresenter()
		: XmlPresenter("heightmap")
	{
		action("heightmap.show",
			   "Show that height map. From a row of `maps`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (auto file = fileOf(args))
				   {
					   load(OM::GetHeightmapData(file->GetName()));
				   }
			   });
		action("heightmap.toggle",
			   "Make the printer use that height map, or stop using it, and show it. From a row of `maps`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (auto file = fileOf(args))
				   {
					   OM::ToggleHeightmap(file->GetName());
					   load(OM::GetHeightmapData(file->GetName()));
				   }
			   });
		action("heightmap.mode",
			   "Colour values by `arg`: `fixed` (±0.25 mm) or `auto` (the map's own range)",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_mode = args.arg == "auto" ? RenderMode::Auto : RenderMode::Fixed;
				   m_modeSubject.set(static_cast<int32_t>(m_mode));
				   OM::HeightmapPtr map;
				   {
					   std::lock_guard lock(m_mapMutex);
					   map = m_map;
				   }
				   show(map);
			   });
		clear();
	}

	void HeightmapPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("surface", m_surface, "The height map shown, sampled for <ds_heatmap>");
		f.field("title", m_title, "E.g. \"Heightmap: heightmap.csv\"");
		f.list("maps", m_maps, "The height map files");
		f.field("stats.points", m_points, "Number of points, as text, e.g. \"Number of points: 0;\"");
		f.field("stats.area", m_area, "Probed area, as text");
		f.field("stats.min", m_min, "Lowest deviation, as text");
		f.field("stats.max", m_max, "Highest deviation, as text");
		f.field("stats.mean", m_mean, "Mean error, as text");
		f.field("stats.rms", m_rms, "RMS error, as text");
		f.field("mode", m_modeSubject, "How values are coloured: 0 fixed (±0.25 mm), 1 auto (the map's own range)");
		f.field("levelling_usable",
				m_levellingUsable,
				"1 if the bed can be levelled or probed: connected and no job running");
	}

	void HeightmapPresenter::onInit()
	{
		registerEventListener<EventType::CompensationFile>(
			[this]()
			{
				showFiles();
				requestFiles();
			});
		registerEventListener<EventType::Directories>([this]() { requestFiles(); });
		registerEventListener<EventType::AxesData>([this]() { axesChanged(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { showUsable(); });
	}

	void HeightmapPresenter::onActivate()
	{
		requestFiles();
		const std::string_view current = OM::GetCurrentHeightmap();
		load(current.empty() ? nullptr : OM::GetHeightmapData(current));
		showUsable();
	}

	void HeightmapPresenter::onConnect()
	{
		requestFiles();
	}

	void HeightmapPresenter::onDisconnect()
	{
		{
			std::lock_guard lock(m_mapMutex);
			m_map = nullptr;
		}
		m_files.clear();
		m_maps.clear();
		clear();
		showUsable();
	}

	void HeightmapPresenter::load(const OM::HeightmapPtr& map)
	{
		{
			std::lock_guard lock(m_mapMutex);
			m_map = map;
		}
		if (map == nullptr)
		{
			clear();
			return;
		}
		// The data may arrive after the tab has gone
		std::weak_ptr<XmlPresenter> self = weak_from_this();
		map->LoadFromDuet(
			[self](OM::Heightmap& loaded)
			{
				if (auto presenter = self.lock())
				{
					auto& heightmap = static_cast<HeightmapPresenter&>(*presenter);
					OM::HeightmapPtr shown;
					{
						std::lock_guard lock(heightmap.m_mapMutex);
						shown = heightmap.m_map;
					}
					if (shown.get() == &loaded) // Not replaced while it loaded
					{
						heightmap.show(shown);
					}
				}
			});
	}

	void HeightmapPresenter::show(const OM::HeightmapPtr& map)
	{
		ZoneScoped;
		if (map == nullptr || !map->IsValid())
		{
			UI_LOCK();
			clear();
			return;
		}
		auto axis0 = map->meta.GetAxis(0);
		auto axis1 = map->meta.GetAxis(1);
		if (axis0 == nullptr || axis1 == nullptr)
		{
			LOG_WARN("Heightmap axes are not valid");
			return;
		}

		// The map is drawn over the axes' travel, allowing for the tool's offset, in whole mm
		auto spanOf = [](const OM::Move::AxisPtr& axis) -> AxisRange
		{
			const float offset = OM::GetCurrentToolAxisOffset(axis);
			return {axis->minPosition + offset, axis->maxPosition + offset};
		};
		const AxisRange xRange = spanOf(axis0);
		const AxisRange yRange = spanOf(axis1);
		Xml::Surface::Values values;
		values.x = {std::trunc(xRange.min), std::trunc(xRange.max)};
		values.y = {std::trunc(yRange.min), std::trunc(yRange.max)};
		values.scale = m_mode == RenderMode::Auto ? Xml::Surface::Range{static_cast<float>(map->GetMinError()),
																		static_cast<float>(map->GetMaxError())}
												  : Xml::Surface::Range{-FIXED_SCALE, FIXED_SCALE};

		// Sampling takes a while for a large map, so it is done before taking the UI lock
		values.columns = SAMPLES;
		values.rows = SAMPLES;
		values.samples.resize(SAMPLES * SAMPLES);
		const float xStep = (values.x.max - values.x.min) / static_cast<float>(SAMPLES);
		const float yStep = (values.y.max - values.y.min) / static_cast<float>(SAMPLES);
		for (size_t j = 0; j < SAMPLES; j++)
		{
			for (size_t i = 0; i < SAMPLES; i++)
			{
				const float x = values.x.min + static_cast<float>(i) * xStep;
				const float y = values.y.min + static_cast<float>(j) * yStep;
				values.samples[j * SAMPLES + i] = static_cast<float>(map->GetInterpolatedPoint(x, y));
			}
		}

		UI_LOCK();
		m_xRange = xRange;
		m_yRange = yRange;
		m_title.set(_("heightmap.title", map->GetFileName()));
		m_surface.set(std::move(values));
		m_points.set(_("heightmap.num_points", map->GetPointCount()));
		m_area.set(_("heightmap.area", map->GetArea() / 100));
		m_min.set(_("heightmap.min_error", map->GetMinError()));
		m_max.set(_("heightmap.max_error", map->GetMaxError()));
		m_mean.set(_("heightmap.mean_error", map->GetMeanError()));
		m_rms.set(_("heightmap.std_dev", map->GetStdDev()));
	}

	void HeightmapPresenter::clear()
	{
		m_title.set(_("heightmap.title", ""));
		m_surface.clear();
		m_points.set(_("heightmap.num_points", 0));
		m_area.set(_("heightmap.area", 0.0));
		m_min.set(_("heightmap.min_error", 0.0));
		m_max.set(_("heightmap.max_error", 0.0));
		m_mean.set(_("heightmap.mean_error", 0.0));
		m_rms.set(_("heightmap.std_dev", 0.0));
	}

	void HeightmapPresenter::requestFiles()
	{
		std::weak_ptr<XmlPresenter> self = weak_from_this();
		OM::RequestHeightmapFiles(
			[self](const OM::FileSystem::ItemList& files)
			{
				if (auto presenter = self.lock())
				{
					auto& heightmap = static_cast<HeightmapPresenter&>(*presenter);
					UI_LOCK();
					heightmap.m_files = OM::GetHeightmapFiles(files);
					OM::FileSystem::SortFilesBy(heightmap.m_files, OM::FileSystem::SortBy::NAME, false);
					heightmap.showFiles();
				}
			});
	}

	void HeightmapPresenter::showFiles()
	{
		UI_LOCK();
		const std::string_view current = OM::GetCurrentHeightmap();
		m_maps.sync(m_files.size(),
					[this, current](HeightmapFileRow& row, size_t i)
					{
						const bool loaded = m_files[i]->GetName() == current;
						row.name.set(m_files[i]->GetName());
						row.loaded.set(loaded ? 1 : 0);
						row.loadText.set(loaded ? _("heightmap.unload") : _("heightmap.load"));
					});
	}

	void HeightmapPresenter::showUsable()
	{
		m_levellingUsable.set(OM::IsConnected() && !OM::IsPrintingStatus(OM::GetStatus()) ? 1 : 0);
	}

	void HeightmapPresenter::axesChanged()
	{
		OM::HeightmapPtr map;
		{
			std::lock_guard lock(m_mapMutex);
			map = m_map;
		}
		if (map == nullptr || !map->IsValid())
		{
			return;
		}
		auto axis0 = map->meta.GetAxis(0);
		auto axis1 = map->meta.GetAxis(1);
		if (axis0 == nullptr || axis1 == nullptr)
		{
			return;
		}
		const float xOffset = OM::GetCurrentToolAxisOffset(axis0);
		const float yOffset = OM::GetCurrentToolAxisOffset(axis1);
		const AxisRange xRange{axis0->minPosition + xOffset, axis0->maxPosition + xOffset};
		const AxisRange yRange{axis1->minPosition + yOffset, axis1->maxPosition + yOffset};
		if (xRange != m_xRange || yRange != m_yRange)
		{
			show(map);
		}
	}

	OM::FileSystem::ItemPtr HeightmapPresenter::fileOf(const Xml::ActionArgs& args) const
	{
		const auto* row = args.row<HeightmapFileRow>();
		if (row == nullptr || static_cast<size_t>(row->index.get()) >= m_files.size())
		{
			LOG_WARN("{:s} must come from a maps row", args.name);
			return nullptr;
		}
		return m_files[static_cast<size_t>(row->index.get())];
	}
} // namespace UI

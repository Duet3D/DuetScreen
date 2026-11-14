/*
 * HeightmapPresenter.cpp
 */

#include "HeightmapPresenter.h"
#include "Debug.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "i18n/i18n.h"
#include "utils/UnitSystem.h"
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
		f.field("has_maps", m_hasMaps, "1 if there are height map files, else 0");
		f.field("stats.points", m_points, "Number of points, e.g. \"121\"");
		f.field("stats.area", m_area, "Probed area, in cm² or in² as the displayed unit is, e.g. \"400 cm²\"");
		f.field("stats.min", m_min, "Lowest deviation in the displayed unit, e.g. \"-0.12 mm\"");
		f.field("stats.max", m_max, "Highest deviation in the displayed unit");
		f.field("stats.mean", m_mean, "Mean error in the displayed unit");
		f.field("stats.rms", m_rms, "RMS error in the displayed unit");
		f.field("mode", m_modeSubject, "How values are coloured: 0 fixed (±0.25 mm), 1 auto (the map's own range)");
		f.field("levelling_usable",
				m_levellingUsable,
				"1 if the bed can be levelled or probed: connected and no job running");
	}

	void HeightmapPresenter::onInit()
	{
		// The map in use is noted here, on the UI's thread: the file list arrives on the connection's
		m_compensationWatch = Xml::OmWatch({"move.compensation"},
										   [this]
										   {
											   m_current = OM::GetCurrentHeightmap();
											   showFiles();
											   requestFiles();
										   });
		m_directoriesWatch = Xml::OmWatch({"directories"}, [this] { requestFiles(); });
		m_axesWatch = Xml::OmWatch({"move.axes", "tools", "state.currentTool"}, [this] { axesChanged(); });
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { showUsable(); });
	}

	void HeightmapPresenter::onActivate()
	{
		requestFiles();
		m_current = OM::GetCurrentHeightmap();
		load(m_current.empty() ? nullptr : OM::GetHeightmapData(m_current));
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
		m_hasMaps.set(0);
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
		// The map is drawn over the axes' travel, allowing for the tool's offset, in whole mm
		const auto x = spanOf(map->meta.GetAxisLetter(0));
		const auto y = spanOf(map->meta.GetAxisLetter(1));
		if (!x || !y)
		{
			LOG_WARN("Heightmap axes are not valid");
			return;
		}
		const AxisRange xRange = *x;
		const AxisRange yRange = *y;
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
		showStats(static_cast<int>(map->GetPointCount()),
				  static_cast<float>(map->GetArea()),
				  static_cast<float>(map->GetMinError()),
				  static_cast<float>(map->GetMaxError()),
				  static_cast<float>(map->GetMeanError()),
				  static_cast<float>(map->GetStdDev()));
	}

	void HeightmapPresenter::showStats(int points, float areaMm2, float min, float max, float mean, float rms)
	{
		// In the displayed unit: millimetres to 0.001, or inches to 0.0001; the area in cm² or in²
		const std::string& unit = Units::getDisplayedDistanceUnit();
		const bool metric = Units::getCurrentDisplayedUnitSystem() == Units::UnitSystem::Metric;
		const float step = metric ? 1000.0f : 10000.0f;
		const auto distance = [step](float mm)
		{ return std::round(Units::convertDistanceToCurrentDisplayedUnit(mm) * step) / step; };
		const float area = metric ? areaMm2 / 100.0f : areaMm2 / (25.4f * 25.4f);
		m_points.set(_("heightmap.num_points", points));
		m_area.set(_("heightmap.area", std::round(area * 10.0f) / 10.0f, metric ? "cm" : unit));
		m_min.set(_("heightmap.min_error", distance(min), unit));
		m_max.set(_("heightmap.max_error", distance(max), unit));
		m_mean.set(_("heightmap.mean_error", distance(mean), unit));
		m_rms.set(_("heightmap.std_dev", distance(rms), unit));
	}

	void HeightmapPresenter::clear()
	{
		m_title.set(_("heightmap.title", ""));
		m_surface.clear();
		showStats(0, 0, 0, 0, 0, 0);
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
		m_maps.sync(m_files.size(),
					[this](HeightmapFileRow& row, size_t i)
					{
						const bool loaded = m_files[i]->GetName() == m_current;
						row.name.set(m_files[i]->GetName());
						row.loaded.set(loaded ? 1 : 0);
						row.loadText.set(loaded ? _("heightmap.unload") : _("heightmap.load"));
					});
		m_hasMaps.set(m_files.empty() ? 0 : 1);
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
		const auto xRange = spanOf(map->meta.GetAxisLetter(0));
		const auto yRange = spanOf(map->meta.GetAxisLetter(1));
		if (!xRange || !yRange)
		{
			return;
		}
		if (*xRange != m_xRange || *yRange != m_yRange)
		{
			show(map);
		}
	}

	std::optional<HeightmapPresenter::AxisRange> HeightmapPresenter::spanOf(char letter)
	{
		MODEL_LOCK();
		const OM::Node& axis = Xml::PrinterModel::axis(letter);
		if (!axis.exists())
		{
			return std::nullopt;
		}
		const float offset = Xml::PrinterModel::toolOffset(letter);
		return AxisRange{static_cast<float>(axis["min"].number()) + offset,
						 static_cast<float>(axis["max"].number()) + offset};
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

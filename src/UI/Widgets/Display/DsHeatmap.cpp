/*
 * DsHeatmap.cpp
 */

#include "DsHeatmap.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Binding/Surface.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <cmath>

namespace UI::Xml
{
	using namespace WidgetSupport;

	/** Pixels along each side when no resolution is given */
	static constexpr uint32_t DEFAULT_RESOLUTION = 200;

	DsHeatmap::DsHeatmap(const std::string& name, lv_obj_t* parent)
		: DsHeatmap(name, LvObjRef(parent))
	{
	}

	DsHeatmap::DsHeatmap(const std::string& name, LvObjRef&& parent)
		: Heatmap(name, parent)
	{
		setResolution(DEFAULT_RESOLUTION, DEFAULT_RESOLUTION);
	}

	void DsHeatmap::registerXml()
	{
		Widgets::add(
			{.name = "ds_heatmap",
			 .extends = "lv_obj",
			 .description = "A colour map of a surface, such as a bed's height map, with X and Y scales, a "
							"grid, a title, and the colour scale beside it",
			 .example =
				 R"(<ds_heatmap bind_surface="surface" bind_title="title" resolution="200" width="400" height="300"/>)",
			 .attributes =
				 {{"bind_surface", "The surface subject to show; redrawn whenever it changes", AttributeKind::Subject},
				  {"bind_title", "Follow a text subject for the title shown above the map", AttributeKind::Subject},
				  {"resolution", "Pixels the map is drawn with along each side, scaled to fit (default 200)"}},
			 .exampleSubjects = {{"title", "string", "Heightmap: heightmap.csv"}}},
			xmlCreate,
			xmlApply);
	}

	void DsHeatmap::show(const Surface& surface)
	{
		const Surface::Values& values = surface.get();
		setXRange({static_cast<int32_t>(values.x.min), static_cast<int32_t>(values.x.max)});
		setYRange({static_cast<int32_t>(values.y.min), static_cast<int32_t>(values.y.max)});
		setValueRange(values.scale.min, values.scale.max); // Also clears the map
		drawGrid();

		// Each pixel takes the sample nearest its position; row 0 of the canvas is the highest y
		uint32_t width = 0;
		uint32_t height = 0;
		getResolution(width, height);
		if (values.columns > 0 && values.rows > 0)
		{
			for (uint32_t px = 0; px < width; px++)
			{
				for (uint32_t py = 0; py < height; py++)
				{
					const float value = surface.sample(px * values.columns / width, py * values.rows / height);
					if (!std::isnan(value))
					{
						setPx(px, height - py - 1, value);
					}
				}
			}
		}
		getCanvas().invalidate();
		renderColorBar();
	}

	void* DsHeatmap::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* heatmap = new DsHeatmap(attrOr(attrs, "name", "heatmap"), parentOf(state));
		deleteWith(heatmap->getRootPtr(), heatmap);
		return heatmap->getRootPtr();
	}

	void DsHeatmap::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& heatmap = static_cast<DsHeatmap&>(*LvObj::fromPtr(root));
		if (const char* resolution = lv_xml_get_value_of(attrs, "resolution"))
		{
			const auto pixels = static_cast<uint32_t>(lv_xml_atoi(resolution));
			heatmap.setResolution(pixels, pixels);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_title"))
		{
			lv_subject_add_observer_obj(subject, titleObserverCb, root, &heatmap);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_surface"))
		{
			lv_subject_add_observer_obj(subject, surfaceObserverCb, root, &heatmap);
		}
	}

	void DsHeatmap::surfaceObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		if (subject->type != LV_SUBJECT_TYPE_POINTER || lv_subject_get_pointer(subject) == nullptr)
		{
			LOG_WARN("ds_heatmap bind_surface needs a Surface");
			return;
		}
		auto& heatmap = *static_cast<DsHeatmap*>(lv_observer_get_user_data(observer));
		heatmap.show(*static_cast<const Surface*>(lv_subject_get_pointer(subject)));
	}

	void DsHeatmap::titleObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsHeatmap*>(lv_observer_get_user_data(observer))->setTitle(lv_subject_get_string(subject));
	}
} // namespace UI::Xml

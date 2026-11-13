/*
 * DsPlot.cpp
 */

#include "DsPlot.h"
#include "DsPlotLayer.h"
#include "DsPlotRect.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "i18n/i18n.h"
#include "src/xml/lv_xml_style.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	/** Pixels along each side when no resolution is given */
	static constexpr uint32_t DEFAULT_RESOLUTION = 200;

	DsPlot::DsPlot(const std::string& name, lv_obj_t* parent)
		: DsPlot(name, LvObjRef(parent))
	{
	}

	DsPlot::DsPlot(const std::string& name, LvObjRef&& parent)
		: Canvas(name, parent)
	{
		setResolution(DEFAULT_RESOLUTION, DEFAULT_RESOLUTION);
		applyRange();
		lv_obj_t* root = getRootPtr();
		lv_obj_add_event_cb(root, eventCb, LV_EVENT_STYLE_CHANGED, this);
		lv_obj_add_event_cb(root, eventCb, LV_EVENT_LAYOUT_CHANGED, this);
		lv_obj_add_event_cb(area(), eventCb, LV_EVENT_SIZE_CHANGED, this);
	}

	void DsPlot::registerXml()
	{
		Widgets::add(
			{.name = "ds_plot",
			 .extends = "lv_obj",
			 .description = "An X/Y area with scales and a grid, such as the bed with its objects, whose content is "
							"widgets placed by X/Y position. `<ds_plot_layer>`s written inside it are kept over its "
							"drawing area, and `<ds_plot_rect>`s inside a layer are kept at their X/Y bounds.",
			 .example =
				 R"(<ds_plot title_tag="object_cancel.current_object_prefix" title_style="text_emphasis" bind_detail="current"
		 bind_x_min="x_min" bind_x_max="x_max" bind_y_min="y_min" bind_y_max="y_max" width="400" height="300">
	<ds_plot_layer name="layer">
		<ds_list list="objects" item="job_object_rect" width="100%" height="100%"/>
	</ds_plot_layer>
</ds_plot>)",
			 .attributes = {{"title", "The title above the area, if there is no `title_tag`"},
							{"title_tag", "Translation key for the title", AttributeKind::Translation},
							{"bind_title", "Follow a text subject for the title", AttributeKind::Subject},
							{"title_style", "A theme style for the title, e.g. `text_emphasis`", AttributeKind::Style},
							{"bind_detail", "Follow a text subject shown after the title", AttributeKind::Subject},
							{"x_min", "Left of the span (default 0)"},
							{"x_max", "Right of the span (default 100)"},
							{"y_min", "Bottom of the span (default 0)"},
							{"y_max", "Top of the span (default 100)"},
							{"bind_x_min", "Follow an int subject for `x_min`", AttributeKind::Subject},
							{"bind_x_max", "Follow an int subject for `x_max`", AttributeKind::Subject},
							{"bind_y_min", "Follow an int subject for `y_min`", AttributeKind::Subject},
							{"bind_y_max", "Follow an int subject for `y_max`", AttributeKind::Subject},
							{"resolution",
							 "Pixels the grid is drawn with along each side, scaled to fit (default 200)"}},
			 .exampleSubjects = {{"current", "string", "gear"},
								 {"x_min", "int", "0"},
								 {"x_max", "int", "300"},
								 {"y_min", "int", "0"},
								 {"y_max", "int", "200"}}},
			xmlCreate,
			xmlApply);
	}

	bool DsPlot::toWidget(float x, float y, lv_point_t& point) const
	{
		int32_t px = 0;
		int32_t py = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		if (!posToPx(x, y, px, py) || !getResolution(width, height) || width == 0 || height == 0)
		{
			return false;
		}
		// The grid is drawn at the resolution and stretched over the drawing area
		lv_obj_t* drawn = const_cast<DsPlot*>(this)->area();
		point.x = static_cast<int32_t>((static_cast<int64_t>(px) * lv_obj_get_width(drawn)) / width);
		point.y = static_cast<int32_t>((static_cast<int64_t>(py) * lv_obj_get_height(drawn)) / height);
		return true;
	}

	DsPlot* DsPlot::of(lv_obj_t* obj)
	{
		for (lv_obj_t* parent = lv_obj_get_parent(obj); parent != nullptr; parent = lv_obj_get_parent(parent))
		{
			if (auto* plot = dynamic_cast<DsPlot*>(LvObj::fromPtr(parent)))
			{
				return plot;
			}
		}
		return nullptr;
	}

	void* DsPlot::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* plot = new DsPlot(attrOr(attrs, "name", "plot"), parentOf(state));
		deleteWith(plot->getRootPtr(), plot);
		return plot->getRootPtr();
	}

	void DsPlot::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& plot = static_cast<DsPlot&>(*LvObj::fromPtr(root));

		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			plot.setTitle(_(tag));
		}
		else if (const char* title = lv_xml_get_value_of(attrs, "title"))
		{
			plot.setTitle(title);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_title"))
		{
			lv_subject_add_observer_obj(subject, titleObserverCb, root, &plot);
		}
		if (const char* styleName = lv_xml_get_value_of(attrs, "title_style"))
		{
			if (lv_xml_style_t* style = lv_xml_get_style_by_name(&state->scope, styleName))
			{
				plot.getTitle().addStyle(lv_xml_style_get_lv_style(style));
			}
			else
			{
				LOG_WARN("ds_plot title_style=\"{:s}\": no such style", styleName);
			}
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_detail"))
		{
			lv_subject_add_observer_obj(subject, detailObserverCb, root, &plot);
		}
		if (const char* resolution = lv_xml_get_value_of(attrs, "resolution"))
		{
			const auto pixels = static_cast<uint32_t>(lv_xml_atoi(resolution));
			plot.setResolution(pixels, pixels);
		}

		// The span: fixed values, then any subjects
		const struct
		{
			const char* name; // Attribute naming the value
			int32_t* value;	  // The end of the span it sets
		} ends[] = {{"x_min", &plot.m_range.xMin},
					{"x_max", &plot.m_range.xMax},
					{"y_min", &plot.m_range.yMin},
					{"y_max", &plot.m_range.yMax}};
		for (const auto& end : ends)
		{
			if (const char* value = lv_xml_get_value_of(attrs, end.name))
			{
				*end.value = lv_xml_atoi(value);
			}
		}
		plot.applyRange();
		for (const auto& end : ends)
		{
			const std::string bind = std::string("bind_") + end.name;
			if (lv_subject_t* subject = findSubject(state, attrs, bind.c_str()))
			{
				lv_subject_add_observer_obj(subject, rangeObserverCb, root, end.value);
			}
		}
	}

	void DsPlot::rangeObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		*static_cast<int32_t*>(lv_observer_get_user_data(observer)) = lv_subject_get_int(subject);
		static_cast<DsPlot&>(*LvObj::fromPtr(lv_observer_get_target_obj(observer))).applyRange();
	}

	void DsPlot::titleObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsPlot*>(lv_observer_get_user_data(observer))->setTitle(lv_subject_get_string(subject));
	}

	void DsPlot::detailObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsPlot*>(lv_observer_get_user_data(observer))->detail().setText(lv_subject_get_string(subject));
	}

	void DsPlot::eventCb(lv_event_t* e)
	{
		auto& plot = *static_cast<DsPlot*>(lv_event_get_user_data(e));
		if (lv_event_get_code(e) == LV_EVENT_STYLE_CHANGED)
		{
			// The grid is drawn in the theme's colours
			plot.clear();
			plot.drawGrid();
			return;
		}
		plot.placeContent();
	}

	void DsPlot::applyRange()
	{
		const range_t x = getXRange();
		const range_t y = getYRange();
		if (x.min == m_range.xMin && x.max == m_range.xMax && y.min == m_range.yMin && y.max == m_range.yMax &&
			getDrawBuf() != nullptr)
		{
			return;
		}
		setXRange(range_t{m_range.xMin, m_range.xMax});
		setYRange(range_t{m_range.yMin, m_range.yMax});
		clear();
		drawGrid();
		placeContent();
	}

	void DsPlot::placeContent()
	{
		lv_obj_t* root = getRootPtr();
		for (uint32_t i = 0; i < lv_obj_get_child_count(root); i++)
		{
			auto* layer = dynamic_cast<DsPlotLayer*>(LvObj::fromPtr(lv_obj_get_child(root, static_cast<int32_t>(i))));
			if (layer == nullptr)
			{
				continue;
			}
			layer->cover(area());
			lv_obj_tree_walk(
				layer->getRootPtr(),
				[](lv_obj_t* obj, void*)
				{
					if (auto* rect = dynamic_cast<DsPlotRect*>(LvObj::fromPtr(obj)))
					{
						rect->place();
					}
					return LV_OBJ_TREE_WALK_NEXT;
				},
				nullptr);
		}
	}

	LvLabel& DsPlot::detail()
	{
		if (m_detail == nullptr)
		{
			auto& header = getHeader();
			header.setFlexFlow(LV_FLEX_FLOW_ROW);
			header.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			m_detail = std::make_unique<LvLabel>("detail", header);
			m_detail->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		}
		return *m_detail;
	}
} // namespace UI::Xml

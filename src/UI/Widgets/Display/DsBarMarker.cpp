/*
 * DsBarMarker.cpp
 */

#include "DsBarMarker.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/View/Actions.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fmt/format.h>

namespace UI::Xml
{
	using namespace WidgetSupport;

	/** How far a press must move, in px, to be a drag rather than a tap */
	static constexpr int32_t DRAG_THRESHOLD = 2;

	/** Width of the triangle's base, px */
	static constexpr int32_t TRIANGLE_WIDTH = 21;

	/** How far outside the marker it can be tapped, px */
	static constexpr int32_t EXT_CLICK_AREA = 20;

	DsBarMarker::DsBarMarker(const std::string& name, lv_obj_t* parent)
		: Button(name, parent, "")
	{
		setExtClickArea(EXT_CLICK_AREA);
		// Added before any <action>, so a drag's click can be stopped before an action sees it
		addEventCallback(eventCb, LV_EVENT_ALL, this);
		show(m_value);
	}

	DsBarMarker::~DsBarMarker()
	{
		setBar(nullptr);
	}

	void DsBarMarker::registerXml()
	{
		Widgets::add(
			{.name = "ds_bar_marker",
			 .extends = "lv_obj",
			 .description =
				 "A value marked on a bar, such as a heater's target temperature above its current-temperature bar: a "
				 "button showing the value, placed along the bar where the value falls and pointing at it with a "
				 "triangle (the indicator part's background colour). Its x position is set from the value; set its y "
				 "with `align` and `y`. Drag it along the bar to choose a value, raising `value_confirmed` with it "
				 "when released (the marker shows the dragged value until the subject changes), or tap it to raise "
				 "`clicked`; a drag doesn't raise `clicked`.",
			 .example =
				 R"(<lv_obj width="400" height="100" scrollable="false">
	<ds_bar name="current" bind_value="current_value" bind_min="scale_min" bind_max="scale_max"
			width="100%" height="15" align="center"/>
	<ds_bar_marker name="target" bar="current" bind_value="active" bind_min="scale_min" bind_max="scale_max"
				   label_format="{} °C" align="left_mid" y="-30">
		<style name="input"/>
		<action trigger="clicked" name="heaters.set_active"/>
		<action trigger="value_confirmed" name="heaters.set_active"/>
	</ds_bar_marker>
</lv_obj>)",
			 .attributes = {{"bar",
							 "Name of the sibling bar it marks, written before the marker. The bar spans its parent's "
							 "width (e.g. width=\"100%\"), as the marker is placed by percentages of that width"},
							{"min", "Bottom of the scale the value is on (default 0)"},
							{"max", "Top of the scale the value is on (default 100)"},
							{"bind_min", "Follow an int subject for the bottom of the scale", AttributeKind::Subject},
							{"bind_max", "Follow an int subject for the top of the scale", AttributeKind::Subject},
							{"bind_value", "Follow an int subject for the value marked", AttributeKind::Subject},
							{"label_format", "fmt format for the value's text (default \"{}\")"}},
			 .exampleSubjects = {{"current_value", "int", "180"},
								 {"active", "int", "210"},
								 {"scale_min", "int", "0"},
								 {"scale_max", "int", "300"}}},
			xmlCreate,
			xmlApply);
	}

	void DsBarMarker::setValue(int32_t value)
	{
		m_value = value;
		if (!m_dragging)
		{
			show(value);
		}
	}

	void DsBarMarker::setRange(int32_t min, int32_t max)
	{
		m_min = min;
		m_max = max;
		place();
	}

	void DsBarMarker::setBar(lv_obj_t* bar)
	{
		if (m_bar != nullptr)
		{
			lv_obj_remove_event_cb_with_user_data(m_bar, barEventCb, this);
		}
		m_bar = bar;
		if (m_bar != nullptr)
		{
			lv_obj_add_event_cb(m_bar, barEventCb, LV_EVENT_SIZE_CHANGED, this);
			lv_obj_add_event_cb(m_bar, barEventCb, LV_EVENT_DELETE, this);
			place();
		}
	}

	void DsBarMarker::setLabelFormat(std::string format)
	{
		m_format = std::move(format);
		show(m_shown);
	}

	void* DsBarMarker::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* marker = new DsBarMarker(attrOr(attrs, "name", "marker"), parentOf(state));
		deleteWith(marker->getRootPtr(), marker);
		return marker->getRootPtr();
	}

	void DsBarMarker::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& marker = static_cast<DsBarMarker&>(*LvObj::fromPtr(root));

		if (const char* format = lv_xml_get_value_of(attrs, "label_format"))
		{
			marker.setLabelFormat(format);
		}
		const char* min = lv_xml_get_value_of(attrs, "min");
		const char* max = lv_xml_get_value_of(attrs, "max");
		if (min || max)
		{
			marker.setRange(min ? lv_xml_atoi(min) : marker.m_min, max ? lv_xml_atoi(max) : marker.m_max);
		}
		if (const char* bar = lv_xml_get_value_of(attrs, "bar"))
		{
			lv_obj_t* found = lv_obj_find_by_name(lv_obj_get_parent(root), bar);
			if (found == nullptr || found == root)
			{
				LOG_WARN("ds_bar_marker bar=\"{:s}\": no such sibling", bar);
			}
			else
			{
				marker.setBar(found);
			}
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_min"))
		{
			lv_subject_add_observer_obj(subject, minObserverCb, root, &marker);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_max"))
		{
			lv_subject_add_observer_obj(subject, maxObserverCb, root, &marker);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_value"))
		{
			lv_subject_add_observer_obj(subject, valueObserverCb, root, &marker);
		}
	}

	void DsBarMarker::valueObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsBarMarker*>(lv_observer_get_user_data(observer))->setValue(lv_subject_get_int(subject));
	}

	void DsBarMarker::minObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& marker = *static_cast<DsBarMarker*>(lv_observer_get_user_data(observer));
		marker.setRange(lv_subject_get_int(subject), marker.m_max);
	}

	void DsBarMarker::maxObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto& marker = *static_cast<DsBarMarker*>(lv_observer_get_user_data(observer));
		marker.setRange(marker.m_min, lv_subject_get_int(subject));
	}

	void DsBarMarker::eventCb(lv_event_t* e)
	{
		auto& marker = *static_cast<DsBarMarker*>(lv_event_get_user_data(e));
		switch (lv_event_get_code(e))
		{
		case LV_EVENT_PRESSED:
			marker.pressed();
			break;
		case LV_EVENT_PRESSING:
			marker.pressing();
			break;
		case LV_EVENT_RELEASED:
		case LV_EVENT_PRESS_LOST:
			marker.released();
			break;
		case LV_EVENT_CLICKED:
		case LV_EVENT_SHORT_CLICKED:
		case LV_EVENT_SINGLE_CLICKED:
			if (marker.m_dragged)
			{
				lv_event_stop_processing(e); // A drag isn't a tap: the actions after this callback don't see it
			}
			break;
		case LV_EVENT_SIZE_CHANGED:
			marker.place(); // The text's width moves where it must be to point at the value
			lv_obj_refresh_ext_draw_size(marker.getRootPtr());
			break;
		case LV_EVENT_REFR_EXT_DRAW_SIZE:
			lv_event_set_ext_draw_size(e, marker.triangleReach());
			break;
		case LV_EVENT_DRAW_MAIN:
			marker.drawTriangle(e);
			break;
		default:
			break;
		}
	}

	void DsBarMarker::barEventCb(lv_event_t* e)
	{
		auto& marker = *static_cast<DsBarMarker*>(lv_event_get_user_data(e));
		if (lv_event_get_code(e) == LV_EVENT_DELETE)
		{
			marker.m_bar = nullptr; // Deleted first: nothing to stop following
			return;
		}
		marker.place();
		lv_obj_refresh_ext_draw_size(marker.getRootPtr());
	}

	void DsBarMarker::pressed()
	{
		m_pressedAt = pointerPoint();
		lv_area_t coords;
		lv_obj_get_coords(getRootPtr(), &coords);
		m_holdOffset = m_pressedAt.x - coords.x1 - (lv_obj_get_width(getRootPtr()) * percentOf(m_shown)) / 100;
		m_dragging = false;
		m_dragged = false;
	}

	void DsBarMarker::pressing()
	{
		lv_indev_t* indev = lv_indev_active();
		if (indev == nullptr || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER ||
			lv_indev_get_scroll_obj(indev) != nullptr || m_bar == nullptr)
		{
			return;
		}
		const lv_point_t point = pointerPoint();
		if (std::abs(point.x - m_pressedAt.x) > DRAG_THRESHOLD)
		{
			m_dragging = true;
		}
		if (m_dragging)
		{
			show(valueAt(point.x));
		}
	}

	void DsBarMarker::released()
	{
		if (!m_dragging)
		{
			return;
		}
		m_dragging = false;
		m_dragged = true;
		float value = static_cast<float>(m_shown);
		lv_obj_send_event(getRootPtr(), Actions::valueConfirmedEvent(), &value);
	}

	void DsBarMarker::drawTriangle(lv_event_t* e)
	{
		if (m_bar == nullptr)
		{
			return;
		}
		lv_obj_t* root = getRootPtr();
		lv_area_t area;
		lv_obj_get_coords(root, &area);
		lv_area_t barArea;
		lv_obj_get_coords(m_bar, &barArea);

		lv_draw_triangle_dsc_t dsc;
		lv_draw_triangle_dsc_init(&dsc);
		dsc.base.layer = lv_event_get_layer(e);
		dsc.color = lv_obj_get_style_bg_color(root, LV_PART_INDICATOR);
		dsc.opa = lv_obj_get_style_bg_opa(root, LV_PART_INDICATOR);

		// The tip is at the value; the base stays within the marker's straight edge
		const int32_t tipX = area.x1 + lv_obj_get_width(root) * percentOf(m_shown) / 100;
		int32_t radius = lv_obj_get_style_radius(root, LV_PART_MAIN);
		if (radius == LV_RADIUS_CIRCLE)
		{
			radius = std::min(lv_obj_get_width(root), lv_obj_get_height(root)) / 2;
		}
		int32_t right = tipX + TRIANGLE_WIDTH / 2;
		int32_t left = tipX - TRIANGLE_WIDTH / 2;
		if (right > area.x2 - radius)
		{
			const int32_t shift = right - (area.x2 - radius);
			right -= shift;
			left -= shift;
		}
		if (left < area.x1 + radius)
		{
			const int32_t shift = area.x1 + radius - left;
			right += shift;
			left += shift;
		}

		const bool above = area.y2 <= barArea.y1;
		dsc.p[0] = {tipX, above ? barArea.y1 : barArea.y2};
		dsc.p[1] = {right, above ? area.y2 : area.y1};
		dsc.p[2] = {left, above ? area.y2 : area.y1};
		lv_draw_triangle(dsc.base.layer, &dsc);
	}

	int32_t DsBarMarker::triangleReach() const
	{
		if (m_bar == nullptr)
		{
			return 0;
		}
		lv_area_t area;
		lv_obj_get_coords(getRootPtr(), &area);
		lv_area_t barArea;
		lv_obj_get_coords(m_bar, &barArea);
		const int32_t reach = area.y2 <= barArea.y1 ? barArea.y1 - area.y2 : area.y1 - barArea.y2;
		return std::max<int32_t>(reach, 0);
	}

	void DsBarMarker::show(int32_t value)
	{
		m_shown = value;
		setText(fmt::format(fmt::runtime(m_format), value));
		place();
	}

	void DsBarMarker::place()
	{
		// Placed so the point `percent` along it is `percent` along the bar: at 0 its left edge is at the bar's left,
		// at 100 its right edge at the bar's right
		const int32_t percent = percentOf(m_shown);
		const int32_t barWidth = m_bar ? lv_obj_get_width(m_bar) : 0;
		const int32_t offset = barWidth == 0 ? 0 : percent * lv_obj_get_width(getRootPtr()) / barWidth;
		lv_obj_set_x(getRootPtr(), lv_pct(percent - offset));
	}

	int32_t DsBarMarker::percentOf(int32_t value) const
	{
		const int32_t range = m_max - m_min;
		return range > 0 ? std::clamp(100 * (value - m_min) / range, 0, 100) : 0;
	}

	int32_t DsBarMarker::valueAt(int32_t x) const
	{
		lv_area_t barArea;
		lv_obj_get_coords(m_bar, &barArea);
		const int32_t width = lv_obj_get_width(m_bar);
		if (width <= 0)
		{
			return m_shown;
		}
		const float range = static_cast<float>(m_max - m_min);
		const float along = static_cast<float>(x - barArea.x1 - m_holdOffset) / static_cast<float>(width);
		const float value =
			std::clamp(range * along + static_cast<float>(m_min), static_cast<float>(m_min), static_cast<float>(m_max));
		return static_cast<int32_t>(std::lroundf(value));
	}

	lv_point_t DsBarMarker::pointerPoint() const
	{
		lv_point_t point{};
		if (lv_indev_t* indev = lv_indev_active())
		{
			lv_indev_get_point(indev, &point);
			lv_obj_transform_point(getRootPtr(), &point, LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE);
		}
		return point;
	}
} // namespace UI::Xml

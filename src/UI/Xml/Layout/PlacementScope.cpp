/*
 * PlacementScope.cpp
 */

#include "PlacementScope.h"
#include "UI/Styles/Theme.h"
#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>

namespace UI::Xml
{
	namespace
	{
		/** The theme const `name` as a whole number of pixels, or nullopt if the theme has none (or it isn't one) */
		std::optional<int32_t> breakpoint(const char* name)
		{
			const char* text = lv_xml_get_const_silent(nullptr, name);
			if (text == nullptr)
			{
				return std::nullopt;
			}
			const std::string_view value(text);
			int32_t pixels = 0;
			const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), pixels);
			if (error != std::errc() || end != value.data() + value.size())
			{
				return std::nullopt;
			}
			return pixels;
		}
	} // namespace

	PlacementScope::PlacementScope(SizeClass unmeasured)
		: m_scope(nullptr)
		, m_themeListener(Themes::addThemeListener([this] { measure(); }))
		, m_widthClass(static_cast<int32_t>(unmeasured))
		, m_heightClass(static_cast<int32_t>(unmeasured))
		, m_sizeClass(static_cast<int32_t>(unmeasured))
	{
		ScopeBinder binder(m_scope);
		fields(binder);
	}

	PlacementScope::~PlacementScope()
	{
		Themes::removeThemeListener(m_themeListener);
		unfollow();
	}

	void PlacementScope::fields(FieldVisitor& f)
	{
		f.field("widget.width_class",
				m_widthClass,
				"How wide the widget's place is: 0 compact, 1 regular, 2 expanded, by the theme's breakpoints "
				"(widget_regular_min, widget_expanded_min); always 1 in a strip, and 2 outside a placed widget");
		f.field("widget.height_class", m_heightClass, "How tall the widget's place is, as `widget.width_class`");
		f.field(
			"widget.size_class",
			m_sizeClass,
			"The smaller of `widget.width_class` and `widget.height_class`: how big something that needs both, such "
			"as an arc, can be");
		f.field("widget.portrait", m_portrait, "1 if the widget's place is taller than it is wide, else 0");
	}

	void PlacementScope::follow(lv_obj_t* place)
	{
		unfollow();
		m_place = place;
		lv_obj_add_event_cb(place, sizeChangedCb, LV_EVENT_SIZE_CHANGED, this);
		lv_obj_add_event_cb(place, placeDeletedCb, LV_EVENT_DELETE, this);
		measure();
	}

	void PlacementScope::unfollow()
	{
		if (m_place != nullptr)
		{
			lv_obj_remove_event_cb_with_user_data(m_place, sizeChangedCb, this);
			lv_obj_remove_event_cb_with_user_data(m_place, placeDeletedCb, this);
			m_place = nullptr;
		}
	}

	PlacementScope::SizeClass PlacementScope::classify(int32_t length)
	{
		if (const auto expanded = breakpoint("widget_expanded_min"); expanded && length >= *expanded)
		{
			return SizeClass::Expanded;
		}
		if (const auto regular = breakpoint("widget_regular_min"); regular && length < *regular)
		{
			return SizeClass::Compact;
		}
		return SizeClass::Regular;
	}

	void PlacementScope::sizeChangedCb(lv_event_t* e)
	{
		static_cast<PlacementScope*>(lv_event_get_user_data(e))->measure();
	}

	void PlacementScope::placeDeletedCb(lv_event_t* e)
	{
		// The classes stay as they were; nothing is left to measure, or to stop following
		static_cast<PlacementScope*>(lv_event_get_user_data(e))->m_place = nullptr;
	}

	void PlacementScope::measure()
	{
		if (m_place == nullptr)
		{
			return; // A strip's widget: it keeps the classes it was made with
		}
		const int32_t width = lv_obj_get_width(m_place);
		const int32_t height = lv_obj_get_height(m_place);
		m_widthClass.set(static_cast<int32_t>(classify(width)));
		m_heightClass.set(static_cast<int32_t>(classify(height)));
		m_sizeClass.set(std::min(m_widthClass.get(), m_heightClass.get()));
		m_portrait.set(height > width ? 1 : 0);
	}
} // namespace UI::Xml

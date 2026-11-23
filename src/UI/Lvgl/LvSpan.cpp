/*
 * LvSpan.cpp
 *
 *  Created on: 2026-02-02
 *      Author: Andy Everitt
 */

#include "LvSpan.h"
#include "Debug.h"

namespace UI
{
	LvSpanGroup::LvSpanGroup(const std::string& name, LvObj& parent)
		: LvObj(lv_spangroup_create, name, parent)
	{
		ZoneScoped;
	}

	LvSpanGroup::LvSpan LvSpanGroup::addSpan()
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_t* span = lv_spangroup_add_span(getRootPtr());
		if (span == nullptr)
		{
			throw std::runtime_error("Failed to add span");
		}
		return LvSpan(*this, *span);
	}

	void LvSpanGroup::deleteSpan(LvSpan&& span)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_delete_span(getRootPtr(), static_cast<lv_span_t*>(span));
	}

	size_t LvSpanGroup::removeSpansFromIndex(size_t index, bool allFollowing)
	{
		ZoneScoped;
		UI_LOCK();
		size_t spanCount = getSpanCount();
		if (index >= spanCount)
		{
			return 0;
		}

		size_t countToRemove = allFollowing ? (spanCount - index) : 1;
		for (size_t i = 0; i < countToRemove; i++)
		{
			deleteSpan(getSpanByIndex(static_cast<int32_t>(index)).value());
		}

		return countToRemove;
	}

	void LvSpanGroup::setSpanText(LvSpan& span, const std::string& text)
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_set_text(static_cast<lv_span_t*>(span), text.c_str());
	}

	void LvSpanGroup::setSpanTextStatic(LvSpan& span, const char* text)
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_set_text_static(static_cast<lv_span_t*>(span), text);
	}

	void LvSpanGroup::setSpanStyle(LvSpan& span, const lv_style_t* style)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_span_style(getRootPtr(), static_cast<lv_span_t*>(span), style);
	}

	void LvSpanGroup::setSpanStyleStatic(LvSpan& span, const lv_style_t* style)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_span_style_static(getRootPtr(), static_cast<lv_span_t*>(span), style);
	}

	void LvSpanGroup::setOverflow(lv_span_overflow_t overflow)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_overflow(getRootPtr(), overflow);
	}

	void LvSpanGroup::setIndent(int32_t indent)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_indent(getRootPtr(), indent);
	}

	void LvSpanGroup::setMaxLines(int32_t lines)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_max_lines(getRootPtr(), lines);
	}

	void LvSpanGroup::setMode(lv_span_mode_t mode)
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_set_mode(getRootPtr(), mode);
	}

	std::optional<LvSpanGroup::LvSpan> LvSpanGroup::getSpanByIndex(int32_t index)
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_t* span = lv_spangroup_get_child(getRootPtr(), index);
		if (span == nullptr)
		{
			return std::nullopt;
		}
		return LvSpan(*this, *span);
	}

	LvSpanGroup::LvSpan LvSpanGroup::getOrCreateSpanByIndex(size_t index)
	{
		ZoneScoped;
		UI_LOCK();
		auto spanOpt = getSpanByIndex(static_cast<int32_t>(index));
		if (spanOpt.has_value())
		{
			return spanOpt.value();
		}

		return addSpan();
	}

	uint32_t LvSpanGroup::getSpanCount() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_span_count(getRootPtr());
	}

	lv_span_overflow_t LvSpanGroup::getOverflow() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_overflow(getRootPtr());
	}

	int32_t LvSpanGroup::getIndent() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_indent(getRootPtr());
	}

	int32_t LvSpanGroup::getMaxLines() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_max_lines(getRootPtr());
	}

	int32_t LvSpanGroup::getMaxLineHeight() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_max_line_height(getRootPtr());
	}

	lv_span_mode_t LvSpanGroup::getMode() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_mode(getRootPtr());
	}

	uint32_t LvSpanGroup::getExpandWidth(uint32_t max_width) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_expand_width(getRootPtr(), max_width);
	}

	int32_t LvSpanGroup::getExpandHeight(int32_t width) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_expand_height(getRootPtr(), width);
	}

	lv_span_coords_t LvSpanGroup::getSpanCoords(LvSpan& span) const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_spangroup_get_span_coords(getRootPtr(), static_cast<lv_span_t*>(span));
	}

	std::optional<LvSpanGroup::LvSpan> LvSpanGroup::getSpanByPoint(const lv_point_t& point) const
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_t* span = lv_spangroup_get_span_by_point(getRootPtr(), &point);
		if (span == nullptr)
		{
			return std::nullopt;
		}
		return LvSpan(const_cast<LvSpanGroup&>(*this), *span);
	}

	void LvSpanGroup::refresh()
	{
		ZoneScoped;
		UI_LOCK();
		lv_spangroup_refresh(getRootPtr());
	}

	void LvSpanGroup::LvSpan::setText(const std::string& text)
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_set_text(&m_span, text.c_str());
	}

	void LvSpanGroup::LvSpan::setTextStatic(const char* text)
	{
		ZoneScoped;
		UI_LOCK();
		lv_span_set_text_static(&m_span, text);
	}

	void LvSpanGroup::LvSpan::setStyle(const lv_style_t* style)
	{
		ZoneScoped;
		m_spangroup.setSpanStyle(*this, style);
	}

	void LvSpanGroup::LvSpan::setStyleStatic(const lv_style_t* style)
	{
		ZoneScoped;
		m_spangroup.setSpanStyleStatic(*this, style);
	}

	std::string_view LvSpanGroup::LvSpan::getText() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_span_get_text(&m_span);
	}

	const lv_style_t* LvSpanGroup::LvSpan::getStyle() const
	{
		ZoneScoped;
		UI_LOCK();
		return lv_span_get_style(&m_span);
	}

} // namespace UI

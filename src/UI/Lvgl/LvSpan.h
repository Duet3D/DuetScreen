/*
 * LvSpan.h
 *
 *  Created on: 2026-02-02
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvObj.h"
#include <optional>

namespace UI
{
	class LvSpanGroup : public LvObj
	{
	  public:
		class LvSpan
		{
		  public:
			LvSpan(LvSpanGroup& spangroup, lv_span_t& span)
				: m_spangroup(spangroup)
				, m_span(span)
			{
			}

			void setText(const std::string& text);
			void setTextStatic(const char* text);

			void setStyle(const lv_style_t* style);
			void setStyleStatic(const lv_style_t* style);

			std::string_view getText() const;
            const lv_style_t* getStyle() const;

            operator lv_span_t*() { return &m_span; }

		  private:
			LvSpanGroup& m_spangroup;
			lv_span_t& m_span;
		};

		LvSpanGroup(const std::string& name, LvObj& parent);

        LvSpan addSpan();
        void deleteSpan(LvSpan&& span);
		size_t removeSpansFromIndex(size_t index, bool allFollowing = false);

		void setSpanText(LvSpan& span, const std::string& text);
		void setSpanTextStatic(LvSpan& span, const char* text);
		void setSpanStyle(LvSpan& span, const lv_style_t* style);
		void setSpanStyleStatic(LvSpan& span, const lv_style_t* style);
		void setOverflow(lv_span_overflow_t overflow);
        void setIndent(int32_t indent);
        void setMaxLines(int32_t lines);
        void setMode(lv_span_mode_t mode);

		std::optional<LvSpan> getSpanByIndex(int32_t index);
		LvSpan getOrCreateSpanByIndex(size_t index);
		uint32_t getSpanCount() const;
        lv_span_overflow_t getOverflow() const;
        int32_t getIndent() const;
        int32_t getMaxLines() const;
        int32_t getMaxLineHeight() const;
        lv_span_mode_t getMode() const;
        uint32_t getExpandWidth(uint32_t max_width) const;
        int32_t getExpandHeight(int32_t width) const;
        lv_span_coords_t getSpanCoords(LvSpan& span) const;
		std::optional<LvSpan> getSpanByPoint(const lv_point_t& point) const;
		void refresh();

	  private:
	};
} // namespace UI

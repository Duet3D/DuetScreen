/*
 * Bar.cpp
 *
 *  Created on: 2026-01-09
 *      Author: Andy Everitt
 */

#include "Bar.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Bar::Bar(const std::string& name, LvObj& parent)
		: LvBar(name, parent)
	{
		ZoneScoped;
		UI_LOCK();

		// Update label text and position
		addEventCallback(barEvent, LV_EVENT_ALL, this);

		addStyle(Themes::getLvglStyles().pad_normal, LV_PART_INDICATOR);
		addStyle(Themes::getComponentStyles().bar_label_bg, LV_PART_CUSTOM_FIRST);
		addStyle(Themes::getComponentStyles().bar_label, LV_PART_CUSTOM_FIRST);
	}

	void Bar::setLabelFormat(const std::string& fmt)
	{
		ZoneScoped;
		UI_LOCK();
		m_labelFmt = fmt;

		refreshSelfSize();
	}

	void Bar::barEvent(lv_event_t* e)
	{
		ZoneScoped;
		[[maybe_unused]] Bar& self = *static_cast<Bar*>(lv_event_get_user_data(e));
		const lv_event_code_t code = lv_event_get_code(e);

		switch (code)
		{
		case LV_EVENT_STYLE_CHANGED:
		case LV_EVENT_SIZE_CHANGED:
		{
			self.refreshSelfSize();
			break;
		}
		case LV_EVENT_GET_SELF_SIZE:
		{
			lv_point_t* p = static_cast<lv_point_t*>(lv_event_get_param(e));

			[[maybe_unused]] const lv_coord_t indic_pad_top =
				lv_obj_get_style_pad_top(self.getRootPtr(), LV_PART_INDICATOR);
			[[maybe_unused]] const lv_coord_t indic_pad_bottom =
				lv_obj_get_style_pad_bottom(self.getRootPtr(), LV_PART_INDICATOR);
			[[maybe_unused]] const lv_coord_t indic_pad_left =
				lv_obj_get_style_pad_left(self.getRootPtr(), LV_PART_INDICATOR);
			[[maybe_unused]] const lv_coord_t indic_pad_right =
				lv_obj_get_style_pad_right(self.getRootPtr(), LV_PART_INDICATOR);

			[[maybe_unused]] const lv_coord_t label_pad_top =
				lv_obj_get_style_pad_top(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
			[[maybe_unused]] const lv_coord_t label_pad_bottom =
				lv_obj_get_style_pad_bottom(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
			[[maybe_unused]] const lv_coord_t label_pad_left =
				lv_obj_get_style_pad_left(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
			[[maybe_unused]] const lv_coord_t label_pad_right =
				lv_obj_get_style_pad_right(self.getRootPtr(), LV_PART_CUSTOM_FIRST);

			lv_draw_label_dsc_t label_dsc;
			lv_draw_label_dsc_init(&label_dsc);
			label_dsc.font = &Themes::getFonts().normal;

			char buf[32];
			auto result = fmt::format_to_n(buf, sizeof(buf) - 1, fmt::runtime(self.m_labelFmt), self.getValue());
			buf[result.size] = '\0';

			lv_point_t txt_size;
			lv_text_get_size(&txt_size,
							 buf,
							 label_dsc.font,
							 label_dsc.letter_space,
							 label_dsc.line_space,
							 self.getWidth() - (indic_pad_left + indic_pad_right),
							 label_dsc.flag);

			const lv_coord_t label_height = txt_size.y + label_pad_top + label_pad_bottom;

			p->y = std::max(p->y, label_height + indic_pad_top + indic_pad_bottom);
			break;
		}
		case LV_EVENT_DRAW_MAIN_END:
		{
			drawLabel(e);
			break;
		}
		default:
			break;
		}
	}

	void Bar::drawLabel(lv_event_t* e)
	{
		ZoneScoped;
		Bar& self = *static_cast<Bar*>(lv_event_get_user_data(e));
		lv_layer_t* layer = lv_event_get_layer(e);

		[[maybe_unused]] const lv_coord_t indic_pad_top =
			lv_obj_get_style_pad_top(self.getRootPtr(), LV_PART_INDICATOR);
		[[maybe_unused]] const lv_coord_t indic_pad_bottom =
			lv_obj_get_style_pad_bottom(self.getRootPtr(), LV_PART_INDICATOR);
		[[maybe_unused]] const lv_coord_t indic_pad_left =
			lv_obj_get_style_pad_left(self.getRootPtr(), LV_PART_INDICATOR);
		[[maybe_unused]] const lv_coord_t indic_pad_right =
			lv_obj_get_style_pad_right(self.getRootPtr(), LV_PART_INDICATOR);

		[[maybe_unused]] const lv_coord_t label_pad_top =
			lv_obj_get_style_pad_top(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
		[[maybe_unused]] const lv_coord_t label_pad_bottom =
			lv_obj_get_style_pad_bottom(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
		[[maybe_unused]] const lv_coord_t label_pad_left =
			lv_obj_get_style_pad_left(self.getRootPtr(), LV_PART_CUSTOM_FIRST);
		[[maybe_unused]] const lv_coord_t label_pad_right =
			lv_obj_get_style_pad_right(self.getRootPtr(), LV_PART_CUSTOM_FIRST);

		lv_draw_rect_dsc_t label_bg_dsc;
		lv_draw_rect_dsc_init(&label_bg_dsc);
		label_bg_dsc.base.layer = layer;
		lv_obj_init_draw_rect_dsc(self.getRootPtr(), LV_PART_CUSTOM_FIRST, &label_bg_dsc);

		lv_draw_label_dsc_t label_dsc;
		lv_draw_label_dsc_init(&label_dsc);
		label_dsc.font = &Themes::getFonts().normal;

		char buf[32];
		auto result = fmt::format_to_n(buf, sizeof(buf) - 1, fmt::runtime(self.m_labelFmt), self.getValue());
		buf[result.size] = '\0';

		lv_point_t txt_size;
		lv_text_get_size(&txt_size,
						 buf,
						 label_dsc.font,
						 label_dsc.letter_space,
						 label_dsc.line_space,
						 self.getWidth() - (indic_pad_left + indic_pad_right),
						 label_dsc.flag);

		lv_area_t txt_area{.x1 = 0, .y1 = 0, .x2 = txt_size.x - 1, .y2 = txt_size.y - 1};

		lv_area_t indic_area = self.getCoords();
		const int32_t range = self.getMaxValue() - self.getMinValue();

		const int32_t filled_width =
			std::clamp(static_cast<int32_t>((lv_area_get_width(&indic_area) * (self.getValue() - self.getMinValue())) /
											std::max(range, 1)),
					   0,
					   lv_area_get_width(&indic_area));
		lv_area_set_width(&indic_area, filled_width);

		lv_style_value_t prop_value;
		if (lv_style_get_prop(Themes::getLvglStyles().text, LV_STYLE_TEXT_COLOR, &prop_value) == LV_STYLE_RES_FOUND)
			label_dsc.color = prop_value.color;
		else
			label_dsc.color = lv_color_white();

		lv_area_t bg_area;
		lv_area_copy(&bg_area, &txt_area);
		lv_area_increase(&bg_area, label_pad_left + label_pad_right, label_pad_top + label_pad_bottom);

		/*If the indicator is long enough put the text inside on the right*/
		if (lv_area_get_width(&indic_area) > lv_area_get_width(&bg_area) + indic_pad_left + indic_pad_right)
		{
			lv_area_align(&indic_area, &bg_area, LV_ALIGN_RIGHT_MID, -indic_pad_right, 0);
			label_bg_dsc.bg_opa = LV_OPA_0;
		}
		/*If the indicator is still short put the text out of it on the right*/
		else
		{
			lv_area_align(&indic_area, &bg_area, LV_ALIGN_OUT_RIGHT_MID, indic_pad_right, 0);
		}

		lv_draw_rect(layer, &label_bg_dsc, &bg_area);

		lv_area_align(&bg_area, &txt_area, LV_ALIGN_CENTER, 0, 0);
		label_dsc.text = buf;
		label_dsc.text_local = true;
		lv_draw_label(layer, &label_dsc, &txt_area);
	}
} // namespace UI

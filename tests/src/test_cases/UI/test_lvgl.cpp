/*
 * test_lvgl.cpp
 *
 *  Created on: 2025-09-03
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

// using namespace UI;

class TestLvgl : public UiTestSuite
{
  public:
	TestLvgl() {}
};

TEST_F(TestLvgl, FlexSizeContentGrow)
{
	lv_obj_t* cont = lv_obj_create(screen);
	lv_obj_set_name(cont, "cont");
	lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_bg_color(cont, lv_color_hex(0xff0000), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

	const int32_t pad_all = lv_obj_get_style_pad_top(cont, LV_PART_MAIN);
	const int32_t pad_gap = lv_obj_get_style_pad_row(cont, LV_PART_MAIN);
	const int32_t space_top = lv_obj_get_style_space_top(cont, LV_PART_MAIN);
	const int32_t space_bottom = lv_obj_get_style_space_bottom(cont, LV_PART_MAIN);

	lv_obj_set_style_pad_all(cont, pad_all, 0);
	lv_obj_set_style_pad_gap(cont, pad_gap, 0);

	lv_obj_t* header = lv_label_create(cont);
	lv_obj_set_name(header, "header");
	lv_label_set_text(header, "header");

	lv_obj_t* item = lv_obj_create(cont);
	lv_obj_set_name(item, "item");
	lv_obj_set_width(item, LV_PCT(100));
	lv_obj_set_flex_grow(item, 1);
	lv_obj_set_style_bg_color(item, lv_color_hex(0x00ff00), 0);
	lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);

	lv_obj_t* footer = lv_label_create(cont);
	lv_obj_set_name(footer, "footer");
	lv_label_set_text(footer, "footer");

	lv_obj_update_layout(cont);
	const int32_t header_height = lv_obj_get_height(header);
	const int32_t footer_height = lv_obj_get_height(footer);

	EXPECT_EQ(header_height + footer_height + space_top + space_bottom + 2 * pad_gap, lv_obj_get_height(cont));
	EXPECT_EQ(0, lv_obj_get_height(item));

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content.png");

	lv_obj_set_style_min_height(item, 200, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_min_size.png");

	/* The min size of the cont should "override" the `LV_SIZE_CONTENT` height so item should be visible and grow to
	 * fill space */
	lv_obj_set_style_min_height(item, 0, LV_PART_MAIN);
	lv_obj_set_style_min_height(cont, 500, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_min_size_cont.png");

	/* The min size of the cont should "override" the `LV_SIZE_CONTENT` height so item should be visible and grow to
	 * fill space up to the max size */
	lv_obj_set_style_max_height(item, 200, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_max_size.png");

	/* item height should be 0 since min size is not set for item or cont, cont should not be max size since max size is
	 * larger than content */
	lv_obj_set_style_min_height(cont, 0, LV_PART_MAIN);
	lv_obj_set_style_max_height(item, 0, LV_PART_MAIN);
	lv_obj_set_style_max_height(cont, 500, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_max_size_cont.png");

	/* item height should be 0 since min size is not set for item or cont, cont should be max size since max size is
	 * smaller than content */
	lv_obj_set_style_max_height(item, 0, LV_PART_MAIN);
	lv_obj_set_style_max_height(cont, 40, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_max_size_cont2.png");

	/* item should be min height (50) and cont will be max height (40) so cont should be scrollable */
	lv_obj_set_style_min_height(item, 50, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_max_size_cont3.png");

	/* max cont height should be ignored here */
	lv_obj_set_style_max_height(cont, LV_PCT(70), LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_size_content_max_size_cont4.png");
}

TEST_F(TestLvgl, FlexFixedSize)
{
	lv_obj_t* cont = lv_obj_create(screen);
	lv_obj_set_name(cont, "cont");
	lv_obj_set_size(cont, LV_PCT(100), 200);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_bg_color(cont, lv_color_hex(0xff0000), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

	lv_obj_t* header = lv_label_create(cont);
	lv_obj_set_name(header, "header");
	lv_label_set_text(header, "header");
	lv_obj_set_height(header, 30);

	lv_obj_t* item = lv_obj_create(cont);
	lv_obj_set_name(item, "item");
	lv_obj_set_width(item, LV_PCT(100));
	lv_obj_set_flex_grow(item, 1);
	lv_obj_set_style_bg_color(item, lv_color_hex(0x00ff00), 0);
	lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);

	lv_obj_t* footer = lv_label_create(cont);
	lv_obj_set_name(footer, "footer");
	lv_label_set_text(footer, "footer");
	lv_obj_set_height(footer, 30);

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_col_grow_fixed_size_cont.png");
}

TEST_F(TestLvgl, FlexSizeContentGrow2)
{
	lv_obj_t* cont = lv_obj_create(screen);
	lv_obj_set_name(cont, "cont");
	lv_obj_set_style_min_width(cont, LV_SIZE_CONTENT, LV_PART_MAIN);
	lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_bg_color(cont, lv_color_hex(0xff0000), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

	lv_obj_t* label = lv_label_create(cont);
	lv_obj_set_name(label, "label");
	lv_label_set_text(label, "label");

	lv_obj_t* sub_cont = lv_obj_create(cont);
	lv_obj_set_name(sub_cont, "sub_cont");
	lv_obj_set_height(sub_cont, LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(sub_cont, 1);
	lv_obj_set_flex_flow(sub_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_style_bg_color(sub_cont, lv_color_hex(0x00ff00), 0);
	lv_obj_set_style_bg_opa(sub_cont, LV_OPA_COVER, 0);

	for (size_t i = 0; i < 5; i++)
	{
		lv_obj_t* item = lv_label_create(sub_cont);
		lv_obj_set_name(item, "item_#");
		lv_label_set_text(item, fmt::format("item_{}", i).c_str());
		lv_obj_set_style_text_color(item, lv_color_black(), 0);
	}

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_grow_size_content2.png");

	lv_obj_set_style_max_width(sub_cont, LV_SIZE_CONTENT, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_grow_size_content_max_size_content.png");

	for (size_t i = 0; i < 20; i++)
	{
		lv_obj_t* item = lv_label_create(sub_cont);
		lv_obj_set_name(item, "item_#");
		lv_label_set_text(item, fmt::format("item_{}", i).c_str());
		lv_obj_set_style_text_color(item, lv_color_black(), 0);
	}

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_grow_size_content_max_size_content_wrap.png");
	lv_obj_set_style_max_width(sub_cont, LV_COORD_MAX, LV_PART_MAIN);
	lv_obj_update_layout(sub_cont);
	lv_obj_set_style_max_width(sub_cont, LV_SIZE_CONTENT, LV_PART_MAIN);
	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_grow_size_content_max_size_content_wrap2.png");
}

static lv_obj_t* create_cont(
	std::string_view name, lv_obj_t* parent, lv_flex_flow_t flow, lv_coord_t width, lv_coord_t height)
{
	lv_obj_t* cont = lv_obj_create(parent);
	lv_obj_set_flex_flow(cont, flow);
	lv_obj_set_size(cont, width, height);
	lv_obj_set_name(cont, name.data());
	lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	return cont;
}

static lv_obj_t* create_item(std::string_view name, lv_obj_t* parent)
{
	lv_obj_t* item = lv_obj_create(parent);
	lv_obj_set_name(item, name.data());
	lv_obj_set_style_bg_color(item, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
	lv_obj_set_width(item, LV_PCT(100));
	return item;
}

TEST_F(TestLvgl, FlexPadding)
{
	lv_obj_t* parent = lv_obj_create(screen);
	lv_obj_set_size(parent, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_style_pad_all(parent, 10, 0);

	const lv_coord_t col_width = 100;

	/* Empty container */
	{
		lv_label_set_text(lv_label_create(parent), "cont_empty");
		lv_obj_t* cont = create_cont("cont_empty", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);
	}

	/* Container with single fixed size obj */
	{
		lv_label_set_text(lv_label_create(parent), "cont_fixed_single");
		lv_obj_t* cont = create_cont("cont_fixed_single", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		lv_obj_t* item = create_item("item_fixed", cont);
		lv_obj_set_height(item, 30);
	}

	/* Container with single grow obj */
	{
		lv_label_set_text(lv_label_create(parent), "cont_grow_single");
		lv_obj_t* cont = create_cont("cont_grow_single", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		lv_obj_t* item = create_item("item_grow", cont);
		lv_obj_set_flex_grow(item, 1);
	}

	/* Container with multiple fixed size */
	{
		lv_label_set_text(lv_label_create(parent), "cont_fixed_multi");
		lv_obj_t* cont = create_cont("cont_fixed_multi", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		for (size_t i = 0; i < 3; i++)
		{
			lv_obj_t* item = create_item(fmt::format("item_fixed_{}", i), cont);
			lv_obj_set_height(item, 30);
		}
	}

	/* Container with multiple grow */
	{
		lv_label_set_text(lv_label_create(parent), "cont_grow_multi");
		lv_obj_t* cont = create_cont("cont_grow_multi", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		for (size_t i = 0; i < 3; i++)
		{
			lv_obj_t* item = create_item(fmt::format("item_grow_{}", i), cont);
			lv_obj_set_flex_grow(item, 1);
		}
	}

	/* Container with grow then fixed */
	{
		lv_label_set_text(lv_label_create(parent), "cont_grow_fixed");
		lv_obj_t* cont = create_cont("cont_grow_fixed", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		lv_obj_t* item_grow = create_item("item_grow", cont);
		lv_obj_set_flex_grow(item_grow, 1);

		lv_obj_t* item_fixed = create_item("item_fixed", cont);
		lv_obj_set_height(item_fixed, 30);
	}

	/* Container with fixed then grow */
	{
		lv_label_set_text(lv_label_create(parent), "cont_fixed_grow");
		lv_obj_t* cont = create_cont("cont_fixed_grow", parent, LV_FLEX_FLOW_COLUMN, col_width, LV_SIZE_CONTENT);

		lv_obj_t* item_fixed = create_item("item_fixed", cont);
		lv_obj_set_height(item_fixed, 30);

		lv_obj_t* item_grow = create_item("item_grow", cont);
		lv_obj_set_flex_grow(item_grow, 1);
	}

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_padding.png");
}

TEST_F(TestLvgl, GridSizeContent)
{
	lv_obj_t* cont = lv_obj_create(screen);
	lv_obj_set_name(cont, "cont");

	int32_t col_dsc[3] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
	int32_t row_dsc[3] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

	lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
	lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

	lv_obj_t* item1 = lv_obj_create(cont);
	lv_obj_set_name(item1, "item1");
	lv_obj_set_style_bg_color(item1, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item1, LV_OPA_COVER, 0);
	lv_obj_set_size(item1, LV_PCT(10), LV_PCT(100));
	lv_obj_set_grid_cell(item1, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);

	lv_obj_t* item2 = lv_obj_create(cont);
	lv_obj_set_name(item2, "item2");
	lv_obj_set_style_bg_color(item2, lv_palette_main(LV_PALETTE_BLUE), 0);
	lv_obj_set_style_bg_opa(item2, LV_OPA_COVER, 0);
	lv_obj_set_height(item2, LV_SIZE_CONTENT);
	lv_obj_set_grid_cell(item2, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 0, 1);

	lv_obj_t* label = lv_label_create(item2);
	lv_obj_set_name(label, "label");
	lv_label_set_text(label, "This is a label");

	lv_obj_t* item3 = lv_obj_create(cont);
	lv_obj_set_name(item3, "item3");
	lv_obj_set_style_bg_color(item3, lv_palette_main(LV_PALETTE_YELLOW), 0);
	lv_obj_set_style_bg_opa(item3, LV_OPA_COVER, 0);
	lv_obj_set_grid_cell(item3, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

	lv_obj_update_layout(cont);
	EXPECT_EQUAL_SCREENSHOT("lvgl/grid_size_content.png");
}
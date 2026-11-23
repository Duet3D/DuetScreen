/*
 * test_lvgl.cpp
 *
 *  Created on: 2025-09-03
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Lvgl/LvTextarea.h"
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
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
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
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
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
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
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
	lv_obj_set_style_outline_color(cont, lv_color_black(), 0);
	lv_obj_set_style_outline_width(cont, 2, 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	return cont;
}

static lv_obj_t* create_item(std::string_view name, lv_obj_t* parent)
{
	lv_obj_t* item = lv_obj_create(parent);
	lv_obj_set_name(item, name.data());
	lv_obj_set_style_bg_color(item, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
	lv_obj_set_style_outline_color(item, lv_color_black(), 0);
	lv_obj_set_style_outline_width(item, 2, 0);
	lv_obj_set_width(item, LV_PCT(100));
	return item;
}

#if 0
/**
* This is a test to try to cause a lvgl layout bug observed with the babystepping increment value buttons.
* I could not replicate the issue here so the test is disabled for now
*/
TEST_F(TestLvgl, FlexGrowMinSize)
{
	lv_obj_t* parent = create_cont("parent", screen.getRootPtr(), LV_FLEX_FLOW_COLUMN, 200, 200);

	lv_obj_t* cont = create_cont("cont", parent, LV_FLEX_FLOW_COLUMN, LV_PCT(100), LV_PCT(100));
	lv_obj_set_flex_grow(cont, 1);
	lv_obj_set_style_min_height(cont, LV_SIZE_CONTENT, LV_PART_MAIN);

	lv_obj_t* item = create_item("item", cont);
	lv_obj_set_flex_grow(item, 50);
	lv_obj_set_style_min_height(item, LV_SIZE_CONTENT, LV_PART_MAIN);

	lv_obj_t* sub_cont = create_cont("sub_cont", cont, LV_FLEX_FLOW_ROW, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(sub_cont, 1);
	lv_obj_set_style_bg_color(sub_cont, lv_color_hex(0x0000ff), 0);

	lv_obj_t* sub_item = create_item("sub_item", sub_cont);
	lv_obj_set_height(sub_item, LV_PCT(100));
	lv_obj_set_style_min_height(sub_item, LV_SIZE_CONTENT, LV_PART_MAIN);
	lv_obj_set_flex_grow(sub_item, 1);
	lv_obj_set_style_bg_color(sub_item, lv_color_hex(0xffff00), 0);

	lv_obj_t* sub_item2 = create_item("sub_item2", sub_cont);
	lv_obj_set_height(sub_item2, LV_PCT(100));
	lv_obj_set_style_min_height(sub_item2, LV_SIZE_CONTENT, LV_PART_MAIN);
	lv_obj_set_flex_grow(sub_item2, 1);
	lv_obj_set_style_bg_color(sub_item2, lv_color_hex(0xffff00), 0);

	lv_label_create(sub_item);

	EXPECT_EQUAL_SCREENSHOT("lvgl/flex_grow_min_size.png");
}
#endif

TEST_F(TestLvgl, FlexPadding)
{
	lv_obj_t* parent = lv_obj_create(screen.getRootPtr());
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
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
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

TEST_F(TestLvgl, CircularHeight)
{
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
	lv_obj_set_name(cont, "cont");
	lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);

	lv_obj_t* item1 = lv_obj_create(cont);
	lv_obj_set_name(item1, "item1");
	lv_obj_set_style_bg_color(item1, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item1, LV_OPA_COVER, 0);
	// because parent is size content this will evaluate to 0 but ideally should be 50 because of item2
	lv_obj_set_height(item1, LV_PCT(100));
	lv_obj_set_flex_grow(item1, 1);

	lv_obj_t* item2 = lv_obj_create(cont);
	lv_obj_set_name(item2, "item2");
	lv_obj_set_style_bg_color(item2, lv_palette_main(LV_PALETTE_BLUE), 0);
	lv_obj_set_style_bg_opa(item2, LV_OPA_COVER, 0);
	lv_obj_set_height(item2, 50); // fixed size
	lv_obj_set_flex_grow(item2, 1);

	lv_obj_t* item3 = lv_obj_create(cont);
	// lv_obj_set_flag(item3, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	lv_obj_set_name(item3, "item3");
	lv_obj_set_style_bg_color(item3, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item3, LV_OPA_COVER, 0);
	lv_obj_set_height(item3, LV_PCT(100)); // same as item1 but checking if child order matters
	lv_obj_set_flex_grow(item3, 1);

	EXPECT_EQUAL_SCREENSHOT("lvgl/circular_height.png");

	lv_obj_set_height(item2, 40);
	EXPECT_EQUAL_SCREENSHOT("lvgl/circular_height2.png");
}

TEST_F(TestLvgl, CircularWidth)
{
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
	lv_obj_set_name(cont, "cont");
	lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_PCT(100));
	lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* item1 = lv_obj_create(cont);
	lv_obj_set_name(item1, "item1");
	lv_obj_set_style_bg_color(item1, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item1, LV_OPA_COVER, 0);
	// because parent is size content this will evaluate to 0 but ideally should be 50 because of item2
	lv_obj_set_width(item1, LV_PCT(100));
	lv_obj_set_flex_grow(item1, 1);

	lv_obj_t* item2 = lv_obj_create(cont);
	lv_obj_set_name(item2, "item2");
	lv_obj_set_style_bg_color(item2, lv_palette_main(LV_PALETTE_BLUE), 0);
	lv_obj_set_style_bg_opa(item2, LV_OPA_COVER, 0);
	lv_obj_set_width(item2, 50); // fixed size
	lv_obj_set_flex_grow(item2, 1);

	lv_obj_t* item3 = lv_obj_create(cont);
	// lv_obj_set_flag(item3, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	lv_obj_set_name(item3, "item3");
	lv_obj_set_style_bg_color(item3, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(item3, LV_OPA_COVER, 0);
	lv_obj_set_width(item3, LV_PCT(100)); // same as item1 but checking if child order matters
	lv_obj_set_flex_grow(item3, 1);

	EXPECT_EQUAL_SCREENSHOT("lvgl/circular_width.png");

	lv_obj_set_width(item2, 40);
	EXPECT_EQUAL_SCREENSHOT("lvgl/circular_width2.png");
}

TEST_F(TestLvgl, DropdownSizeContent)
{
	lv_obj_t* cont = lv_obj_create(lv_screen_active());
	lv_obj_set_name(cont, "cont");
	lv_obj_set_size(cont, 300, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_RED), 0);
	lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

	lv_obj_t* label = lv_label_create(cont);
	lv_obj_set_name(label, "label");
	lv_label_set_text(label, "Dropdown with size content:");

#if 1
	lv_obj_t* sub_cont = lv_obj_create(cont);
	lv_obj_set_name(sub_cont, "sub_cont");
	lv_obj_set_style_bg_color(sub_cont, lv_palette_main(LV_PALETTE_GREEN), 0);
	lv_obj_set_style_bg_opa(sub_cont, LV_OPA_COVER, 0);
	lv_obj_set_height(sub_cont, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(sub_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_grow(sub_cont, 1);
	lv_obj_set_style_min_width(sub_cont, LV_SIZE_CONTENT, LV_PART_MAIN);

	lv_obj_t* dd = lv_dropdown_create(sub_cont);
#else
	lv_obj_t* dd = lv_dropdown_create(cont);
#endif
	lv_obj_set_name(dd, "dropdown");
	lv_dropdown_set_options(dd, "Short\nA bit longer option\nThe longest option in the list");
	lv_obj_set_width(dd, 0);
	lv_obj_set_style_min_width(dd, LV_SIZE_CONTENT, 0);
	lv_obj_set_flex_grow(dd, 1);
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_1.png");

#if 1
	lv_dropdown_open(dd);
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_1_open.png");

	lv_test_mouse_click_at(300, 75); // select 2nd option
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_2.png");
#endif

	// lv_dropdown_set_selected(dd, 1);
	// EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_2.png");

	lv_dropdown_set_selected(dd, 2);
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_3.png");

	lv_dropdown_set_text(dd, "text");
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_text.png");

	lv_label_set_text(label, "label");
	EXPECT_EQUAL_SCREENSHOT("lvgl/dropdown/content_size_text2.png");
}

TEST_F(TestLvgl, TextareaOneLine)
{
	UI::LvTextarea textarea("textarea", screen);

	textarea.setText("Type here...");
	textarea.setWidth(100);
	textarea.setHeight(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("lvgl/textarea/one_line.png");
}

TEST_F(TestLvgl, ScaleSize)
{
	lv_obj_t* cont = lv_obj_create(screen.getRootPtr());
	lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

	auto createScale = [&](std::string_view name, lv_scale_mode_t mode)
	{
		lv_obj_t* scale = lv_scale_create(cont);
		lv_obj_set_name(scale, name.data());
		switch (mode)
		{
		case LV_SCALE_MODE_HORIZONTAL_TOP:
		case LV_SCALE_MODE_HORIZONTAL_BOTTOM:
			lv_obj_set_height(scale, LV_SIZE_CONTENT);
			break;
		case LV_SCALE_MODE_VERTICAL_LEFT:
		case LV_SCALE_MODE_VERTICAL_RIGHT:
			lv_obj_set_width(scale, LV_SIZE_CONTENT);
			break;
		default:
			break;
		}
		lv_scale_set_mode(scale, mode);
		lv_scale_set_label_show(scale, true);
		lv_obj_set_style_border_width(scale, 2, LV_PART_MAIN);
		lv_obj_set_style_border_color(scale, lv_color_white(), LV_PART_MAIN);

		return scale;
	};

	lv_obj_t* scale = createScale("scale_horizontal_top", LV_SCALE_MODE_HORIZONTAL_TOP);
	lv_obj_t* scale2 = createScale("scale_horizontal_bottom", LV_SCALE_MODE_HORIZONTAL_BOTTOM);
	lv_obj_t* scale3 = createScale("scale_vertical_left", LV_SCALE_MODE_VERTICAL_LEFT);
	lv_obj_t* scale4 = createScale("scale_vertical_right", LV_SCALE_MODE_VERTICAL_RIGHT);

	auto setTickLength = [&](lv_obj_t* scale, int32_t major_len, int32_t minor_len)
	{
		lv_obj_set_style_length(scale, major_len, LV_PART_INDICATOR);
		lv_obj_set_style_length(scale, minor_len, LV_PART_ITEMS);
		return scale;
	};

	lv_obj_t* scale6 =
		setTickLength(createScale("scale_horizontal_top_long_tick", LV_SCALE_MODE_HORIZONTAL_TOP), 30, 5);
	lv_obj_set_flag(scale6, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	lv_obj_t* scale7 =
		setTickLength(createScale("scale_horizontal_bottom_long_tick", LV_SCALE_MODE_HORIZONTAL_BOTTOM), 30, 5);
	lv_obj_t* scale8 = setTickLength(createScale("scale_vertical_left_long_tick", LV_SCALE_MODE_VERTICAL_LEFT), 30, 5);
	lv_obj_t* scale9 =
		setTickLength(createScale("scale_vertical_right_long_tick", LV_SCALE_MODE_VERTICAL_RIGHT), 30, 5);

	EXPECT_EQUAL_SCREENSHOT("lvgl/scale_size.png");
}

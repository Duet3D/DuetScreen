/*
 * test_lazy_list.cpp
 *
 *  Created on: 2025-12-02
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Widgets/Layout/LazyList.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class LazyLabel : public LazyObj<LvLabel>
{
  public:
	int32_t getWidth() const override { return 0; }
	int32_t getHeight() const override { return 30; }
	void update(size_t /* index */, LvLabel& obj) override { obj.setText(m_text); }

	void setText(const std::string& text) { m_text = text; }

  private:
	std::string m_text;
};

class LazyBox : public LazyObj<LvContainer>
{
  public:
	LazyBox(lv_coord_t width, lv_coord_t height)
		: m_width(width)
		, m_height(height)
	{
	}

	int32_t getWidth() const override { return m_width; }
	int32_t getHeight() const override { return m_height; }
	void update(size_t /* index */, LvContainer& obj) override
	{
		obj.addStyle(Themes::getLvglStyles().bg_color_primary);
		obj.addStyle(Themes::getLvglStyles().border);
	}

  private:
	lv_coord_t m_width;
	lv_coord_t m_height;
};

class TestLazyList : public UiTestSuite
{
  public:
	TestLazyList() {}
};

TEST_F(TestLazyList, Basic)
{
	LazyList<LazyLabel> list("lazy_list", screen);

	list.setSize(100, 100);
	list.setItemCount(10,
					  [](size_t index)
					  {
						  auto lazyLabel = std::make_unique<LazyLabel>();
						  lazyLabel->setText(fmt::format("Item {:d}", index));
						  return lazyLabel;
					  });

	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_basic.png");
	EXPECT_EQ(list.getLazyItems().size(), 10);
	EXPECT_EQ(list.getVisibleItems().size(), 4);
	/**
	 * The capacity indicates whether the amount of visible items was ever set to 10 in which case there is no advantage
	 * over a regular list. This could happen when the list size is first calculated if the list container uses
	 * LV_SIZE_CONTENT.
	 */
	EXPECT_EQ(list.getVisibleItems().capacity(), 4);

	list.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_basic_content_size.png");
	EXPECT_EQ(list.getLazyItems().size(), 10);
	EXPECT_EQ(list.getVisibleItems().size(), 10);
	EXPECT_EQ(list.getVisibleItems().capacity(), 10);
}

TEST_F(TestLazyList, RowWrap)
{
	LazyList<LazyBox> list("lazy_list_row_wrap", screen);

	list.setSize(120, 100);
	list.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
	list.setListPad(5, LV_PART_MAIN, Padding::VERTICAL);
	list.setListPad(0, LV_PART_MAIN, Padding::ROW);
	list.setListPad(0, LV_PART_MAIN, Padding::COLUMN);
	list.setItemCount(10, [](size_t /* index */) { return std::make_unique<LazyBox>(50, 30); });
	list.getListContainer().setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
	list.getListContainer().setStyleBgOpa(LV_OPA_COVER);

	ASSERT_GT(list.getVisibleItems().size(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getX(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getY(), 0);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_row_wrap.png");

	list.setHeight(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_row_wrap_content_height.png");
}

TEST_F(TestLazyList, ColumnWrap)
{
	LazyList<LazyBox> list("lazy_list_column_wrap", screen);

	list.setSize(100, 100);
	list.setListFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	list.setListPad(0, LV_PART_MAIN, Padding::ROW);
	list.setListPad(0, LV_PART_MAIN, Padding::COLUMN);
	list.setItemCount(10, [](size_t /* index */) { return std::make_unique<LazyBox>(50, 30); });
	list.getListContainer().setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
	list.getListContainer().setStyleBgOpa(LV_OPA_COVER);

	ASSERT_GT(list.getVisibleItems().size(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getX(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getY(), 0);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_column_wrap.png");

	list.setWidth(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_column_wrap_content_width.png");
}

TEST_F(TestLazyList, RowWrapWithPadding)
{
	LazyList<LazyBox> list("lazy_list_row_wrap_padding", screen);

	list.setSize(220, 100);
	list.setListFlow(LV_FLEX_FLOW_ROW_WRAP);
	list.setListPad(5, LV_PART_MAIN, Padding::ROW);
	list.setListPad(10, LV_PART_MAIN, Padding::COLUMN);
	list.setItemCount(10, [](size_t /* index */) { return std::make_unique<LazyBox>(50, 30); });
	list.getListContainer().setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
	list.getListContainer().setStyleBgOpa(LV_OPA_COVER);

	EXPECT_EQ(list.getListContainer().getStyleProp(LV_STYLE_PAD_ROW).num, 5);
	EXPECT_EQ(list.getListContainer().getStyleProp(LV_STYLE_PAD_COLUMN).num, 10);
	ASSERT_GT(list.getVisibleItems().size(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getX(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getY(), 0);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_row_wrap_padding.png");

	list.setHeight(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_row_wrap_padding_content_height.png");
}

TEST_F(TestLazyList, ColumnWrapWithPadding)
{
	LazyList<LazyBox> list("lazy_list_column_wrap_padding", screen);

	list.setSize(100, 120);
	list.setListFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	list.setListPad(7, LV_PART_MAIN, Padding::ROW);
	list.setListPad(9, LV_PART_MAIN, Padding::COLUMN);
	list.setItemCount(10, [](size_t /* index */) { return std::make_unique<LazyBox>(50, 30); });
	list.getListContainer().setStyleBgColor(lv_palette_main(LV_PALETTE_RED));
	list.getListContainer().setStyleBgOpa(LV_OPA_COVER);

	EXPECT_EQ(list.getListContainer().getStyleProp(LV_STYLE_PAD_ROW).num, 7);
	EXPECT_EQ(list.getListContainer().getStyleProp(LV_STYLE_PAD_COLUMN).num, 9);
	ASSERT_GT(list.getVisibleItems().size(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getX(), 0);
	EXPECT_EQ(list.getVisibleItem(0)->getY(), 0);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_column_wrap_padding.png");

	list.setWidth(LV_SIZE_CONTENT);
	EXPECT_EQUAL_SCREENSHOT("list/lazy_list_column_wrap_padding_content_width.png");
}

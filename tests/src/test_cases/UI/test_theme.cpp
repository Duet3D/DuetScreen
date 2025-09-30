/*
 * test_theme.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/AxisControl/GenericAxisControl.h"
#include "UI/Components/AxisControl/XYControl.h"
#include "UI/Components/Canvas/Canvas.h"
#include "UI/Components/Graph/Graph.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/LVGL/LvArc.h"
#include "UI/Components/LVGL/LvArcLabel.h"
#include "UI/Components/LVGL/LvBar.h"
#include "UI/Components/LVGL/LvButtonMatrix.h"
#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvDropdown.h"
#include "UI/Components/LVGL/LvImage.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvList.h"
#include "UI/Components/LVGL/LvSlider.h"
#include "UI/Components/LVGL/LvTextArea.h"
#include "UI/Components/MessageBox/AlertMessageBox.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Theme/ThemePreview.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Themes/DefaultTheme.h"
#include "UI/Widgets/SideBar/SideBar.h"
#include "UI/Widgets/Temperature/HeaterSlider.h"
#include "UI/Widgets/ToolList/ToolList.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

using namespace UI;

class TestTheme : public UiTestSuite
{
  public:
	TestTheme() {}

	~TestTheme()
	{
		const Themes::Theme* default_theme = Themes::getTheme(StorageHelper::getData<int>(ID_THEME, 0));
		if (default_theme == nullptr)
		{
			return;
		}
		default_theme->setThemeActive();
	}
};

TEST_F(TestTheme, DefaultTheme)
{
	constexpr uint16_t primaryHue = 210;
	constexpr uint16_t secondaryHue = 50;
	constexpr float chroma = 0.02f;
	const lv_font_t* font = LV_FONT_DEFAULT;
	constexpr bool darkMode = true;

	Themes::ThemeColors colors = Themes::createThemeColors(primaryHue, secondaryHue, chroma, darkMode);
	Themes::DefaultTheme theme("test_theme", colors, font, darkMode, [](Themes::Theme* theme) {});
	theme.init();
	theme.setThemeActive();

	ThemePreview themePreview("theme_preview", screen);
	themePreview.setSize(LV_PCT(100), LV_SIZE_CONTENT);

	EXPECT_TRUE(Themes::getCurrentTheme() == &theme);
	themePreview.setPrimaryHue(primaryHue);
	themePreview.setSecondaryHue(secondaryHue);
	themePreview.setChroma(chroma);
	themePreview.setDarkMode(darkMode);
	EXPECT_EQUAL_SCREENSHOT("theme_1.png");

	themePreview.setPrimaryHue(25);
	themePreview.setSecondaryHue(150);
	themePreview.setChroma(0.5f);
	themePreview.setDarkMode(false);
	EXPECT_EQUAL_SCREENSHOT("theme_2.png");

	themePreview.setPrimaryHue(300);
	themePreview.setSecondaryHue(50);
	themePreview.setChroma(0.5f);
	themePreview.setDarkMode(true);
	EXPECT_EQUAL_SCREENSHOT("theme_3.png");
}

static std::unique_ptr<LvLabel> createLabel(const std::string& text, LvObj& parent)
{
	auto label = std::make_unique<LvLabel>("label", parent);
	label->setText(text);
	label->setAlign(LV_ALIGN_CENTER, 0, 0);
	label->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	label->setMaxWidth(LV_PCT(100));
	return label;
}

TEST_F(TestTheme, Widgets)
{
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

	ThemePreview preview("theme_preview", screen);
	preview.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	preview.showControls(false);

	LvContainer cont("container", screen);
	cont.setWidth(LV_PCT(100));
	cont.setFlexGrow(1);
	cont.setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	cont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

	constexpr size_t col_width = LV_PCT(20);

	/* LvBar */
	LvBar bar("bar", cont);
	bar.setWidth(col_width);
	bar.setValue(40);

	/* LvButtonMatrix */
	LvButtonMatrix button_matrix("button_matrix", cont);
	const char* btnm_map[] = {"Default", "Checked", "\0"};
	const lv_buttonmatrix_ctrl_t btnm_ctrl_map[] = {
		LV_BUTTONMATRIX_CTRL_WIDTH_1,
		LV_BUTTONMATRIX_CTRL_CHECKED,
	};
	button_matrix.setMap(btnm_map);
	button_matrix.setCtrlMap(btnm_ctrl_map);
	button_matrix.setSize(col_width, 100);

	/* LvCheckbox */
	LvCheckbox checkbox("checkbox", cont);
	LvCheckbox checkbox2("checkbox2", cont);
	checkbox2.setChecked(true);

	/* LvImage */
	LvImage image("image", cont);

	LvLabel label("label", cont);
	label.setText("Label");

	/* LvSlider */
	LvSlider slider("slider", cont);
	slider.setWidth(col_width);
	slider.setValue(50);

	/* LvDropdown */
	LvDropdown dropdown("dropdown", cont);
	dropdown.open();

	/* LvTextArea */
	LvTextArea text_area("text_area", cont);
	text_area.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	text_area.setSize(col_width, LV_SIZE_CONTENT);
	text_area.setText("This is a text area. You can type here.");

	/* LvKeyboard */
	LvKeyboard keyboard("keyboard", cont);
	keyboard.setWidth(250);

	/* LvArc */
	LvContainer arc_cont("arc_cont", cont);
	arc_cont.setSize(col_width, 100);
	// arc_cont.setFlexGrow(1);
	arc_cont.setFlexFlow(LV_FLEX_FLOW_ROW);

	LvArc arc("arc", arc_cont);
	arc.setSize(LV_PCT(40), LV_PCT(100));

	LvArcLabel arc_label("arc_label", arc_cont);
	arc_label.setRadius(30);
	arc_label.setAngleStart(270);
	arc_label.setAngleSize(180);
	arc_label.setSize(LV_PCT(40), LV_PCT(100));
	arc_label.setText("Arc label");

	/* LvList */
	LvList list("list", cont);
	list.setSize(LV_PCT(30), LV_SIZE_CONTENT);
	list.addText("List text");
	list.addButton(nullptr, "List button");
	list.addButton(nullptr, "List button 2");

	/* Button */
	LvContainer btn_cont("btn_cont", cont);
	btn_cont.setSize(LV_PCT(30), LV_SIZE_CONTENT);
	btn_cont.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);

	Button button("button", btn_cont, "Button");

	Button button_checked("button_checked", btn_cont, "Checked");
	button_checked.setChecked(true);

	Button long_press("long_press", btn_cont, "Long Press");
	long_press.addStyle(Themes::getLvglStyles().long_press);

	Button action("action", btn_cont, "Action");
	action.addStyle(Themes::getLvglStyles().actionBtn);

	DraggableButton draggable("draggable", btn_cont, "Draggable");
	DraggableButton dragging("dragging", btn_cont, "Dragging");
	dragging.setState(LV_STATE_PRESSED, true);

	/* Card */
	Card card("card", cont);
	card.setSize(LV_PCT(30), LV_SIZE_CONTENT);
	auto card_label = createLabel("Card", card);

	/* File & Folder */
	LvContainer file_folder_cont("file_folder_cont", cont);
	file_folder_cont.setWidth(LV_PCT(30));
	file_folder_cont.setFlexGrow(1);
	file_folder_cont.setFlexFlow(LV_FLEX_FLOW_ROW);

	FileView file_view(file_folder_cont);
	file_view.hide();

	FileView::FileItem file_item(0, file_folder_cont, file_view);
	file_item.setFlexGrow(1);
	// file_item.setHeight(LV_PCT(100));
	file_item.setType(false);
	file_item.setFileLabel("File name");
	file_item.setFileDate("2025-08-20 12:37:10");
	file_item.setFileSize("671 KB");

	FileView::FileItem folder_item(0, file_folder_cont, file_view);
	folder_item.setFlexGrow(1);
	// folder_item.setHeight(LV_PCT(100));
	folder_item.setType(true);
	folder_item.setFileLabel("Folder name");
	folder_item.setFileDate("2025-08-20 12:39:59");
	folder_item.setFileSize("0 B");

	/* Graph */
	Graph graph("graph", cont);
	graph.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	graph.setWidth(250);
	graph.showLegend(true);
	srand(0);
	for (size_t i = 0; i < 3; ++i)
	{
		graph.createSeries(lv_palette_main((lv_palette_t)graph.getSeriesCount()), fmt::format("Series {}", i));
		for (size_t j = 0; j < 100; ++j)
		{
			graph.addData(i, rand() % 100);
		}
	}
	graph.getSeries(0)->legendObj->setText("Hidden");
	graph.showSeries(0, false);

	/* Canvas */
	Canvas canvas("canvas", cont);
	canvas.setSize(col_width, LV_SIZE_CONTENT);
	canvas.setTitle("Canvas");
	canvas.setResolution(100, 50);

	/* lv_table */
	lv_obj_t* table = lv_table_create(cont);
	lv_obj_set_size(table, col_width, LV_SIZE_CONTENT);
	const size_t table_cols = 3;
	const size_t table_rows = 3;
	lv_table_set_column_count(table, table_cols);
	lv_table_set_row_count(table, table_rows);
	lv_table_set_selected_cell(table, 1, 1);
	for (size_t i = 0; i < table_cols * table_rows; i++)
	{
		lv_table_set_cell_value(table, i % table_cols, i / table_cols, fmt::format("Cell {}", i).c_str());
	}

	/* Page 2 */

	LvContainer cont2("container", screen);
	cont2.setWidth(LV_PCT(100));
	cont2.setFlexGrow(1);
	int32_t cont2_cols[11];
	for (int i = 0; i < std::size(cont2_cols) - 1; ++i)
	{
		cont2_cols[i] = LV_GRID_FR(1);
	}
	cont2_cols[std::size(cont2_cols) - 1] = LV_GRID_TEMPLATE_LAST;
	int32_t cont2_rows[5] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
	cont2.setGridDsc(cont2_cols, cont2_rows);

	/* Numberpad */
	NumberPad numberpad("numberpad", cont2, layout_t{0, 0, 20, 100});
	cont2.setGridCell(numberpad, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 4);
	numberpad.setHeader("Numberpad");

	/* Sidebar */
	SideBar sidebar("sidebar", cont2);
	cont2.setGridCell(sidebar, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 4);

	/* Message Box */
	MessageBox message_box("message_box", cont2, layout_t(0, 0, 0, 0));
	cont2.setGridCell(message_box, LV_GRID_ALIGN_STRETCH, 3, 2, LV_GRID_ALIGN_STRETCH, 0, 2);
	message_box.setTitle("Message Box Title");
	message_box.setText("Message Box Text");
	message_box.cancelVisible(true);
	message_box.okVisible(true);
	message_box.imageVisible(true);
	message_box.setImage(IMAGE_ASSET("examples/example.bmp"));
	message_box.progressVisible(true);
	message_box.setProgress(40);

	/* Heater Slider */
	HeaterSlider heater_slider("heater_slider", cont2);
	cont2.setGridCell(heater_slider, LV_GRID_ALIGN_STRETCH, 3, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
	heater_slider.setHeaterName("Heater");
	heater_slider.setHeaterMinTemperature(0);
	heater_slider.setHeaterMaxTemperature(300);
	heater_slider.setActiveTemperature(200);
	heater_slider.setStandbyTemperature(0);
	heater_slider.setCurrentTemperature(250);
	heater_slider.setHeaterState(HeaterSliderPresenter::heater_state_t::active, "Active");

	/* Icon */
	Icon icon("icon", cont2);
	cont2.setGridCell(icon, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 3, 1);
	icon.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true);
	icon.setSrc(IMAGE_ASSET("examples/example_full_color.png"));

	/* Axis Control */
	LvContainer axis_cont("axis_cont", cont2);
	cont2.setGridCell(axis_cont, LV_GRID_ALIGN_STRETCH, 5, 2, LV_GRID_ALIGN_STRETCH, 0, 2);
	axis_cont.setFlexFlow(LV_FLEX_FLOW_ROW);
	axis_cont.addStyle(Themes::getLvglStyles().pad_zero);

	XYControl axis_control("xy_control", axis_cont);
	axis_control.setSize(LV_PCT(75), LV_PCT(100));
	axis_control.setYHomed(false);
	axis_control.setYDisabled(true);

	GenericAxisControl generic_axis("generic_axis", axis_cont);
	generic_axis.setSize(LV_PCT(25), LV_PCT(100));
	generic_axis.setAxisLetter('Z');
	generic_axis.setAxisPosition(100.0f);
	generic_axis.setDisabled(true);

	/* Tool List */
	ToolList tool_list("tool_list", cont2);
	cont2.setGridCell(tool_list, LV_GRID_ALIGN_STRETCH, 5, 5, LV_GRID_ALIGN_STRETCH, 2, 2);
	tool_list.setItemCnt(2);
	for (size_t i = 0; i < tool_list.getItemCnt(); ++i)
	{
		auto item = tool_list.getToolListItem(i);
		if (item == nullptr)
		{
			continue;
		}
		item->setLabel(fmt::format("Tool {}", i));
		item->setSelected(i == 0);
		item->setStatus("state");
		item->setCurrentTemp(100);
		item->setActiveTemp(200);
		item->setStandbyTemp(0);
		item->showTemps(true);
	}

	/* Text Box */
	TextBox text_box("text_box", cont2);
	cont2.setGridCell(text_box, LV_GRID_ALIGN_STRETCH, 7, 3, LV_GRID_ALIGN_STRETCH, 0, 1);
	text_box.setLabel("Text box label");
	text_box.setPlaceholderText("Placeholder");

	/* Slider */
	Slider slider2("slider2", cont2);
	cont2.setGridCell(slider2, LV_GRID_ALIGN_STRETCH, 7, 3, LV_GRID_ALIGN_STRETCH, 1, 1);
	slider2.setValue(40);
	slider2.setLabel("Slider label");

	for (size_t i = 0; i < Themes::getThemeCount(); ++i)
	{
		const Themes::Theme* theme = Themes::getTheme(i);
		if (theme == nullptr)
		{
			continue;
		}
		LOG_INFO("Testing theme: {}", theme->getName());
		theme->setThemeActive();
		preview.updateSwatches();
		canvas.clear();
		canvas.drawLine({0, 0}, {99, 99}, lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);
		canvas.drawLabelPx({75, 20}, "Label", lv_palette_main(LV_PALETTE_BLUE), LV_OPA_COVER);

		cont.show();
		cont2.hide();
		EXPECT_EQUAL_SCREENSHOT(fmt::format("theme_widgets_{}.png", theme->getName()).c_str());

		cont.hide();
		cont2.show();
		EXPECT_EQUAL_SCREENSHOT(fmt::format("theme_widgets_{}_2.png", theme->getName()).c_str());
	}
}

/*
 * test_theme.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Components/AxisControl/GenericAxisControl.h"
#include "UI/Components/AxisControl/XYControl.h"
#include "UI/Components/Button/DraggableButton.h"
#include "UI/Components/Canvas/Canvas.h"
#include "UI/Components/Graph/Graph.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/LVGL/LvArc.h"
#include "UI/Components/LVGL/LvArclabel.h"
#include "UI/Components/LVGL/LvBar.h"
#include "UI/Components/LVGL/LvButtonmatrix.h"
#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvDropdown.h"
#include "UI/Components/LVGL/LvImage.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvList.h"
#include "UI/Components/LVGL/LvSlider.h"
#include "UI/Components/LVGL/LvTextarea.h"
#include "UI/Components/MessageBox/AlertMessageBox.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Theme/ThemePreview.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Styles/ThemeFile.h"
#include "UI/Widgets/Temperature/HeaterSlider.h"
#include "UI/Widgets/ToolList/ToolList.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <fstream>
#include <gtest/gtest.h>

using namespace UI;

class TestTheme : public UiTestSuite
{
  public:
	TestTheme() {}

	~TestTheme()
	{
		Themes::Theme* default_theme = Themes::getThemeByName(StorageHelper::getData(ID_THEME));
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
	constexpr bool darkMode = true;

	// A customisable theme with an all-zero light palette and fonts without weights
	std::string error;
	auto file = Themes::parseThemeFile(R"(
		<theme name="test_theme" extends="base">
			<palette primary_hue="0" secondary_hue="0" chroma="0" dark="false"/>
			<fonts>
				<font name="header" size="18" weight="none" style="bold"/>
				<font name="normal" size="14" weight="none" style="normal"/>
				<font name="emphasis" size="14" weight="none" style="bold"/>
				<font name="subdued" size="12" weight="none" style="normal"/>
			</fonts>
		</theme>)",
									   "test_theme.xml",
									   error);
	ASSERT_TRUE(file.has_value()) << error;
	ASSERT_EQ(file->fonts["header"].weight, 0u) << "none is weight 0: the face's own weight";

	// Load it alongside the built-in themes
	std::vector<Themes::ThemeFile> files;
	std::string text;
	for (const char* name : {"base", "duetscreen"})
	{
		std::ifstream in(fmt::format("assets/ui/themes/{:s}.xml", name));
		text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		files.push_back(*Themes::parseThemeFile(text, name, error));
	}
	files.push_back(std::move(*file));
	Themes::setThemeFiles(std::move(files), nullptr);

	Themes::Theme* theme = Themes::getThemeByName("test_theme");
	ASSERT_NE(theme, nullptr);
	ASSERT_TRUE(theme->isCustomisable());
	theme->resetPalette();
	theme->setThemeActive();

	ThemePreview themePreview("theme_preview", screen);
	themePreview.setSize(LV_PCT(100), LV_SIZE_CONTENT);

	EXPECT_TRUE(Themes::getCurrentTheme() == theme);
	themePreview.setDarkMode(darkMode);
	themePreview.setPrimaryHue(primaryHue);
	themePreview.setSecondaryHue(secondaryHue);
	themePreview.setChroma(chroma);
	EXPECT_EQUAL_SCREENSHOT("theme/example_1.png");

	themePreview.setDarkMode(false);
	themePreview.setPrimaryHue(25);
	themePreview.setSecondaryHue(150);
	themePreview.setChroma(0.5f);
	EXPECT_EQUAL_SCREENSHOT("theme/example_2.png");

	themePreview.setDarkMode(true);
	themePreview.setPrimaryHue(300);
	themePreview.setSecondaryHue(50);
	themePreview.setChroma(0.5f);
	EXPECT_EQUAL_SCREENSHOT("theme/example_3.png");
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
	screen.setFlexFlow(LV_FLEX_FLOW_COLUMN);

	ThemePreview preview("theme_preview", screen);
	preview.setSize(LV_PCT(100), LV_SIZE_CONTENT);
	preview.showControls(false);

	LvContainer cont("container", screen);
	cont.setWidth(LV_PCT(100));
	cont.setFlexGrow(1);
	cont.setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
	cont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
	cont.setScrollbarMode(LV_SCROLLBAR_MODE_OFF);

	cont.updateLayout();
	const size_t col_width = cont.getWidth() / 5;

	/* LvBar */
	LvBar bar("bar", cont);
	bar.setWidth(col_width);
	bar.setValue(40);

	Bar label_bar("label_bar", cont);
	label_bar.setWidth(col_width);
	label_bar.setValue(10);

	Bar label_bar2("label_bar2", cont);
	label_bar2.setWidth(col_width);
	label_bar2.setValue(70);

	/* LvButtonmatrix */
	LvButtonmatrix button_matrix("button_matrix", cont);
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
	checkbox.setText("Checkbox");
	checkbox2.setText("Checkbox");
	checkbox2.setChecked(true);

	/* LvImage */
	LvImage image("image", cont);

	LvLabel label("label", cont);
	label.setText("Label");

	/* LvSlider */
	LvSlider lv_slider("lv_slider", cont);
	lv_slider.setWidth(col_width);
	lv_slider.setValue(50);

	/* LvDropdown */
	LvDropdown dropdown("dropdown", cont);
	dropdown.setFlag(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, true); // so dropdown menu doesn't render over other widgets
	dropdown.open();

	/* LvTextarea */
	LvTextarea text_area("text_area", cont);
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

	LvArclabel arc_label("arc_label", arc_cont);
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

	FileView file_view("files",
					   file_folder_cont,
					   FileView::StorageKeys{.sortBy = {"file_sort_by", FilePresenter::SortBy::DATE},
											 .sortDescending = {"file_sort_descending", true},
											 .displayMode = {"file_display_mode", FileView::DisplayMode::List}});
	file_view.hide();

	FileView::FileItem file_item("file_item", file_folder_cont);
	file_item.setHeight(80);
	file_item.setFlexGrow(1);
	// file_item.setHeight(LV_PCT(100));
	file_item.setType(false);
	file_item.setFileLabel("File name");
	file_item.setFileDate("2025-08-20 12:37:10");
#if SHOW_FILE_ITEM_SIZE
	file_item.setFileSize("671 KB");
#endif

	FileView::FileItem folder_item("folder_item", file_folder_cont);
	folder_item.setHeight(80);
	folder_item.setFlexGrow(1);
	// folder_item.setHeight(LV_PCT(100));
	folder_item.setType(true);
	folder_item.setFileLabel("Folder name");
	folder_item.setFileDate("2025-08-20 12:39:59");
#if SHOW_FILE_ITEM_SIZE
	folder_item.setFileSize("0 B");
#endif

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
	lv_obj_t* table = lv_table_create(cont.getRootPtr());
	lv_obj_set_size(table, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	const size_t table_cols = 3;
	const size_t table_rows = 3;
	lv_table_set_column_count(table, table_cols);
	lv_table_set_row_count(table, table_rows);
	lv_table_set_selected_cell(table, 1, 1);
	for (size_t i = 0; i < table_cols * table_rows; i++)
	{
		lv_table_set_cell_value(table, i % table_cols, i / table_cols, fmt::format("Cell {}", i).c_str());
	}

	/* Numberpad */
	NumberPad numberpad("numberpad", cont, layout_t{0, 0, 20, 100});

	/* Message Box */
	UI::Themes::setIconFolder("examples");
	MessageBox message_box("message_box", cont, layout_t(0, 0, 40, 50));
	message_box.setTitle("Message Box Title");
	message_box.setText("Message Box Text");
	message_box.cancelVisible(true);
	message_box.okVisible(true);
	message_box.imageVisible(true);
	message_box.setImage(Themes::getFixedIconPath("examples", "small_print_thumbnail.png").c_str());
	message_box.progressVisible(true);
	message_box.setProgress(40);
	UI::Themes::resetIconFolder();

	/* Heater Slider */
	HeaterSlider heater_slider("heater_slider", cont);
	heater_slider.setSize(LV_PCT(40), LV_SIZE_CONTENT);
	heater_slider.setHeaterName("Heater");
	heater_slider.setHeaterMinTemperature(0);
	heater_slider.setHeaterMaxTemperature(300);
	heater_slider.setActiveTemperature(200);
	heater_slider.setStandbyTemperature(0);
	heater_slider.setCurrentTemperature(250);
	heater_slider.setHeaterState(HeaterSliderPresenter::heater_state_t::active, "Active");

	/* Icon */
	Icon icon("icon", cont);
	icon.setSize(col_width, col_width);
	icon.setFixedIcon(Themes::getFixedIconPath("examples", "example_full_color.png"));

	/* Axis Control */
	LvContainer axis_cont("axis_cont", cont);
	axis_cont.setSize(LV_PCT(25), LV_PCT(100));
	axis_cont.setMinWidth(LV_SIZE_CONTENT);
	axis_cont.setFlexFlow(LV_FLEX_FLOW_ROW);
	axis_cont.addStyle(Themes::getLvglStyles().pad_zero);

	XYControl axis_control("xy_control", axis_cont);
	axis_control.setSize(LV_PCT(75), LV_PCT(100));
	axis_control.setYHomed(false);
	axis_control.setYDisabled(true);

	GenericAxisControl generic_axis("generic_axis", axis_cont);
	generic_axis.setSize(LV_PCT(25), LV_PCT(100));
	generic_axis.setMinWidth(LV_SIZE_CONTENT);
	generic_axis.setAxisLetter('Z');
	generic_axis.setAxisPosition(100.0f);
	generic_axis.setDisabled(true);

	/* Tool List */
	ToolList tool_list("tool_list", cont);
	tool_list.setSize(LV_PCT(40), LV_SIZE_CONTENT);
	tool_list.setToolCount(2);
	for (size_t i = 0; i < tool_list.getToolCount(); ++i)
	{
		auto item = tool_list.getTool(i);
		if (item == nullptr)
		{
			continue;
		}
		item->setLabel(fmt::format("Tool {}", i));
		item->setSelected(i == 0);
		item->setHeaterCount(i);
		for (size_t j = 0; j < item->getHeaterCount(); ++j)
		{
			auto heater = item->getHeater(j);
			if (heater == nullptr)
			{
				continue;
			}
			heater->setLabel(fmt::format("Heater {}", j));
			heater->setStatus("standby");
			heater->setCurrentTemp(100 + j * 10);
			heater->setActiveTemp(200 + j * 10);
			heater->setStandbyTemp(0);
		}
	}

	/* Text Box */
	TextBox empty_text_box("empty_text_box", cont);
	empty_text_box.setSize(LV_PCT(40), LV_SIZE_CONTENT);
	empty_text_box.setLabel("Text box label");
	empty_text_box.setPlaceholderText("Placeholder");

	TextBox text_box("text_box", cont);
	text_box.setSize(LV_PCT(30), LV_SIZE_CONTENT);
	text_box.setLabel("Text box label");
	text_box.setPlaceholderText("Placeholder");
	text_box.setText("This is some sample text in the text box.");

	/* Slider */
	Slider slider("slider", cont);
	slider.setSize(LV_PCT(30), LV_SIZE_CONTENT);
	slider.setValue(40);
	slider.setLabel("Slider label");
	numberpad.setHeader("Numberpad");

	/* Sidebar */
	Xml::XmlView sidebar("sidebar", cont, "side_bar");
	sidebar.setSize(LV_PCT(10), LV_PCT(100));

	for (size_t i = 0; i < Themes::getThemeCount(); ++i)
	{
		ZoneScopedN("Testing Theme");
		Themes::Theme* theme = Themes::getTheme(i);
		if (theme == nullptr)
		{
			continue;
		}
		LOG_INFO("Testing theme: {}", theme->getName());
		theme->setThemeActive();
		preview.updateSwatches();

		cont.scrollToX(0, LV_ANIM_OFF);

		/* Reset canvas */
		canvas.clear();
		canvas.drawLine({0, 0}, {99, 99}, lv_palette_main(LV_PALETTE_RED), LV_OPA_COVER);
		canvas.drawLabelPx({75, 20}, "Label", lv_palette_main(LV_PALETTE_BLUE), LV_OPA_COVER);

		/* Open dropdown */
		dropdown.open();

		/**
		 * The container is variable width based on each themes styles.
		 * More padding could make the widgets overflow etc.
		 */
		const auto width = cont.getWidth();
		const auto screen_width = cont.getParent()->getWidth();

		size_t iteration = 0;
		EXPECT_EQUAL_SCREENSHOT(fmt::format("theme/{:s}/widgets_{:d}.png", theme->getName(), iteration++).c_str());

		/* Close dropdown */
		dropdown.close();

		/**
		 * Scroll the container so each child is tested while it is fully visible.
		 */
		while (cont.getScrollRight() > 0)
		{
			lv_coord_t cont_x2 = cont.getX2();
			lv_coord_t x_snap = LV_COORD_MAX;
			cont.iterateChildren(
				[&](size_t /* index */, LvObj& child)
				{
					auto child_coords = child.getCoords();
					if (child_coords.x1 > cont_x2)
					{
						/* Child is completely to the right of the container */
						return;
					}

					if (child_coords.x2 <= cont_x2)
					{
						/* Child has already been fully processed */
						return;
					}

					/* Child is only partially visible */
					x_snap = std::min(child_coords.x1, x_snap);
				});
			cont.scrollByBounded(-x_snap, LV_ANIM_OFF);
			EXPECT_EQUAL_SCREENSHOT(fmt::format("theme/{:s}/widgets_{:d}.png", theme->getName(), iteration++).c_str());
		}
	}
}

/*
 * test_bar_marker.cpp
 *
 * <ds_bar_marker> (src/UI/Xml/Widgets/DsBarMarker.h): a value marked on a bar, dragged along it or tapped.
 */

#include "UI/Components/Button/Button.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "helix_xml.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

/** Holds the value marked, and records the actions the marker raises */
class MarkerPresenter : public UI::Xml::XmlPresenter
{
  public:
	MarkerPresenter()
		: XmlPresenter("test_marker")
	{
		target.set(0);
		action("test.tapped", "Count a tap", [this](const UI::Xml::ActionArgs&) { taps++; });
		action("test.set",
			   "Record the edited value",
			   [this](const UI::Xml::ActionArgs& args) { values.push_back(args.value().value_or(-1)); });
	}

	void fields(UI::Xml::FieldVisitor& f) override { f.field("target", target, "The value marked"); }

	UI::Xml::IntSubject target; // The value marked
	int taps = 0;				// `clicked` actions raised
	std::vector<float> values;	// `value_confirmed` actions' values
};

class TestBarMarker : public UiTestSuite
{
  protected:
	TestBarMarker()
	{
		// A 300 px bar, the width of the view, from 0 to 100, with the marker above it
		lv_xml_register_component_from_data("test_bar_marker", R"(
			<component>
				<view extends="lv_obj" width="300" height="120" style_pad_all="0" scrollable="false">
					<ds_bar name="bar" min="0" max="100" width="100%" height="10" align="center"/>
					<ds_bar_marker name="marker" bar="bar" min="0" max="100" bind_value="target"
								   label_format="{} C" align="left_mid" y="-30">
						<action trigger="clicked" name="test.tapped"/>
						<action trigger="value_confirmed" name="test.set"/>
					</ds_bar_marker>
				</view>
			</component>)");
		view = std::make_unique<UI::Xml::XmlView>("marker_view", screen, "test_bar_marker", presenter);
		view->show();
		lv_obj_update_layout(screen.getRootPtr());
	}

	~TestBarMarker() override { lv_test_mouse_release(); }

	/** Let the pointer report where it is */
	static void read() { lv_indev_read(lv_test_indev_get_indev(LV_INDEV_TYPE_POINTER)); }

	/** The marker's area on screen */
	lv_area_t markerArea() const
	{
		lv_area_t area;
		lv_obj_get_coords(lv_obj_find_by_name(view->getRootPtr(), "marker"), &area);
		return area;
	}

	/** The bar's area on screen */
	lv_area_t barArea() const
	{
		lv_area_t area;
		lv_obj_get_coords(lv_obj_find_by_name(view->getRootPtr(), "bar"), &area);
		return area;
	}

	std::shared_ptr<MarkerPresenter> presenter = std::make_shared<MarkerPresenter>();
	std::unique_ptr<UI::Xml::XmlView> view;
};

TEST_F(TestBarMarker, FollowsTheValueAlongTheBar)
{
	EXPECT_EQ(markerArea().x1, barArea().x1) << "at the bottom of the scale its left edge is at the bar's";

	presenter->target.set(100);
	lv_obj_update_layout(screen.getRootPtr());
	EXPECT_NEAR(markerArea().x2, barArea().x2, 2) << "at the top its right edge is at the bar's";
	auto& marker = static_cast<UI::Button&>(*UI::LvObj::fromPtr(lv_obj_find_by_name(view->getRootPtr(), "marker")));
	EXPECT_EQ(marker.getText(), "100 C");
}

TEST_F(TestBarMarker, ATapIsAClick)
{
	const lv_area_t area = markerArea();
	lv_test_mouse_move_to((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
	lv_test_mouse_press();
	read();
	lv_test_mouse_release();
	read();

	EXPECT_EQ(presenter->taps, 1);
	EXPECT_TRUE(presenter->values.empty());
}

TEST_F(TestBarMarker, ADragChoosesAValueAndIsNotAClick)
{
	const lv_area_t area = markerArea();
	const int32_t y = (area.y1 + area.y2) / 2;
	lv_test_mouse_move_to(area.x1 + 5, y);
	lv_test_mouse_press();
	read();
	for (int32_t dx = 10; dx <= 150; dx += 10) // Half the bar: half the scale
	{
		lv_test_mouse_move_to(area.x1 + 5 + dx, y);
		read();
	}
	lv_test_mouse_release();
	read();

	EXPECT_EQ(presenter->taps, 0);
	ASSERT_EQ(presenter->values.size(), 1u);
	EXPECT_NEAR(presenter->values[0], 50.0f, 1.0f);
	EXPECT_EQ(presenter->target.get(), 0) << "the subject is the printer's to change";
}

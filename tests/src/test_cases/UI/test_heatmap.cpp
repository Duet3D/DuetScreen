/*
 * test_heatmap.cpp
 *
 * <ds_heatmap> (src/UI/Widgets/Display/DsHeatmap.h) drawing a Surface (src/UI/Xml/Binding/Surface.h).
 */

#include "UI/Xml/Binding/Surface.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "UI/Xml/View/XmlView.h"
#include "helix_xml.h"
#include "test_utils/UiTestSuite.h"
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <memory>

/** Holds the surface shown */
class SurfacePresenter : public UI::Xml::XmlPresenter
{
  public:
	SurfacePresenter()
		: XmlPresenter("test_surface")
	{
		title.set("Surface");
	}

	void fields(UI::Xml::FieldVisitor& f) override
	{
		f.field("surface", surface, "The surface shown");
		f.field("title", title, "Its title");
	}

	UI::Xml::Surface surface;		  // The surface shown
	UI::Xml::StringSubject<32> title; // Its title
};

class TestHeatmap : public UiTestSuite
{
  protected:
	TestHeatmap()
	{
		lv_xml_register_component_from_data("test_heatmap", R"(
			<component>
				<view extends="lv_obj" style_pad_all="0">
					<ds_heatmap name="heatmap" bind_surface="surface" bind_title="title" resolution="100"
								width="100%" height="100%"/>
				</view>
			</component>)");
		view = std::make_unique<UI::Xml::XmlView>("heatmap_view", screen, "test_heatmap", presenter);
		view->setSize(500, 400);
		view->show();
	}

	std::shared_ptr<SurfacePresenter> presenter = std::make_shared<SurfacePresenter>();
	std::unique_ptr<UI::Xml::XmlView> view;
};

TEST_F(TestHeatmap, Empty)
{
	EXPECT_EQUAL_SCREENSHOT("heatmap/empty.png");
}

TEST_F(TestHeatmap, DrawsTheSamples)
{
	// A slope from -1 at the left to +1 at the right over X -50 to 150 and Y 0 to 200, with a hole in the middle
	UI::Xml::Surface::Values values;
	values.x = {-50.0f, 150.0f};
	values.y = {0.0f, 200.0f};
	values.scale = {-1.0f, 1.0f};
	values.columns = 20;
	values.rows = 10;
	for (size_t j = 0; j < values.rows; j++)
	{
		for (size_t i = 0; i < values.columns; i++)
		{
			const bool hole = i >= 8 && i < 12 && j >= 4 && j < 6;
			values.samples.push_back(hole ? std::numeric_limits<float>::quiet_NaN()
										  : -1.0f + 2.0f * static_cast<float>(i) / (values.columns - 1));
		}
	}
	presenter->surface.set(std::move(values));

	EXPECT_EQUAL_SCREENSHOT("heatmap/slope.png");
}

TEST_F(TestHeatmap, SamplesOutsideTheGridHaveNoValue)
{
	presenter->surface.set({.columns = 2, .rows = 1, .samples = {0.5f, 1.0f}});
	EXPECT_EQ(presenter->surface.sample(1, 0), 1.0f);
	EXPECT_TRUE(std::isnan(presenter->surface.sample(2, 0)));
	EXPECT_TRUE(std::isnan(presenter->surface.sample(0, 1)));
}

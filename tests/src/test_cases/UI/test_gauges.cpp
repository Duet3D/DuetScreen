/*
 * test_gauges.cpp
 *
 * The values gauge widgets show: the global `gauges` list (src/UI/Xml/Data/GaugeSource.h) and `gauges.edit`
 * (src/UI/Presenters/Gauge/GaugeActions.h).
 */

#include "UI/Core/Model.h"
#include "UI/Xml/Binding/Scope.h"
#include "UI/Xml/Data/GaugeSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using UI::Xml::GaugeRow;

class TestGauges : public UiTestSuite
{
  protected:
	TestGauges()
	{
		load_model_data_from_file("tests/object_model/test_bench/model_heat_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_tools_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_fans_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_sensors_v.json");
		pump();
		UI::Xml::Services::setGcodeSender([this](std::string_view gcode) { m_sent.emplace_back(gcode); });
	}

	~TestGauges() override
	{
		UI::Xml::Services::setGcodeSender(nullptr);
		if (m_widget != nullptr)
		{
			lv_obj_delete(m_widget);
		}
	}

	/** Deliver queued model events, as the event loop would */
	static void pump()
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}

	/** The row of `gauges` whose id is `id`, or nullptr */
	static GaugeRow* gauge(std::string_view id)
	{
		UI::Xml::ListBase* list = UI::Xml::Scope::findList(nullptr, "gauges");
		if (list == nullptr)
		{
			return nullptr;
		}
		for (size_t i = 0; i < list->size(); ++i)
		{
			auto* row = dynamic_cast<GaugeRow*>(&list->row(i));
			if (row != nullptr && row->id.get() == id)
			{
				return row;
			}
		}
		return nullptr;
	}

	/** Raise `gauges.edit` with `arg` from a widget in `row`, as a tapped gauge does */
	void edit(GaugeRow& row, std::string_view arg)
	{
		if (m_widget == nullptr)
		{
			m_widget = lv_obj_create(screen.getRootPtr());
		}
		UI::Xml::Actions::setRow(m_widget, &row);
		UI::Xml::Actions::dispatch(m_widget, "actions.gauge.edit", arg);
	}

	std::vector<std::string> m_sent; // G-code sent
	lv_obj_t* m_widget = nullptr;	 // The widget edits are raised from
};

TEST_F(TestGauges, EveryValueHasARowNamedForWhatItIs)
{
	// Tool 0's only heater, named after the tool
	const GaugeRow* tool = gauge("tool.0.0");
	ASSERT_NE(tool, nullptr);
	EXPECT_EQ(tool->kind.get(), "heater");
	EXPECT_EQ(tool->unit.get(), "°C");
	EXPECT_EQ(tool->text.get(), "23.6");
	EXPECT_EQ(tool->value.get(), 24);
	EXPECT_EQ(tool->max.get(), 320) << "the heater's maximum";
	EXPECT_EQ(tool->state.get(), "active");
	EXPECT_EQ(tool->editable.get(), 1);

	// The bed and chamber, by their numbers
	const GaugeRow* bed = gauge("bed.0");
	ASSERT_NE(bed, nullptr);
	EXPECT_EQ(bed->name.get(), _("temperature.bed"));
	EXPECT_EQ(bed->state.get(), "off");
	EXPECT_EQ(bed->detail.get(), _("temperature.status.off")) << "no target while it is off";
	EXPECT_EQ(bed->target.get(), bed->min.get());
	EXPECT_NE(gauge("chamber.1"), nullptr);

	// Fans in percent, with their speed in RPM where they have a tachometer
	const GaugeRow* fan = gauge("fan.1");
	ASSERT_NE(fan, nullptr);
	EXPECT_EQ(fan->kind.get(), "fan");
	EXPECT_EQ(fan->unit.get(), "%");
	EXPECT_EQ(fan->max.get(), 100);

	// Sensors that heaters use show as those heaters
	EXPECT_EQ(gauge("sensor.0"), nullptr);
	EXPECT_EQ(gauge("sensor.1"), nullptr);

	// The job and the factors are always there
	ASSERT_NE(gauge("job.progress"), nullptr);
	EXPECT_EQ(gauge("job.progress")->editable.get(), 0);
	ASSERT_NE(gauge("speed_factor"), nullptr);
	EXPECT_EQ(gauge("speed_factor")->text.get(), "100");
	EXPECT_EQ(gauge("speed_factor")->max.get(), 200) << "100% half way";
	EXPECT_NE(gauge("extrusion_factor"), nullptr);
}

TEST_F(TestGauges, EditingSetsAFansSpeed)
{
	GaugeRow* fan = gauge("fan.1");
	ASSERT_NE(fan, nullptr);
	ASSERT_EQ(fan->editable.get(), 1);
	edit(*fan, "60");
	ASSERT_EQ(m_sent.size(), 1u);
	EXPECT_EQ(m_sent[0], "M106 P1 S153\n");
}

TEST_F(TestGauges, ValuesThatCantBeChangedAreLeftAlone)
{
	GaugeRow* progress = gauge("job.progress");
	ASSERT_NE(progress, nullptr);
	edit(*progress, "50");
	EXPECT_TRUE(m_sent.empty());
}

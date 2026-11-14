/*
 * test_om_bindings.cpp
 *
 * XML names for the object model (src/UI/Xml/Om): `om.*` subjects and lists, converters, and the names a row of an
 * object model list gives relative to its item.
 */

#include "ObjectModel/Tree/Tree.h"
#include "UI/Xml/Binding/Globals.h"
#include "UI/Xml/Om/Converters.h"
#include "UI/Xml/Om/OmName.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

namespace
{
	// One line each: the decoder treats a newline as the end of a message
	constexpr std::string_view HEAT =
		R"({"key":"heat","flags":"v","result":{"heaters":[null,{"current":24.16,"active":0,"standby":0,"max":280,"state":"off"},{"current":20,"active":60,"standby":0,"max":120,"state":"active"}]}})";
	constexpr std::string_view HEAT_LIVE =
		R"({"key":"","flags":"d99f","result":{"heat":{"heaters":[null,{"current":61.25,"state":"active"},{"current":20,"state":"active"}]}}})";
	constexpr std::string_view FANS = R"({"key":"fans","flags":"v","result":[{"actualValue":0.45,"name":"Part"}]})";
	constexpr std::string_view TOOLS =
		R"({"key":"tools","flags":"v","result":[{"heaters":[1,2],"name":"Hot end"},{"heaters":[2],"name":"Second"}]})";
} // namespace

class TestOmBindings : public UiTestSuite
{
  protected:
	~TestOmBindings() override { m_views.clear(); }

	/** Feed one response to the decoder and deliver the change, as the event loop would */
	static void receive(std::string_view response)
	{
		load_model_data(std::string(response) + "\n");
		pump();
	}

	/** Deliver queued model events */
	static void pump()
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}

	/** Register `xml` as component `name` and show it */
	UI::Xml::XmlView& show(const char* name, const char* xml)
	{
		lv_xml_register_component_from_data(name, xml);
		return *m_views.emplace_back(std::make_unique<UI::Xml::XmlView>(name, screen, name));
	}

	/** The text of the label called `name` in `root` */
	static std::string text(lv_obj_t* root, const char* name)
	{
		lv_obj_t* label = lv_obj_find_by_name(root, name);
		EXPECT_NE(label, nullptr) << name;
		return label ? lv_label_get_text(label) : "";
	}

	/** The text of the label called `name` in `view` */
	static std::string text(UI::LvObj& view, const char* name) { return text(view.getRootPtr(), name); }

	/** The items of the list called `name` in `view` */
	static lv_obj_t* list(UI::LvObj& view, const char* name) { return lv_obj_find_by_name(view.getRootPtr(), name); }

	std::vector<std::unique_ptr<UI::Xml::XmlView>> m_views; // Views shown
};

TEST_F(TestOmBindings, NamesTakeApartIntoPathAndConverter)
{
	const auto name = UI::Xml::OmName::parse("om.heat.heaters[{value}].current|fixed:1");
	ASSERT_TRUE(name);
	EXPECT_TRUE(name->absolute);
	EXPECT_EQ(name->path, "heat.heaters[{value}].current");
	EXPECT_EQ(name->converter, "fixed");
	EXPECT_EQ(name->argument, "1");

	const auto relative = UI::Xml::OmName::parse("current");
	ASSERT_TRUE(relative);
	EXPECT_FALSE(relative->absolute);
	EXPECT_EQ(relative->converter, "");

	EXPECT_EQ(UI::Xml::OmName::parse("a[{b|int}]|count")->path, "a[{b|int}]") << "a '|' inside an index is the index's";
	EXPECT_FALSE(UI::Xml::OmName::parse("om."));
	EXPECT_FALSE(UI::Xml::OmName::parse("a|"));
}

TEST_F(TestOmBindings, ConvertersGiveWhatTheySay)
{
	using UI::Xml::findConverter;
	const OM::Node number(OM::Node::Kind::number, "61.25");
	const OM::Node fraction(OM::Node::Kind::number, "0.456");
	const OM::Node yes(OM::Node::Kind::boolean, "true");
	const OM::Node state(OM::Node::Kind::string, "active");
	const OM::Node seconds(OM::Node::Kind::number, "3725");
	const OM::Node size(OM::Node::Kind::number, "1572864");
	const OM::Node& missing = OM::Node::missing();

	EXPECT_EQ(findConverter("")->toText(number, {}, ""), "61.25");
	EXPECT_EQ(findConverter("")->toText(missing, {}, ""), "");
	EXPECT_EQ(findConverter("int")->toInteger(number, {}, ""), 61);
	EXPECT_EQ(findConverter("int")->toInteger(yes, {}, ""), 1);
	EXPECT_EQ(findConverter("int")->toInteger(state, {}, ""), 0);
	EXPECT_EQ(findConverter("percent")->toInteger(fraction, {}, ""), 46);
	EXPECT_EQ(findConverter("scale")->toInteger(number, {}, "10"), 613);
	EXPECT_EQ(findConverter("fixed")->toText(number, {}, "1"), "61.2");
	EXPECT_EQ(findConverter("fixed")->toText(state, {}, "1"), "");
	EXPECT_EQ(findConverter("exists")->toInteger(missing, {}, ""), 0);
	EXPECT_EQ(findConverter("exists")->toInteger(number, {}, ""), 1);
	EXPECT_EQ(findConverter("eq")->toInteger(state, {}, "active"), 1);
	EXPECT_EQ(findConverter("eq")->toInteger(state, {}, "fault"), 0);
	EXPECT_EQ(findConverter("duration")->toText(seconds, {}, ""), "1:02:05");
	EXPECT_EQ(findConverter("duration")->toText(fraction, {}, ""), "0:00");
	EXPECT_EQ(findConverter("bytes")->toText(size, {}, ""), "1.5 MB");
	EXPECT_EQ(findConverter("translate")->toText(state, {}, "tr.no.such.prefix"), "active")
		<< "no translation gives the value";
	EXPECT_EQ(findConverter("translate")->toText(state, {}, "temperature.status"), "active")
		<< "a key not written tr.<key> gives the value";
	EXPECT_EQ(findConverter("tr"), nullptr);
	const OM::Node unnamed(OM::Node::Kind::string, "");
	const OM::Node named(OM::Node::Kind::string, "Part cooling");
	EXPECT_EQ(findConverter("default")->toText(named, *OM::Path::parse("fans[2].name"), "tr.fine_tune.fan"),
			  "Part cooling");
	EXPECT_EQ(findConverter("default")->toText(unnamed, *OM::Path::parse("fans[2].name"), "tr.fine_tune.fan"), "Fan 2");
	EXPECT_EQ(findConverter("default")->toText(missing, *OM::Path::parse("fans[3].name"), "tr.fine_tune.fan"), "Fan 3");
	EXPECT_EQ(findConverter("default")->toText(missing, *OM::Path::parse("fans[3].name"), "fine_tune.fan"), "")
		<< "a key not written tr.<key> gives nothing";
	EXPECT_EQ(findConverter("default")->toText(
				  unnamed, *OM::Path::parse("sensors.analog[5].name"), "tr.temperature.heater:2"),
			  "Heater 2")
		<< "the number after the key, not the item's";
	EXPECT_EQ(
		findConverter("numbered")->toText(number, *OM::Path::parse("heat.bedHeaters[1]"), "tr.temperature.bed_number"),
		"Bed 1")
		<< "the item's number, whatever its value";
	EXPECT_EQ(findConverter("numbered")->toText(number, {}, "tr.temperature.bed_number"), "") << "no item number";
	const OM::Node below(OM::Node::Kind::number, "-10.5");
	EXPECT_EQ(findConverter("atleast")->toInteger(below, {}, "0"), 0);
	EXPECT_EQ(findConverter("atleast")->toInteger(number, {}, "0"), 61) << "rounded down";
	EXPECT_EQ(findConverter("atmost")->toInteger(number, {}, "300"), 62) << "rounded up";
	EXPECT_EQ(findConverter("atmost")->toInteger(number, {}, "50"), 50);
	EXPECT_EQ(findConverter("clamp")->toInteger(number, {}, "0:50"), 50);
	EXPECT_EQ(findConverter("clamp")->toInteger(below, {}, "0:50"), 0);
	EXPECT_EQ(findConverter("clamp")->toInteger(number, {}, "0:100"), 61);
	EXPECT_EQ(findConverter("nonsense"), nullptr);
}

TEST_F(TestOmBindings, FieldsBindBeforeTheDataArrivesAndFollowIt)
{
	auto& view = show("om_fields", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%" flex_flow="column">
				<lv_label name="current" bind_text="om.heat.heaters[1].current"/>
				<lv_label name="fixed" bind_text="om.heat.heaters[1].current|fixed:1"/>
				<lv_label name="fan" bind_text="om.fans[0].actualValue|percent" bind_text-fmt="%d%%"/>
				<lv_label name="count" bind_text="om.heat.heaters|count" bind_text-fmt="%d"/>
				<lv_obj name="hot">
					<bind_flag_if cond="om.heat.heaters[1].current|int lt 50" flag="hidden"/>
				</lv_obj>
			</view>
		</component>)");
	lv_obj_t* hot = lv_obj_find_by_name(view.getRootPtr(), "hot");

	EXPECT_EQ(text(view, "current"), "");
	EXPECT_EQ(text(view, "fan"), "0%");
	EXPECT_EQ(text(view, "count"), "0");
	EXPECT_TRUE(lv_obj_has_flag(hot, LV_OBJ_FLAG_HIDDEN));

	receive(HEAT);
	receive(FANS);
	EXPECT_EQ(text(view, "current"), "24.16");
	EXPECT_EQ(text(view, "fixed"), "24.2");
	EXPECT_EQ(text(view, "fan"), "45%");
	EXPECT_EQ(text(view, "count"), "3");

	receive(HEAT_LIVE);
	EXPECT_EQ(text(view, "current"), "61.25");
	EXPECT_FALSE(lv_obj_has_flag(hot, LV_OBJ_FLAG_HIDDEN));

	OM::RemoveAll();
	pump();
	EXPECT_EQ(text(view, "current"), "") << "a disconnect empties every field";
	EXPECT_EQ(text(view, "count"), "0");
}

TEST_F(TestOmBindings, AListHasARowPerItemWithNamesRelativeToIt)
{
	lv_xml_register_component_from_data("om_heater_row", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<bind_flag_if_eq subject="row.exists" ref_value="0" flag="hidden"/>
				<lv_label name="number" bind_text="row.index" bind_text-fmt="%d"/>
				<lv_label name="current" bind_text="row.current|fixed:1"/>
				<lv_label name="state" bind_text="row.state"/>
			</view>
		</component>)");
	receive(HEAT);
	auto& view = show("om_heaters", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="heaters" list="om.heat.heaters" item="om_heater_row" width="100%" height="content"
						 flex_flow="column"/>
			</view>
		</component>)");

	lv_obj_t* rows = list(view, "heaters");
	ASSERT_EQ(lv_obj_get_child_count(rows), 3u) << "a row per item, nulls included";
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_get_child(rows, 0), LV_OBJ_FLAG_HIDDEN)) << "the null item is hidden";
	EXPECT_EQ(text(lv_obj_get_child(rows, 1), "number"), "1");
	EXPECT_EQ(text(lv_obj_get_child(rows, 1), "current"), "24.2");
	EXPECT_EQ(text(lv_obj_get_child(rows, 2), "state"), "active");

	receive(HEAT_LIVE);
	EXPECT_EQ(text(lv_obj_get_child(rows, 1), "current"), "61.2");

	receive(R"({"key":"heat","flags":"v","result":{"heaters":[null,{"current":30,"state":"off"}]}})");
	EXPECT_EQ(lv_obj_get_child_count(rows), 2u) << "a shorter array removes rows";
	EXPECT_EQ(text(lv_obj_get_child(rows, 1), "current"), "30.0");
}

TEST_F(TestOmBindings, NestedListsAndItemsThatAreValues)
{
	lv_xml_register_component_from_data("om_tool_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="heater" bind_text="row.value"/>
				<lv_label name="heater_int" bind_text="row.value|int" bind_text-fmt="H%d"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("om_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content" flex_flow="column">
				<lv_label name="name" bind_text="row.name"/>
				<ds_list name="heaters" list="row.heaters" item="om_tool_heater" width="100%" height="content"/>
			</view>
		</component>)");
	receive(TOOLS);
	auto& view = show("om_tools", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools" item="om_tool" width="100%" height="content" flex_flow="column"/>
			</view>
		</component>)");

	lv_obj_t* tools = list(view, "tools");
	ASSERT_EQ(lv_obj_get_child_count(tools), 2u);
	lv_obj_t* first = lv_obj_get_child(tools, 0);
	EXPECT_EQ(text(first, "name"), "Hot end");
	lv_obj_t* heaters = lv_obj_find_by_name(first, "heaters");
	ASSERT_EQ(lv_obj_get_child_count(heaters), 2u);
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "heater"), "2");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "heater_int"), "H2");
}

TEST_F(TestOmBindings, ANameIsOnlyFoundWhereItsPrefixSays)
{
	UI::Xml::StringSubject<16> global("global");
	UI::Xml::Globals::add("source.test.state", global);
	lv_xml_register_component_from_data("om_strict_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="own" bind_text="row.value"/>
				<lv_label name="tool" bind_text="parent.name"/>
				<lv_label name="global" bind_text="source.test.state"/>
				<lv_label name="unprefixed" text="unchanged" bind_text="name"/>
				<lv_label name="outer_as_own" text="unchanged" bind_text="row.name"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("om_strict_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<ds_list name="heaters" list="row.heaters" item="om_strict_heater" width="100%" height="content"/>
			</view>
		</component>)");
	receive(TOOLS);
	auto& view = show("om_strict", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools" item="om_strict_tool" width="100%" height="content"/>
			</view>
		</component>)");

	lv_obj_t* heater = lv_obj_get_child(lv_obj_find_by_name(lv_obj_get_child(list(view, "tools"), 0), "heaters"), 1);
	EXPECT_EQ(text(heater, "own"), "2") << "row.* is the innermost row's";
	EXPECT_EQ(text(heater, "tool"), "Hot end") << "parent.* is the row around it";
	EXPECT_EQ(text(heater, "global"), "global");
	EXPECT_EQ(text(heater, "unprefixed"), "unchanged") << "a name without a prefix is found nowhere";
	EXPECT_EQ(text(heater, "outer_as_own"), "") << "row.name is the heater number's (it has none), never the tool's";

	m_views.clear();
	UI::Xml::Globals::remove("source.test.state");
}

TEST_F(TestOmBindings, AConvertersArgumentFollowsTheNamesInIt)
{
	// A heater's name: its sensor's, else "Heater <its number>", the number being the row's
	lv_xml_register_component_from_data("om_named_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="name"
						  bind_text="om.sensors.analog[{om.heat.heaters[{row.value}].sensor}].name|default:tr.temperature.heater:{row.value}"/>
			</view>
		</component>)");
	receive(R"({"key":"heat","flags":"v","result":{"heaters":[null,{"sensor":1},{"sensor":2},{"sensor":3}]}})");
	receive(R"({"key":"sensors","flags":"v","result":{"analog":[null,{"name":"Nozzle"},{"name":""},{"name":""}]}})");
	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[1,2]}]})");
	auto& view = show("om_named", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools[0].heaters" item="om_named_heater" width="100%" height="content"/>
			</view>
		</component>)");

	lv_obj_t* heaters = list(view, "tools");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 0), "name"), "Nozzle");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "name"), "Heater 2") << "the row's number, not the sensor's";

	// The tool's second heater becomes heater 3: the name follows, the number in the argument too
	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[1,3]}]})");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "name"), "Heater 3");
}

TEST_F(TestOmBindings, AFieldTheItemHasntSentYetIsStillItsOwn)
{
	lv_xml_register_component_from_data("om_late_row", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="max" bind_text="row.max" />
			</view>
		</component>)");
	// Only the live fields so far: no heater has a `max`
	receive(R"({"key":"","flags":"d99f","result":{"heat":{"heaters":[{"current":20}]}}})");
	auto& view = show("om_late", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="heaters" list="om.heat.heaters" item="om_late_row" width="100%" height="content"/>
			</view>
		</component>)");
	lv_obj_t* row = lv_obj_get_child(list(view, "heaters"), 0);
	EXPECT_EQ(text(row, "max"), "");

	receive(R"({"key":"heat","flags":"v","result":{"heaters":[{"current":20,"max":285}]}})");
	EXPECT_EQ(text(row, "max"), "285");
}

TEST_F(TestOmBindings, AnUnknownConverterBindsNothing)
{
	auto& view = show("om_bad", R"(
		<component>
			<view extends="lv_obj">
				<lv_label name="bad" text="unchanged" bind_text="om.heat.heaters[1].current|nonsense"/>
			</view>
		</component>)");
	receive(HEAT);
	EXPECT_EQ(text(view, "bad"), "unchanged");
}

namespace
{
	// The example from the design: tools list their heaters by number
	constexpr std::string_view JOIN_HEAT =
		R"({"key":"heat","flags":"v","result":{"heaters":[{"current":0,"active":200,"standby":100},null,{"current":40,"active":210,"standby":110},{"current":50,"active":220,"standby":120}]}})";
	constexpr std::string_view JOIN_TOOLS =
		R"({"key":"tools","flags":"v","result":[{"heaters":[0,2]},{"heaters":[3]}]})";
} // namespace

class TestOmJoins : public TestOmBindings
{
  protected:
	/** Register the tool card and its heater row, which reads `om.heat.heaters[{value}]` */
	static void registerToolCards()
	{
		lv_xml_register_component_from_data("join_heater", R"(
			<component>
				<view extends="lv_obj" width="100%" height="content" flex_flow="row">
					<lv_label name="number" bind_text="row.value"/>
					<lv_label name="current" bind_text="om.heat.heaters[{row.value}].current"/>
					<lv_label name="active" bind_text="om.heat.heaters[{row.value}].active"/>
					<lv_label name="standby" bind_text="om.heat.heaters[{row.value}].standby"/>
				</view>
			</component>)");
		lv_xml_register_component_from_data("join_tool", R"(
			<component>
				<view extends="lv_obj" width="100%" height="content" flex_flow="column">
					<lv_label name="title" bind_text="row.index" bind_text-fmt="T%d"/>
					<ds_list name="heaters" list="row.heaters" item="join_heater" width="100%" height="content"/>
				</view>
			</component>)");
	}

	/** The heater row `row` of tool card `tool` */
	static lv_obj_t* heaterRow(UI::LvObj& view, uint32_t tool, uint32_t row)
	{
		lv_obj_t* card = lv_obj_get_child(list(view, "tools"), static_cast<int32_t>(tool));
		return lv_obj_get_child(lv_obj_find_by_name(card, "heaters"), static_cast<int32_t>(row));
	}

	/** "current / active / standby" of a heater row */
	static std::string temperatures(lv_obj_t* row)
	{
		return text(row, "current") + " / " + text(row, "active") + " / " + text(row, "standby");
	}
};

TEST_F(TestOmJoins, ToolsShowTheirHeatersThroughTheirNumbers)
{
	registerToolCards();
	receive(JOIN_HEAT);
	receive(JOIN_TOOLS);
	auto& view = show("join_tools", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools" item="join_tool" width="100%" height="content" flex_flow="column"/>
			</view>
		</component>)");

	ASSERT_EQ(lv_obj_get_child_count(list(view, "tools")), 2u);
	EXPECT_EQ(temperatures(heaterRow(view, 0, 0)), "0 / 200 / 100");
	EXPECT_EQ(temperatures(heaterRow(view, 0, 1)), "40 / 210 / 110");
	EXPECT_EQ(temperatures(heaterRow(view, 1, 0)), "50 / 220 / 120");

	// A temperature changes: the row reading that heater follows
	receive(
		R"({"key":"","flags":"d99f","result":{"heat":{"heaters":[{"current":0},null,{"current":41},{"current":50}]}}})");
	EXPECT_EQ(text(heaterRow(view, 0, 1), "current"), "41");

	// T0's second heater becomes heater 3: the same widgets now read heater 3
	lv_obj_t* label = lv_obj_find_by_name(heaterRow(view, 0, 1), "current");
	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[0,3]},{"heaters":[3]}]})");
	EXPECT_EQ(lv_obj_find_by_name(heaterRow(view, 0, 1), "current"), label) << "nothing was rebuilt";
	EXPECT_EQ(temperatures(heaterRow(view, 0, 1)), "50 / 220 / 120");

	// A tool loses a heater, then the view goes: the forwarders go with their rows, before what they observe
	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[0]},{"heaters":[3]}]})");
	EXPECT_EQ(lv_obj_get_child_count(lv_obj_find_by_name(lv_obj_get_child(list(view, "tools"), 0), "heaters")), 1u);
	m_views.clear();
	receive(JOIN_TOOLS);
}

TEST_F(TestOmJoins, ARowsIndexReadsAParallelArrayOfItsParent)
{
	// In RepRapFirmware a tool's own temperatures run parallel to its heaters: the tool's (the row around the heater's)
	// `active` at the heater row's index
	lv_xml_register_component_from_data("parallel_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="active" bind_text="parent.active[{row.index}]"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("parallel_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<ds_list name="heaters" list="row.heaters" item="parallel_heater" width="100%" height="content"/>
			</view>
		</component>)");
	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[1,2],"active":[205,60]}]})");
	auto& view = show("parallel_tools", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools" item="parallel_tool" width="100%" height="content"/>
			</view>
		</component>)");

	lv_obj_t* heaters = lv_obj_find_by_name(lv_obj_get_child(list(view, "tools"), 0), "heaters");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 0), "active"), "205");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "active"), "60");
}

TEST_F(TestOmJoins, TheCurrentTool)
{
	lv_xml_register_component_from_data("current_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="heater" bind_text="row.value"/>
			</view>
		</component>)");
	receive(R"({"key":"tools","flags":"v","result":[{"name":"Left","heaters":[1]},{"name":"Right","heaters":[2,3]}]})");
	receive(R"({"key":"state","flags":"vn","result":{"currentTool":-1}})");
	auto& view = show("current_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%" flex_flow="column">
				<lv_label name="name" bind_text="om.tools[{om.state.currentTool}].name"/>
				<ds_list name="heaters" list="om.tools[{om.state.currentTool}].heaters" item="current_heater"
						 width="100%" height="content"/>
			</view>
		</component>)");

	EXPECT_EQ(text(view, "name"), "") << "no current tool";
	EXPECT_EQ(lv_obj_get_child_count(list(view, "heaters")), 0u);

	receive(R"({"key":"state","flags":"vn","result":{"currentTool":1}})");
	EXPECT_EQ(text(view, "name"), "Right");
	ASSERT_EQ(lv_obj_get_child_count(list(view, "heaters")), 2u);
	EXPECT_EQ(text(lv_obj_get_child(list(view, "heaters"), 1), "heater"), "3");

	receive(R"({"key":"state","flags":"vn","result":{"currentTool":0}})");
	EXPECT_EQ(text(view, "name"), "Left");
	ASSERT_EQ(lv_obj_get_child_count(list(view, "heaters")), 1u);
	EXPECT_EQ(text(lv_obj_get_child(list(view, "heaters"), 0), "heater"), "1");
}

TEST_F(TestOmJoins, AScopeNamesOneElement)
{
	lv_xml_register_component_from_data("scope_detail", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="current" bind_text="row.current"/>
				<lv_label name="active" bind_text="row.active|int" bind_text-fmt="%d°C"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("scope_heater", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<ds_scope name="detail" path="om.heat.heaters[{row.value}]" item="scope_detail" width="100%" height="content"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("scope_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<ds_list name="heaters" list="row.heaters" item="scope_heater" width="100%" height="content"/>
			</view>
		</component>)");
	receive(JOIN_HEAT);
	receive(JOIN_TOOLS);
	auto& view = show("scope_tools", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="tools" list="om.tools" item="scope_tool" width="100%" height="content"/>
			</view>
		</component>)");

	lv_obj_t* heaters = lv_obj_find_by_name(lv_obj_get_child(list(view, "tools"), 0), "heaters");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "current"), "40");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "active"), "210°C");

	receive(R"({"key":"tools","flags":"v","result":[{"heaters":[0,3]},{"heaters":[3]}]})");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "current"), "50");
	EXPECT_EQ(text(lv_obj_get_child(heaters, 1), "active"), "220°C");
}

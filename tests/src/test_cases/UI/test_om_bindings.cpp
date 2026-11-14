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

	EXPECT_EQ(findConverter("")->toText(number, ""), "61.25");
	EXPECT_EQ(findConverter("")->toText(missing, ""), "");
	EXPECT_EQ(findConverter("int")->toInteger(number, ""), 61);
	EXPECT_EQ(findConverter("int")->toInteger(yes, ""), 1);
	EXPECT_EQ(findConverter("int")->toInteger(state, ""), 0);
	EXPECT_EQ(findConverter("percent")->toInteger(fraction, ""), 46);
	EXPECT_EQ(findConverter("scale")->toInteger(number, "10"), 613);
	EXPECT_EQ(findConverter("fixed")->toText(number, "1"), "61.2");
	EXPECT_EQ(findConverter("fixed")->toText(state, "1"), "");
	EXPECT_EQ(findConverter("exists")->toInteger(missing, ""), 0);
	EXPECT_EQ(findConverter("exists")->toInteger(number, ""), 1);
	EXPECT_EQ(findConverter("eq")->toInteger(state, "active"), 1);
	EXPECT_EQ(findConverter("eq")->toInteger(state, "fault"), 0);
	EXPECT_EQ(findConverter("duration")->toText(seconds, ""), "1:02:05");
	EXPECT_EQ(findConverter("duration")->toText(fraction, ""), "0:00");
	EXPECT_EQ(findConverter("bytes")->toText(size, ""), "1.5 MB");
	EXPECT_EQ(findConverter("tr")->toText(state, "no.such.prefix"), "active") << "no translation gives the value";
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
				<bind_flag_if_eq subject="exists" ref_value="0" flag="hidden"/>
				<lv_label name="number" bind_text="index" bind_text-fmt="%d"/>
				<lv_label name="current" bind_text="current|fixed:1"/>
				<lv_label name="state" bind_text="state"/>
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
				<lv_label name="heater" bind_text="value"/>
				<lv_label name="heater_int" bind_text="value|int" bind_text-fmt="H%d"/>
			</view>
		</component>)");
	lv_xml_register_component_from_data("om_tool", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content" flex_flow="column">
				<lv_label name="name" bind_text="name"/>
				<ds_list name="heaters" list="heaters" item="om_tool_heater" width="100%" height="content"/>
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

TEST_F(TestOmBindings, AnItemsFieldWinsOverAnOuterNameAndOtherNamesPassThrough)
{
	UI::Xml::StringSubject<16> outer("outer");
	UI::Xml::StringSubject<16> title("title");
	UI::Xml::Globals::add("state", outer);
	UI::Xml::Globals::add("test.title", title);
	lv_xml_register_component_from_data("om_shadow_row", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="state" bind_text="state"/>
				<lv_label name="title" bind_text="test.title"/>
			</view>
		</component>)");
	receive(HEAT);
	auto& view = show("om_shadow", R"(
		<component>
			<view extends="lv_obj" width="100%" height="100%">
				<ds_list name="heaters" list="om.heat.heaters" item="om_shadow_row" width="100%" height="content"/>
			</view>
		</component>)");

	lv_obj_t* row = lv_obj_get_child(list(view, "heaters"), 1);
	EXPECT_EQ(text(row, "state"), "off") << "the item's field wins over the global";
	EXPECT_EQ(text(row, "title"), "title") << "a name the item doesn't have is found outside";

	m_views.clear();
	UI::Xml::Globals::remove("state");
	UI::Xml::Globals::remove("test.title");
}

TEST_F(TestOmBindings, AFieldTheItemHasntSentYetIsStillItsOwn)
{
	lv_xml_register_component_from_data("om_late_row", R"(
		<component>
			<view extends="lv_obj" width="100%" height="content">
				<lv_label name="max" bind_text="max" />
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

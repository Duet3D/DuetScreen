/*
 * test_om_tree.cpp
 *
 * The object model tree (src/ObjectModel/Tree): paths, and how the JSON decoder's responses are merged into it.
 * Every recorded response in tests/object_model must arrive whole; the merge rules are checked with small responses.
 */

#include "ObjectModel/Tree/Tree.h"
#include "test_utils/TestSuite.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using OM::ChangeSet;
using OM::Node;
using OM::Path;
using OM::Tree;

class TestOmTree : public TestSuite
{
  protected:
	/** Start each test with no changes waiting from an earlier one */
	TestOmTree() { Tree::get().takeChanges(); }

	/** The node at `path` in the object model */
	static const Node& at(std::string_view path) { return Tree::get().root().at(path); }

	/** The changed paths since the last call, as a sorted list */
	static std::vector<std::string> changes()
	{
		const ChangeSet set = Tree::get().takeChanges();
		return {set.paths().begin(), set.paths().end()};
	}

	/** Check that `node` holds exactly what `json` does; `where` names it in failures */
	static void expectSame(const Node& node, const nlohmann::json& json, const std::string& where)
	{
		switch (json.type())
		{
		case nlohmann::json::value_t::object:
			ASSERT_TRUE(node.isObject()) << where;
			EXPECT_EQ(node.size(), json.size()) << where;
			for (const auto& [name, child] : json.items())
			{
				expectSame(node[std::string_view(name)], child, where + "." + name);
			}
			break;
		case nlohmann::json::value_t::array:
			ASSERT_TRUE(node.isArray()) << where;
			ASSERT_EQ(node.size(), json.size()) << where;
			for (size_t i = 0; i < json.size(); i++)
			{
				expectSame(node[i], json[i], where + "[" + std::to_string(i) + "]");
			}
			break;
		case nlohmann::json::value_t::null:
			EXPECT_EQ(node.kind(), Node::Kind::null) << where;
			break;
		case nlohmann::json::value_t::boolean:
			EXPECT_EQ(node.kind(), Node::Kind::boolean) << where;
			EXPECT_EQ(node.boolean(), json.get<bool>()) << where;
			break;
		case nlohmann::json::value_t::string:
			EXPECT_EQ(node.kind(), Node::Kind::string) << where;
			EXPECT_EQ(node.text(), json.get<std::string>()) << where;
			break;
		default:
			EXPECT_EQ(node.kind(), Node::Kind::number) << where;
			EXPECT_DOUBLE_EQ(node.number(), json.get<double>()) << where;
			break;
		}
	}
};

TEST_F(TestOmTree, PathsReadAndWriteAsRepRapFirmwareDoes)
{
	const auto path = Path::parse("heat.heaters[1].current");
	ASSERT_TRUE(path);
	EXPECT_EQ(path->size(), 4u);
	EXPECT_EQ(path->str(), "heat.heaters[1].current");
	EXPECT_EQ(Path::parse("move.axes[0].drives[2]")->str(), "move.axes[0].drives[2]");
	EXPECT_EQ(Path::parse("tools[0].axes[1][0]")->str(), "tools[0].axes[1][0]");
	EXPECT_EQ(Path::parse("")->str(), "");

	for (const char* bad :
		 {"heat.", ".heat", "heat..x", "heat[", "heat[]", "heat[x]", "[0]", "heat.[0]", "a[0]b", "a b"})
	{
		EXPECT_FALSE(Path::parse(bad)) << bad;
	}

	const size_t indices[] = {1, 2};
	EXPECT_EQ(Path::fromDecoderId("heat:heaters^:current", indices)->str(), "heat.heaters[1].current");
	EXPECT_EQ(Path::fromDecoderId("tools^:axes^", indices)->str(), "tools[1].axes[2]");
	EXPECT_EQ(Path::fromDecoderId(":heaters^", indices)->str(), "heaters[1]");
	EXPECT_FALSE(Path::fromDecoderId("a^:b^:c^", indices)) << "more '^' than indices";

	EXPECT_TRUE(OM::isAtOrBelow("heat.heaters[1].current", "heat.heaters[1]"));
	EXPECT_TRUE(OM::isAtOrBelow("heat.heaters[1]", "heat.heaters[1]"));
	EXPECT_TRUE(OM::isAtOrBelow("heat.heaters[1]", "heat.heaters"));
	EXPECT_TRUE(OM::isAtOrBelow("heat", ""));
	EXPECT_FALSE(OM::isAtOrBelow("heat.heaters[10]", "heat.heaters[1]"));
	EXPECT_FALSE(OM::isAtOrBelow("heaters", "heat"));
}

TEST_F(TestOmTree, MissingNodesReadAsNull)
{
	const Node& node = at("heat.heaters[7].current");
	EXPECT_EQ(node.kind(), Node::Kind::missing);
	EXPECT_TRUE(node.isNull());
	EXPECT_FALSE(node.exists());
	EXPECT_EQ(node.text(), "");
	EXPECT_EQ(node.number(3.5), 3.5);
	EXPECT_EQ(node.size(), 0u);
	EXPECT_EQ(&node["anything"][4], &Node::missing());
}

TEST_F(TestOmTree, EveryRecordedResponseArrivesWhole)
{
	size_t checked = 0;
	for (const auto& folder : {"5_axis", "test_bench", "job", "m291"})
	{
		for (const auto& entry : std::filesystem::directory_iterator(std::string("tests/object_model/") + folder))
		{
			std::ifstream file(entry.path());
			const auto json = nlohmann::json::parse(file);
			if (json.value("next", 0) != 0)
			{
				continue; // A partial array; see PartialArraysReplaceOnlyWhatWasSent
			}
			OM::RemoveAll();
			load_model_data_from_file(entry.path().string());
			const std::string key = json["key"].get<std::string>();
			expectSame(at(key), json["result"], entry.path().string() + ":" + key);
			checked++;
		}
	}
	EXPECT_GT(checked, 40u);
}

TEST_F(TestOmTree, ValuesKeepTheirJsonKind)
{
	load_model_data(R"({"key":"x","flags":"v","result":{"s":"123","n":123,"f":-1.5e-3,"b":true,"z":null,"e":""}})"
					"\n");
	EXPECT_EQ(at("x.s").kind(), Node::Kind::string);
	EXPECT_EQ(at("x.s").number(), 123);
	EXPECT_EQ(at("x.n").kind(), Node::Kind::number);
	EXPECT_EQ(at("x.n").text(), "123");
	EXPECT_DOUBLE_EQ(at("x.f").number(), -1.5e-3);
	EXPECT_EQ(at("x.f").text(), "-1.5e-3");
	EXPECT_EQ(at("x.b").kind(), Node::Kind::boolean);
	EXPECT_EQ(at("x.b").text(), "true");
	EXPECT_EQ(at("x.b").integer(), 1);
	EXPECT_EQ(at("x.z").kind(), Node::Kind::null);
	EXPECT_EQ(at("x.e").kind(), Node::Kind::string);
	EXPECT_EQ(at("x.e").text(), "");
}

TEST_F(TestOmTree, ADetailedResponseReplacesItsKey)
{
	load_model_data(R"({"key":"heat","flags":"v","result":{"a":1,"b":2,"c":{"d":3}}})"
					"\n");
	EXPECT_EQ(changes(), std::vector<std::string>{"heat"});

	load_model_data(R"({"key":"heat","flags":"v","result":{"a":1,"c":{"d":4}}})"
					"\n");
	EXPECT_EQ(at("heat.b").kind(), Node::Kind::missing) << "a field a detailed response leaves out is removed";
	EXPECT_EQ(at("heat.c.d").integer(), 4);
	EXPECT_EQ(changes(), (std::vector<std::string>{"heat.b", "heat.c.d"})) << "only what changed is noted";
}

TEST_F(TestOmTree, ALiveResponseOnlyUpdatesWhatItSends)
{
	load_model_data(
		R"({"key":"heat","flags":"v","result":{"heaters":[{"current":20,"max":280},{"current":21,"max":120}]}})"
		"\n");
	changes();

	load_model_data(R"({"key":"","flags":"d99f","result":{"heat":{"heaters":[{"current":25},{"current":21}]}}})"
					"\n");
	EXPECT_EQ(at("heat.heaters[0].current").integer(), 25);
	EXPECT_EQ(at("heat.heaters[0].max").integer(), 280) << "a field a live response doesn't send is kept";
	EXPECT_EQ(changes(), std::vector<std::string>{"heat.heaters[0].current"});
}

TEST_F(TestOmTree, ArraysTakeTheLengthSent)
{
	load_model_data(R"({"key":"fans","flags":"v","result":[{"a":1},{"a":2},{"a":3}]})"
					"\n");
	changes();

	load_model_data(R"({"key":"fans","flags":"v","result":[null,{"a":2}]})"
					"\n");
	EXPECT_EQ(at("fans").size(), 2u);
	EXPECT_EQ(at("fans[0]").kind(), Node::Kind::null);
	EXPECT_EQ(changes(), std::vector<std::string>{"fans"}) << "a length change notes the array itself";

	load_model_data(R"({"key":"fans","flags":"v","result":[{},{"a":2}]})"
					"\n");
	EXPECT_TRUE(at("fans[0]").isObject()) << "an object without fields is still an object";
	EXPECT_EQ(changes(), std::vector<std::string>{"fans[0]"});

	load_model_data(R"({"key":"fans","flags":"v","result":[]})"
					"\n");
	EXPECT_TRUE(at("fans").isArray());
	EXPECT_EQ(at("fans").size(), 0u);
}

TEST_F(TestOmTree, PartialArraysReplaceOnlyWhatWasSent)
{
	load_model_data(R"({"key":"move.axes","flags":"v","result":[{"letter":"X"},{"letter":"Y"},{"letter":"Z"}]})"
					"\n");
	changes();

	// The first part of a longer array: elements past it stay
	load_model_data(R"({"key":"move.axes","flags":"va0","result":[{"letter":"A"}],"next":1})"
					"\n");
	EXPECT_EQ(at("move.axes").size(), 3u);
	EXPECT_EQ(at("move.axes[0].letter").text(), "A");
	EXPECT_EQ(changes(), std::vector<std::string>{"move.axes[0].letter"});

	// The last part sets the length
	load_model_data(R"({"key":"move.axes","flags":"va1","result":[{"letter":"B"}],"next":0})"
					"\n");
	EXPECT_EQ(at("move.axes").size(), 2u);
	EXPECT_EQ(at("move.axes[0].letter").text(), "A");
	EXPECT_EQ(at("move.axes[1].letter").text(), "B");

	// A recorded 10-axis machine, sent as a first part
	OM::RemoveAll();
	load_model_data_from_file("tests/object_model/test_bench/model_10_axes.json");
	EXPECT_EQ(at("move.axes").size(), 10u);
	EXPECT_TRUE(at("move.axes[9]").isObject());
}

TEST_F(TestOmTree, AResultMayBeAValue)
{
	load_model_data(R"({"key":"state.status","flags":"v","result":"idle"})"
					"\n");
	EXPECT_EQ(at("state.status").text(), "idle");
	EXPECT_TRUE(at("state").isObject());
	EXPECT_EQ(changes(), (std::vector<std::string>{"state", "state.status"}));
}

TEST_F(TestOmTree, AResponseCutShortChangesNothing)
{
	load_model_data(R"({"key":"heat","flags":"v","result":{"a":5)"
					"\n");
	EXPECT_EQ(at("heat").kind(), Node::Kind::missing);
	EXPECT_TRUE(changes().empty());

	load_model_data(R"({"key":"heat","flags":"v","result":{"a":6}})"
					"\n");
	EXPECT_EQ(at("heat.a").integer(), 6);
}

TEST_F(TestOmTree, AResponseThatChangesNothingNotesNothing)
{
	const char* response = R"({"key":"heat","flags":"v","result":{"a":1,"b":[1,2]}})"
						   "\n";
	load_model_data(response);
	changes();
	load_model_data(response);
	EXPECT_TRUE(changes().empty());
}

TEST_F(TestOmTree, ListenersAreToldOnceUntilTheChangesAreTaken)
{
	Tree tree;
	int told = 0;
	const size_t id = tree.addListener([&] { told++; });

	OM::Patch patch;
	patch.setKey("a");
	patch.setFlags("v");
	patch.value(Path(), Node::Kind::number, "1");
	tree.apply(patch);
	EXPECT_EQ(told, 1);

	patch.value(Path(), Node::Kind::number, "2");
	tree.apply(patch);
	EXPECT_EQ(told, 1) << "not told again while changes wait";

	EXPECT_EQ(tree.takeChanges().paths().size(), 1u);
	patch.value(Path(), Node::Kind::number, "3");
	tree.apply(patch);
	EXPECT_EQ(told, 2);

	tree.takeChanges();
	tree.clear();
	EXPECT_EQ(told, 3);
	EXPECT_TRUE(tree.root().isObject());
	EXPECT_EQ(tree.root().size(), 0u);
	EXPECT_TRUE(tree.takeChanges().touches("a.b"));

	tree.removeListener(id);
	tree.apply(patch);
	EXPECT_EQ(told, 3) << "a removed listener isn't told";
}

/*
 * test_om_json.cpp
 *
 * Reading JSON text as a Node (src/ObjectModel/Tree/Json.h): what the printer sends, standalone and through DSF,
 * read exactly, and anything else refused.
 */

#include "ObjectModel/Tree/Json.h"
#include <gtest/gtest.h>
#include <string>

using OM::Node;
using OM::parseJson;

TEST(OmJson, ValuesKeepTheirKindAndText)
{
	const auto node = parseJson(R"({"s":"123","i":-42,"u":7,"f":-1.5e-3,"g":20.10,"t":true,"n":false,"z":null})");
	ASSERT_TRUE(node) << node.error().message;
	EXPECT_EQ((*node)["s"].kind(), Node::Kind::string);
	EXPECT_EQ((*node)["i"].text(), "-42");
	EXPECT_EQ((*node)["u"].integer(), 7);
	EXPECT_EQ((*node)["f"].text(), "-1.5e-3") << "a number keeps the text it was sent as";
	EXPECT_EQ((*node)["g"].text(), "20.10");
	EXPECT_DOUBLE_EQ((*node)["g"].number(), 20.1);
	EXPECT_EQ((*node)["t"].kind(), Node::Kind::boolean);
	EXPECT_FALSE((*node)["n"].boolean(true));
	EXPECT_EQ((*node)["z"].kind(), Node::Kind::null);
}

TEST(OmJson, ObjectsAndArraysNest)
{
	const auto node = parseJson(R"({"a":[1,[2,3],{"b":[]},{}],"c":{"d":{"e":"f"}}})");
	ASSERT_TRUE(node);
	EXPECT_EQ(node->at("a").size(), 4u);
	EXPECT_EQ(node->at("a[1][1]").integer(), 3);
	EXPECT_TRUE(node->at("a[2].b").isArray());
	EXPECT_EQ(node->at("a[2].b").size(), 0u);
	EXPECT_TRUE(node->at("a[3]").isObject());
	EXPECT_EQ(node->at("c.d.e").text(), "f");

	const auto value = parseJson("  \"just a string\"\n");
	ASSERT_TRUE(value);
	EXPECT_EQ(value->text(), "just a string");
}

TEST(OmJson, EscapesBecomeUtf8)
{
	// DSF escapes '+', '\'', '&', '<', '>' and everything outside ASCII, as .NET does by default
	const auto node = parseJson(R"({"by":"PrusaSlicer 2.7.1\u002Bwin64",)"
								R"("name":"Andy\u0027s \u0026 \u003Cpart\u003E.gcode",)"
								R"("umlaut":"Gr\u00F6\u00DFe","cjk":"\u6E29\u5EA6",)"
								R"("emoji":"\uD83D\uDD25","raw":"Größe",)"
								R"("controls":"a\"b\\c\/d\n\t\r\b\f"})");
	ASSERT_TRUE(node) << node.error().message;
	EXPECT_EQ((*node)["by"].text(), "PrusaSlicer 2.7.1+win64");
	EXPECT_EQ((*node)["name"].text(), "Andy's & <part>.gcode");
	EXPECT_EQ((*node)["umlaut"].text(), "Größe");
	EXPECT_EQ((*node)["cjk"].text(), "温度");
	EXPECT_EQ((*node)["emoji"].text(), "\xF0\x9F\x94\xA5") << "a surrogate pair is one character";
	EXPECT_EQ((*node)["raw"].text(), "Größe") << "UTF-8 sent as it is stays as it is";
	EXPECT_EQ((*node)["controls"].text(), "a\"b\\c/d\n\t\r\b\f");
}

TEST(OmJson, AnythingButJsonIsRefused)
{
	for (const char* bad : {"",
							"{",
							R"({"a":1)",
							R"({"a":1}})",
							R"({"a":1} {"b":2})",
							R"({"a":01})",
							R"({"a":.5})",
							R"({"a":tru})",
							R"({"a":"unterminated})",
							"{\"a\":\"tab\there\"}",
							R"({"a":"\x"})",
							R"({"a":"\u12"})",
							R"({a:1})",
							R"(ok)"})
	{
		const auto node = parseJson(bad);
		EXPECT_FALSE(node) << bad;
		if (!node)
		{
			EXPECT_FALSE(node.error().message.empty()) << bad;
		}
	}
}

TEST(OmJson, NestingIsLimited)
{
	const std::string fine = std::string(OM::maxJsonDepth, '[') + std::string(OM::maxJsonDepth, ']');
	EXPECT_TRUE(parseJson(fine));

	const std::string tooDeep = std::string(OM::maxJsonDepth + 1, '[') + std::string(OM::maxJsonDepth + 1, ']');
	const auto node = parseJson(tooDeep);
	ASSERT_FALSE(node);
	EXPECT_NE(node.error().message.find("nesting"), std::string::npos);
}

TEST(OmJson, ANodeMovesWithItsChildren)
{
	auto parsed = parseJson(R"({"a":{"b":[1,2]}})");
	ASSERT_TRUE(parsed);
	Node moved = std::move(*parsed);
	EXPECT_EQ(moved.at("a.b[1]").integer(), 2);
	EXPECT_EQ(parsed->kind(), Node::Kind::null) << "the node moved from is left null";
	EXPECT_EQ(parsed->size(), 0u);
}

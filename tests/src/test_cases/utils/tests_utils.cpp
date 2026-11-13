#include <gtest/gtest.h>
#include <string>

#include "Debug.h"
#include "utils/utils.h"

TEST(Utils, BoundInt)
{
	EXPECT_EQ(utils::bound(5, 0, 10), 5);
	EXPECT_EQ(utils::bound(-1, 0, 10), 0);
	EXPECT_EQ(utils::bound(42, 0, 10), 10);
}

TEST(Utils, ReplaceSubstring)
{
	std::string s = "foo_bar_bar";
	utils::replaceSubstring(s, "bar", "baz");
	EXPECT_EQ(s, std::string("foo_baz_baz"));
}

TEST(Utils, SplitString)
{
	auto parts = utils::splitString("a,b,,c", ",");
	ASSERT_EQ(parts.size(), 3u);
	EXPECT_EQ(parts[0], std::string("a"));
	EXPECT_EQ(parts[1], std::string("b"));
	EXPECT_EQ(parts[2], std::string("c"));
}

TEST(Utils, FindInstance)
{
	size_t pos = utils::findInstance("abc abc abc", "abc", 2);
	// The second instance should begin at index 4 (0-based): "abc abc ..."
	EXPECT_EQ(pos, 5u); // function returns pos+1 after counting
}

TEST(Utils, Format)
{
	EXPECT_EQ(fmt::format("Hello, {:s}", "World"), "Hello, World");
	EXPECT_EQ(fmt::format("The answer is {:d}", 42), "The answer is 42");
	EXPECT_EQ(fmt::format("Pi is approximately {:.2f}", 3.14159), "Pi is approximately 3.14");
	EXPECT_EQ(fmt::format("Float: {:g}", 3.14000), "Float: 3.14");
	EXPECT_EQ(fmt::format("Float: {:g}", 3.00000), "Float: 3");
	EXPECT_EQ(fmt::format("Float: {:g}", 3.00012), "Float: 3.00012");
	EXPECT_EQ(fmt::format("Float: {:.2g}", 3.0123), "Float: 3");
	EXPECT_EQ(fmt::format("Float: {:.3g}", 3.0123), "Float: 3.01");
	EXPECT_EQ(fmt::format("Reorder: {2:s}, {0:d}, {1:.1f}", 42, 3.14, "Hello"), "Reorder: Hello, 42, 3.1");
	EXPECT_EQ(fmt::format("Multiple: {0:s}, {0:s}, {0:s}", "Echo"), "Multiple: Echo, Echo, Echo");
	EXPECT_EQ(fmt::format("Named arg: {name:s}, {value:d}", fmt::arg("name", "Count"), fmt::arg("value", 5)),
			  "Named arg: Count, 5");
}

TEST(Utils, FormatFloat)
{
	EXPECT_EQ(utils::formatFloat(100.0, 0), "100");
	EXPECT_EQ(utils::formatFloat(120.0), "120");
	EXPECT_EQ(utils::formatFloat(0.1), "0.1");
	EXPECT_EQ(utils::formatFloat(2.50, 2), "2.5");
	EXPECT_EQ(utils::formatFloat(3.0, 2), "3");
	EXPECT_EQ(utils::formatFloat(0.0, 0), "0");
	EXPECT_EQ(utils::formatFloat(1.26, 1), "1.3");
}

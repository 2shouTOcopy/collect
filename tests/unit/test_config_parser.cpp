#include <gtest/gtest.h>
#include "config/ConfigParser.h"

// ─── Empty / Comment Input ──────────────────────────────────

TEST(ConfigParserTest, EmptyInput)
{
	auto items = ConfigParser::ParseString("");
	EXPECT_TRUE(items.empty());
}

TEST(ConfigParserTest, CommentOnly)
{
	auto items = ConfigParser::ParseString(
		"# This is a comment\n"
		"# Another comment\n");
	EXPECT_TRUE(items.empty());
}

TEST(ConfigParserTest, BlankLines)
{
	auto items = ConfigParser::ParseString("\n\n\n");
	EXPECT_TRUE(items.empty());
}

// ─── Simple Key-Value ───────────────────────────────────────

TEST(ConfigParserTest, SimpleKeyValue)
{
	auto items = ConfigParser::ParseString("Hostname \"localhost\"\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].key, "Hostname");
	ASSERT_EQ(items[0].values.size(), 1u);
	EXPECT_EQ(items[0].values[0], "localhost");
	EXPECT_TRUE(items[0].children.empty());
}

TEST(ConfigParserTest, KeyWithoutQuotes)
{
	auto items = ConfigParser::ParseString("FQDNLookup false\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].key, "FQDNLookup");
	ASSERT_EQ(items[0].values.size(), 1u);
	EXPECT_EQ(items[0].values[0], "false");
}

TEST(ConfigParserTest, KeyNoValue)
{
	auto items = ConfigParser::ParseString("SomeDirective\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].key, "SomeDirective");
	EXPECT_TRUE(items[0].values.empty());
}

TEST(ConfigParserTest, MultipleValues)
{
	auto items = ConfigParser::ParseString("Interval 10\nHostname \"test\"\n");
	ASSERT_EQ(items.size(), 2u);
	EXPECT_EQ(items[0].key, "Interval");
	EXPECT_EQ(items[0].values[0], "10");
	EXPECT_EQ(items[1].key, "Hostname");
	EXPECT_EQ(items[1].values[0], "test");
}

// ─── Quoted Strings ─────────────────────────────────────────

TEST(ConfigParserTest, SingleQuotedString)
{
	auto items = ConfigParser::ParseString("BaseDir '/var/lib/collect'\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].values[0], "/var/lib/collect");
}

TEST(ConfigParserTest, QuotedStringWithSpaces)
{
	auto items = ConfigParser::ParseString(
		"Desc \"a long description here\"\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].values[0], "a long description here");
}

// ─── Block Parsing ──────────────────────────────────────────

TEST(ConfigParserTest, SimpleBlock)
{
	auto items = ConfigParser::ParseString(
		"<Plugin cpu>\n"
		"  ReportByCpu false\n"
		"  ValuesPercentage true\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].key, "Plugin");
	ASSERT_EQ(items[0].values.size(), 1u);
	EXPECT_EQ(items[0].values[0], "cpu");

	ASSERT_EQ(items[0].children.size(), 2u);
	EXPECT_EQ(items[0].children[0].key, "ReportByCpu");
	EXPECT_EQ(items[0].children[0].values[0], "false");
	EXPECT_EQ(items[0].children[1].key, "ValuesPercentage");
	EXPECT_EQ(items[0].children[1].values[0], "true");
}

TEST(ConfigParserTest, BlockWithQuotedArg)
{
	auto items = ConfigParser::ParseString(
		"<Plugin \"csv\">\n"
		"  DataDir \"/tmp/csv\"\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].values[0], "csv");
	ASSERT_EQ(items[0].children.size(), 1u);
	EXPECT_EQ(items[0].children[0].values[0], "/tmp/csv");
}

TEST(ConfigParserTest, EmptyBlock)
{
	auto items = ConfigParser::ParseString(
		"<Plugin empty>\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].key, "Plugin");
	EXPECT_TRUE(items[0].children.empty());
}

// ─── Mixed Content ──────────────────────────────────────────

TEST(ConfigParserTest, MixedGlobalsAndBlocks)
{
	auto items = ConfigParser::ParseString(
		"# Global settings\n"
		"Hostname \"example.com\"\n"
		"Interval 30\n"
		"\n"
		"LoadPlugin cpu\n"
		"LoadPlugin memory\n"
		"\n"
		"<Plugin cpu>\n"
		"  ReportByCpu true\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 5u);

	EXPECT_EQ(items[0].key, "Hostname");
	EXPECT_EQ(items[0].values[0], "example.com");

	EXPECT_EQ(items[1].key, "Interval");
	EXPECT_EQ(items[1].values[0], "30");

	EXPECT_EQ(items[2].key, "LoadPlugin");
	EXPECT_EQ(items[2].values[0], "cpu");

	EXPECT_EQ(items[3].key, "LoadPlugin");
	EXPECT_EQ(items[3].values[0], "memory");

	EXPECT_EQ(items[4].key, "Plugin");
	EXPECT_EQ(items[4].values[0], "cpu");
	ASSERT_EQ(items[4].children.size(), 1u);
}

// ─── Comment Handling ───────────────────────────────────────

TEST(ConfigParserTest, InlineCommentsIgnored)
{
	// Lines starting with '#' are skipped entirely
	auto items = ConfigParser::ParseString(
		"LoadPlugin cpu\n"
		"#LoadPlugin disabled\n"
		"LoadPlugin memory\n");

	ASSERT_EQ(items.size(), 2u);
	EXPECT_EQ(items[0].values[0], "cpu");
	EXPECT_EQ(items[1].values[0], "memory");
}

TEST(ConfigParserTest, CommentsInsideBlock)
{
	auto items = ConfigParser::ParseString(
		"<Plugin test>\n"
		"  Key1 value1\n"
		"#  Key2 value2\n"
		"  Key3 value3\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 1u);
	ASSERT_EQ(items[0].children.size(), 2u);
	EXPECT_EQ(items[0].children[0].key, "Key1");
	EXPECT_EQ(items[0].children[1].key, "Key3");
}

// ─── Whitespace / Tabs ─────────────────────────────────────

TEST(ConfigParserTest, TabIndented)
{
	auto items = ConfigParser::ParseString(
		"<Plugin csv>\n"
		"\tDataDir \"/tmp\"\n"
		"\tStoreRates false\n"
		"</Plugin>\n");

	ASSERT_EQ(items.size(), 1u);
	ASSERT_EQ(items[0].children.size(), 2u);
	EXPECT_EQ(items[0].children[0].key, "DataDir");
	EXPECT_EQ(items[0].children[0].values[0], "/tmp");
}

TEST(ConfigParserTest, MultipleSpacesAroundValue)
{
	auto items = ConfigParser::ParseString(
		"Hostname     \"test\"\n");
	ASSERT_EQ(items.size(), 1u);
	EXPECT_EQ(items[0].values[0], "test");
}

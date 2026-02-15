#include <gtest/gtest.h>
#include "utils/StringUtils.h"

// ─── Trim ──────────────────────────────────────────────────

TEST(StringUtilsTest, TrimBothSides)
{
	EXPECT_EQ(StringUtils::Trim("  hello  "), "hello");
}

TEST(StringUtilsTest, TrimLeadingOnly)
{
	EXPECT_EQ(StringUtils::Trim("\t\thello"), "hello");
}

TEST(StringUtilsTest, TrimTrailingOnly)
{
	EXPECT_EQ(StringUtils::Trim("hello\r\n"), "hello");
}

TEST(StringUtilsTest, TrimAllWhitespace)
{
	EXPECT_EQ(StringUtils::Trim("   \t\n  "), "");
}

TEST(StringUtilsTest, TrimEmpty)
{
	EXPECT_EQ(StringUtils::Trim(""), "");
}

TEST(StringUtilsTest, TrimNoWhitespace)
{
	EXPECT_EQ(StringUtils::Trim("hello"), "hello");
}

// ─── Split ─────────────────────────────────────────────────

TEST(StringUtilsTest, SplitByComma)
{
	auto parts = StringUtils::Split("a,b,c", ',');
	ASSERT_EQ(parts.size(), 3u);
	EXPECT_EQ(parts[0], "a");
	EXPECT_EQ(parts[1], "b");
	EXPECT_EQ(parts[2], "c");
}

TEST(StringUtilsTest, SplitSingleElement)
{
	auto parts = StringUtils::Split("hello", ',');
	ASSERT_EQ(parts.size(), 1u);
	EXPECT_EQ(parts[0], "hello");
}

TEST(StringUtilsTest, SplitEmpty)
{
	auto parts = StringUtils::Split("", ',');
	// std::getline on empty string produces no parts
	EXPECT_EQ(parts.size(), 0u);
}

TEST(StringUtilsTest, SplitTrailingDelimiter)
{
	auto parts = StringUtils::Split("a,b,", ',');
	// std::getline does not produce trailing empty part
	ASSERT_EQ(parts.size(), 2u);
	EXPECT_EQ(parts[0], "a");
	EXPECT_EQ(parts[1], "b");
}

TEST(StringUtilsTest, SplitBySlash)
{
	auto parts = StringUtils::Split("usr/lib/collect", '/');
	ASSERT_EQ(parts.size(), 3u);
	EXPECT_EQ(parts[0], "usr");
	EXPECT_EQ(parts[2], "collect");
}

// ─── EqualsIgnoreCase ──────────────────────────────────────

TEST(StringUtilsTest, EqualsIgnoreCaseMatch)
{
	EXPECT_TRUE(StringUtils::EqualsIgnoreCase("Hello", "hello"));
	EXPECT_TRUE(StringUtils::EqualsIgnoreCase("DEBUG", "debug"));
	EXPECT_TRUE(StringUtils::EqualsIgnoreCase("ABC", "abc"));
}

TEST(StringUtilsTest, EqualsIgnoreCaseNoMatch)
{
	EXPECT_FALSE(StringUtils::EqualsIgnoreCase("hello", "world"));
	EXPECT_FALSE(StringUtils::EqualsIgnoreCase("abc", "abcd"));
}

TEST(StringUtilsTest, EqualsIgnoreCaseEmpty)
{
	EXPECT_TRUE(StringUtils::EqualsIgnoreCase("", ""));
	EXPECT_FALSE(StringUtils::EqualsIgnoreCase("", "a"));
}

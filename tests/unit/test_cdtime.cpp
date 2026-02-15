#include <gtest/gtest.h>
#include "types/CdTime.h"

#include <chrono>

// ─── Factory Methods ───────────────────────────────────────

TEST(CdTimeTest, DefaultIsZero)
{
	CdTime t;
	EXPECT_EQ(t.Raw(), 0u);
	EXPECT_DOUBLE_EQ(t.ToDouble(), 0.0);
}

TEST(CdTimeTest, FromDoubleSeconds)
{
	CdTime t = CdTime::FromDouble(1.5);
	EXPECT_EQ(t.Raw(), 1500000000u);
	EXPECT_DOUBLE_EQ(t.ToDouble(), 1.5);
}

TEST(CdTimeTest, FromDoubleZero)
{
	CdTime t = CdTime::FromDouble(0.0);
	EXPECT_EQ(t.Raw(), 0u);
}

TEST(CdTimeTest, ExplicitConstructor)
{
	CdTime t(42000000000u);  // 42 seconds
	EXPECT_EQ(t.Raw(), 42000000000u);
	EXPECT_DOUBLE_EQ(t.ToDouble(), 42.0);
}

TEST(CdTimeTest, FromDuration)
{
	CdTime t = CdTime::FromDuration(std::chrono::milliseconds(500));
	EXPECT_EQ(t.Raw(), 500000000u);
	EXPECT_DOUBLE_EQ(t.ToDouble(), 0.5);
}

TEST(CdTimeTest, ToDuration)
{
	CdTime t = CdTime::FromDouble(2.0);
	auto dur = t.ToDuration();
	EXPECT_EQ(dur.count(), 2000000000);
}

TEST(CdTimeTest, NowReturnsNonZero)
{
	CdTime t = CdTime::Now();
	EXPECT_GT(t.Raw(), 0u);
}

// ─── Comparison Operators ──────────────────────────────────

TEST(CdTimeTest, LessThan)
{
	CdTime a = CdTime::FromDouble(1.0);
	CdTime b = CdTime::FromDouble(2.0);
	EXPECT_TRUE(a < b);
	EXPECT_FALSE(b < a);
}

TEST(CdTimeTest, Equality)
{
	CdTime a = CdTime::FromDouble(3.0);
	CdTime b = CdTime::FromDouble(3.0);
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
}

TEST(CdTimeTest, GreaterThan)
{
	CdTime a = CdTime::FromDouble(5.0);
	CdTime b = CdTime::FromDouble(3.0);
	EXPECT_TRUE(a > b);
	EXPECT_TRUE(a >= b);
	EXPECT_FALSE(a <= b);
}

// ─── Arithmetic ────────────────────────────────────────────

TEST(CdTimeTest, Addition)
{
	CdTime a = CdTime::FromDouble(1.0);
	CdTime b = CdTime::FromDouble(2.0);
	CdTime c = a + b;
	EXPECT_DOUBLE_EQ(c.ToDouble(), 3.0);
}

TEST(CdTimeTest, Subtraction)
{
	CdTime a = CdTime::FromDouble(5.0);
	CdTime b = CdTime::FromDouble(2.0);
	CdTime c = a - b;
	EXPECT_DOUBLE_EQ(c.ToDouble(), 3.0);
}

TEST(CdTimeTest, PlusEquals)
{
	CdTime a = CdTime::FromDouble(1.0);
	a += CdTime::FromDouble(0.5);
	EXPECT_DOUBLE_EQ(a.ToDouble(), 1.5);
}

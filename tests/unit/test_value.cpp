#include <gtest/gtest.h>
#include "types/ValueList.h"

// ─── Value Type Query ──────────────────────────────────────

TEST(ValueTest, IsGauge)
{
	Value v = Value::Gauge(1.0);
	EXPECT_TRUE(v.IsGauge());
	EXPECT_FALSE(v.IsDerive());
	EXPECT_FALSE(v.IsCounter());
	EXPECT_FALSE(v.IsAbsolute());
}

TEST(ValueTest, IsDerive)
{
	Value v = Value::Derive(-42);
	EXPECT_TRUE(v.IsDerive());
	EXPECT_FALSE(v.IsGauge());
	EXPECT_FALSE(v.IsCounter());
}

TEST(ValueTest, IsCounter)
{
	Value v = Value::Counter(100);
	EXPECT_TRUE(v.IsCounter());
	EXPECT_FALSE(v.IsGauge());
	EXPECT_FALSE(v.IsDerive());
}

TEST(ValueTest, IsAbsolute)
{
	Value v = Value::Absolute(999);
	EXPECT_TRUE(v.IsAbsolute());
	EXPECT_FALSE(v.IsGauge());
}

// ─── Value Accessors ───────────────────────────────────────

TEST(ValueTest, AsGaugeCorrectType)
{
	Value v = Value::Gauge(3.14);
	EXPECT_DOUBLE_EQ(v.AsGauge(), 3.14);
}

TEST(ValueTest, AsGaugeWrongType)
{
	Value v = Value::Counter(10);
	EXPECT_TRUE(std::isnan(v.AsGauge()));
}

TEST(ValueTest, AsDeriveCorrectType)
{
	Value v = Value::Derive(-100);
	EXPECT_EQ(v.AsDerive(), -100);
}

TEST(ValueTest, AsDeriveWrongType)
{
	Value v = Value::Gauge(42.0);
	EXPECT_EQ(v.AsDerive(), 0);
}

TEST(ValueTest, AsCounterCorrectType)
{
	Value v = Value::Counter(12345);
	EXPECT_EQ(v.AsCounter(), 12345u);
}

TEST(ValueTest, AsCounterWrongType)
{
	Value v = Value::Derive(10);
	EXPECT_EQ(v.AsCounter(), 0u);
}

TEST(ValueTest, AsAbsoluteCorrectType)
{
	Value v = Value::Absolute(777);
	EXPECT_EQ(v.AsAbsolute(), 777u);
}

TEST(ValueTest, AsAbsoluteWrongType)
{
	Value v = Value::Gauge(1.0);
	EXPECT_EQ(v.AsAbsolute(), 0u);
}

// ─── DataSet Construction ──────────────────────────────────

TEST(DataSetTest, DefaultConstruction)
{
	DataSet ds;
	EXPECT_TRUE(ds.type.empty());
	EXPECT_TRUE(ds.sources.empty());
}

TEST(DataSetTest, AddSources)
{
	DataSet ds;
	ds.type = "cpu";
	DataSource s1;
	s1.name = "value";
	s1.type = DataSourceType::Gauge;
	s1.min = 0;
	s1.max = 100;
	ds.sources.push_back(s1);

	EXPECT_EQ(ds.sources.size(), 1u);
	EXPECT_EQ(ds.sources[0].name, "value");
	EXPECT_DOUBLE_EQ(ds.sources[0].max, 100.0);
}

// ─── ValueList Copy Semantics ──────────────────────────────

TEST(ValueListTest, CopyPreservesData)
{
	ValueList vl;
	vl.plugin = "memory";
	vl.type = "memory";
	vl.typeInstance = "used";
	vl.values.push_back(Value::Gauge(1024.0));
	vl.time = CdTime::FromDouble(100.0);

	ValueList copy = vl;
	EXPECT_EQ(copy.plugin, "memory");
	EXPECT_EQ(copy.typeInstance, "used");
	EXPECT_EQ(copy.values.size(), 1u);
	EXPECT_DOUBLE_EQ(copy.values[0].AsGauge(), 1024.0);
	EXPECT_EQ(copy.time, vl.time);
}

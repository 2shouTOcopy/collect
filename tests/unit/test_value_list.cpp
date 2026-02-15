#include <gtest/gtest.h>
#include "types/ValueList.h"

TEST(ValueListTest, DefaultConstruction)
{
	ValueList vl;
	EXPECT_TRUE(vl.plugin.empty());
	EXPECT_TRUE(vl.values.empty());
}

TEST(ValueListTest, GaugeValueFactory)
{
	Value v = Value::Gauge(42.5);
	EXPECT_EQ(v.type, DataSourceType::Gauge);
	EXPECT_DOUBLE_EQ(v.AsGauge(), 42.5);
}

TEST(ValueListTest, DeriveValueFactory)
{
	Value v = Value::Derive(100);
	EXPECT_EQ(v.type, DataSourceType::Derive);
	EXPECT_EQ(v.data.derive, 100);
}

TEST(ValueListTest, CounterValueFactory)
{
	Value v = Value::Counter(999);
	EXPECT_EQ(v.type, DataSourceType::Counter);
	EXPECT_EQ(v.data.counter, 999u);
}

TEST(ValueListTest, PopulateValueList)
{
	ValueList vl;
	vl.plugin = "cpu";
	vl.pluginInstance = "0";
	vl.type = "percent";
	vl.typeInstance = "user";
	vl.values.push_back(Value::Gauge(23.5));
	vl.interval = CdTime::FromDouble(10.0);

	EXPECT_EQ(vl.plugin, "cpu");
	EXPECT_EQ(vl.values.size(), 1u);
	EXPECT_DOUBLE_EQ(vl.values[0].AsGauge(), 23.5);
	EXPECT_DOUBLE_EQ(vl.interval.ToDouble(), 10.0);
}

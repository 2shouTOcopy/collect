#include <gtest/gtest.h>
#include "output/JsonFormatter.h"
#include "types/DataSet.h"
#include "types/ValueList.h"

TEST(JsonFormatterTest, ContentType)
{
	JsonFormatter fmt;
	EXPECT_EQ(fmt.ContentType(), "application/json");
}

TEST(JsonFormatterTest, FormatSingleGauge)
{
	JsonFormatter fmt;
	fmt.SetHost("test-host");

	DataSet ds;
	ds.type = "gauge";
	DataSource src;
	src.name = "value";
	src.type = DataSourceType::Gauge;
	ds.sources.push_back(src);

	ValueList vl;
	vl.plugin = "cpu";
	vl.pluginInstance = "0";
	vl.type = "percent";
	vl.typeInstance = "user";
	vl.interval = CdTime::FromDouble(10.0);
	vl.values.push_back(Value::Gauge(23.5));

	std::string json = fmt.Format(ds, vl);

	// Should contain all expected fields
	EXPECT_NE(json.find("\"plugin\":\"cpu\""), std::string::npos);
	EXPECT_NE(json.find("\"plugin_instance\":\"0\""), std::string::npos);
	EXPECT_NE(json.find("\"type\":\"percent\""), std::string::npos);
	EXPECT_NE(json.find("\"type_instance\":\"user\""), std::string::npos);
	EXPECT_NE(json.find("\"host\":\"test-host\""), std::string::npos);
	EXPECT_NE(json.find("\"data_type\":\"gauge\""), std::string::npos);
	EXPECT_NE(json.find("\"value\":23.5"), std::string::npos);
	EXPECT_NE(json.find("\"interval_sec\":10"), std::string::npos);
	EXPECT_NE(json.find("\"ds_type\":\"gauge\""), std::string::npos);
	EXPECT_NE(json.find("\"timestamp\""), std::string::npos);
}

TEST(JsonFormatterTest, FormatDeriveValue)
{
	JsonFormatter fmt;

	DataSet ds;
	ds.type = "derive";
	DataSource src;
	src.name = "rx_bytes";
	src.type = DataSourceType::Derive;
	ds.sources.push_back(src);

	ValueList vl;
	vl.plugin = "network";
	vl.type = "if_octets";
	vl.values.push_back(Value::Derive(123456));

	std::string json = fmt.Format(ds, vl);

	EXPECT_NE(json.find("\"data_type\":\"derive\""), std::string::npos);
	EXPECT_NE(json.find("\"ds_name\":\"rx_bytes\""), std::string::npos);
	EXPECT_NE(json.find("123456"), std::string::npos);
}

TEST(JsonFormatterTest, FormatMultipleValues)
{
	JsonFormatter fmt;

	DataSet ds;
	ds.type = "if_octets";
	DataSource rxSrc;
	rxSrc.name = "rx";
	rxSrc.type = DataSourceType::Derive;
	DataSource txSrc;
	txSrc.name = "tx";
	txSrc.type = DataSourceType::Derive;
	ds.sources.push_back(rxSrc);
	ds.sources.push_back(txSrc);

	ValueList vl;
	vl.plugin = "network";
	vl.pluginInstance = "eth0";
	vl.type = "if_octets";
	vl.values.push_back(Value::Derive(100));
	vl.values.push_back(Value::Derive(200));

	std::string json = fmt.Format(ds, vl);

	// Multiple values → should produce "values" array
	EXPECT_NE(json.find("\"values\""), std::string::npos);
	EXPECT_NE(json.find("\"ds_name\":\"rx\""), std::string::npos);
	EXPECT_NE(json.find("\"ds_name\":\"tx\""), std::string::npos);
}

TEST(JsonFormatterTest, FormatBatch)
{
	JsonFormatter fmt;

	DataSet ds;
	ds.type = "gauge";
	DataSource src;
	src.name = "value";
	src.type = DataSourceType::Gauge;
	ds.sources.push_back(src);

	ValueList vl1;
	vl1.plugin = "cpu";
	vl1.type = "percent";
	vl1.values.push_back(Value::Gauge(10.0));

	ValueList vl2;
	vl2.plugin = "memory";
	vl2.type = "percent";
	vl2.values.push_back(Value::Gauge(60.0));

	std::vector<std::pair<DataSet, ValueList>> entries;
	entries.push_back({ds, vl1});
	entries.push_back({ds, vl2});

	std::string json = fmt.FormatBatch(entries, "smart-camera-01");

	EXPECT_NE(json.find("\"host\":\"smart-camera-01\""), std::string::npos);
	EXPECT_NE(json.find("\"metrics\""), std::string::npos);
	EXPECT_NE(json.find("\"count\":2"), std::string::npos);
	EXPECT_NE(json.find("\"plugin\":\"cpu\""), std::string::npos);
	EXPECT_NE(json.find("\"plugin\":\"memory\""), std::string::npos);
}

TEST(JsonFormatterTest, EmptyValuesProducesValidJson)
{
	JsonFormatter fmt;

	DataSet ds;
	ds.type = "gauge";
	ValueList vl;
	vl.plugin = "test";
	vl.type = "test";
	// No values added

	std::string json = fmt.Format(ds, vl);
	EXPECT_NE(json.find("\"plugin\":\"test\""), std::string::npos);
	// Should still be valid JSON (no crash)
	EXPECT_FALSE(json.empty());
}

TEST(JsonFormatterTest, NoHostOmitsField)
{
	JsonFormatter fmt;
	// Don't set host

	DataSet ds;
	ds.type = "gauge";
	DataSource src;
	src.name = "v";
	src.type = DataSourceType::Gauge;
	ds.sources.push_back(src);

	ValueList vl;
	vl.plugin = "cpu";
	vl.type = "percent";
	vl.values.push_back(Value::Gauge(50.0));

	std::string json = fmt.Format(ds, vl);
	EXPECT_EQ(json.find("\"host\""), std::string::npos);
}

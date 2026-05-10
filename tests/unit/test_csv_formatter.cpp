#include <gtest/gtest.h>

#include "output/CsvFormatter.h"
#include "types/DataSet.h"
#include "types/ValueList.h"

namespace
{
DataSet MakeDataSet(std::initializer_list<std::string> sourceNames)
{
	DataSet ds;
	ds.type = "test_type";
	for (const auto &name : sourceNames)
	{
		DataSource src;
		src.name = name;
		src.type = DataSourceType::Gauge;
		ds.sources.push_back(src);
	}
	return ds;
}
}

TEST(CsvFormatterTest, HeaderUsesDataSourceNames)
{
	DataSet ds = MakeDataSet({"rx", "tx"});

	EXPECT_EQ(CsvFormatter::Header(ds), "epoch,rx,tx");
}

TEST(CsvFormatterTest, HeaderEscapesCsvSpecialCharacters)
{
	DataSet ds = MakeDataSet({"plain", "has,comma", "has\"quote"});

	EXPECT_EQ(CsvFormatter::Header(ds),
	          "epoch,plain,\"has,comma\",\"has\"\"quote\"");
}

TEST(CsvFormatterTest, FormatWritesEpochAndTypedValues)
{
	CsvFormatter formatter;
	DataSet ds = MakeDataSet({"gauge", "derive", "counter", "absolute"});

	ValueList vl;
	vl.time = CdTime::FromDouble(12.3456);
	vl.values.push_back(Value::Gauge(1.25));
	vl.values.push_back(Value::Derive(-42));
	vl.values.push_back(Value::Counter(99));
	vl.values.push_back(Value::Absolute(7));

	EXPECT_EQ(formatter.Format(ds, vl), "12.346,1.250,-42,99,7");
}

TEST(CsvFormatterTest, HeaderFallsBackForMissingDataSources)
{
	DataSet ds = MakeDataSet({"first"});

	ValueList vl;
	vl.time = CdTime::FromDouble(1.0);
	vl.values.push_back(Value::Gauge(2.0));
	vl.values.push_back(Value::Gauge(3.0));

	EXPECT_EQ(CsvFormatter::Header(ds, vl), "epoch,first,value_1");
}


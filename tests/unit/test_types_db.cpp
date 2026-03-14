#include <gtest/gtest.h>
#include "config/TypesDb.h"

#include <fstream>
#include <cmath>
#include <cstdio>

/// Helper: create a temp file with given content, return its path.
static std::string WriteTempFile(const std::string &content)
{
	std::string path = "test_types_db_tmp.db";
	std::ofstream out(path);
	out << content;
	out.close();
	return path;
}

/// Helper: clean up temp file.
static void RemoveTempFile(const std::string &path)
{
	std::remove(path.c_str());
}

// ─── Basic Loading ──────────────────────────────────────────

TEST(TypesDbTest, LoadSingleSource)
{
	std::string path = WriteTempFile("cpu\tvalue:DERIVE:0:U\n");
	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);
	EXPECT_EQ(db.Size(), 1u);

	const DataSet *ds = db.GetDataSet("cpu");
	ASSERT_NE(ds, nullptr);
	EXPECT_EQ(ds->type, "cpu");
	ASSERT_EQ(ds->sources.size(), 1u);
	EXPECT_EQ(ds->sources[0].name, "value");
	EXPECT_EQ(ds->sources[0].type, DataSourceType::Derive);
	EXPECT_DOUBLE_EQ(ds->sources[0].min, 0.0);
	EXPECT_TRUE(std::isnan(ds->sources[0].max));

	RemoveTempFile(path);
}

TEST(TypesDbTest, LoadMultipleSources)
{
	std::string path = WriteTempFile(
		"df\tused:GAUGE:0:1125899906842623, free:GAUGE:0:1125899906842623\n");
	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);

	const DataSet *ds = db.GetDataSet("df");
	ASSERT_NE(ds, nullptr);
	ASSERT_EQ(ds->sources.size(), 2u);

	EXPECT_EQ(ds->sources[0].name, "used");
	EXPECT_EQ(ds->sources[0].type, DataSourceType::Gauge);
	EXPECT_DOUBLE_EQ(ds->sources[0].min, 0.0);
	EXPECT_DOUBLE_EQ(ds->sources[0].max, 1125899906842623.0);

	EXPECT_EQ(ds->sources[1].name, "free");
	EXPECT_EQ(ds->sources[1].type, DataSourceType::Gauge);

	RemoveTempFile(path);
}

// ─── All Data Source Types ──────────────────────────────────

TEST(TypesDbTest, AllSourceTypes)
{
	std::string path = WriteTempFile(
		"t_gauge\tval:GAUGE:0:100\n"
		"t_counter\tval:COUNTER:0:U\n"
		"t_derive\tval:DERIVE:U:U\n"
		"t_absolute\tval:ABSOLUTE:0:U\n");

	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);
	EXPECT_EQ(db.Size(), 4u);

	EXPECT_EQ(db.GetDataSet("t_gauge")->sources[0].type, DataSourceType::Gauge);
	EXPECT_EQ(db.GetDataSet("t_counter")->sources[0].type, DataSourceType::Counter);
	EXPECT_EQ(db.GetDataSet("t_derive")->sources[0].type, DataSourceType::Derive);
	EXPECT_EQ(db.GetDataSet("t_absolute")->sources[0].type, DataSourceType::Absolute);

	RemoveTempFile(path);
}

// ─── Unbounded Values (U) ───────────────────────────────────

TEST(TypesDbTest, UnboundedMinMax)
{
	std::string path = WriteTempFile("counter\tvalue:COUNTER:U:U\n");
	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);

	const DataSet *ds = db.GetDataSet("counter");
	ASSERT_NE(ds, nullptr);
	EXPECT_TRUE(std::isnan(ds->sources[0].min));
	EXPECT_TRUE(std::isnan(ds->sources[0].max));

	RemoveTempFile(path);
}

// ─── Comment & Empty Lines ──────────────────────────────────

TEST(TypesDbTest, SkipCommentsAndEmpty)
{
	std::string path = WriteTempFile(
		"# This is a comment\n"
		"\n"
		"cpu\tvalue:DERIVE:0:U\n"
		"# Another comment\n"
		"\n"
		"memory\tvalue:GAUGE:0:281474976710656\n");

	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);
	EXPECT_EQ(db.Size(), 2u);
	EXPECT_NE(db.GetDataSet("cpu"), nullptr);
	EXPECT_NE(db.GetDataSet("memory"), nullptr);

	RemoveTempFile(path);
}

// ─── Lookup ─────────────────────────────────────────────────

TEST(TypesDbTest, LookupMissingType)
{
	std::string path = WriteTempFile("cpu\tvalue:DERIVE:0:U\n");
	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);

	EXPECT_EQ(db.GetDataSet("nonexistent"), nullptr);

	RemoveTempFile(path);
}

TEST(TypesDbTest, EmptyDbSize)
{
	TypesDb db;
	EXPECT_EQ(db.Size(), 0u);
	EXPECT_EQ(db.GetDataSet("anything"), nullptr);
}

// ─── File Not Found ─────────────────────────────────────────

TEST(TypesDbTest, FileNotFound)
{
	TypesDb db;
	EXPECT_EQ(db.Load("/nonexistent/path/types.db"), -1);
	EXPECT_EQ(db.Size(), 0u);
}

// ─── Multiple Types ─────────────────────────────────────────

TEST(TypesDbTest, LoadMultipleTypes)
{
	std::string path = WriteTempFile(
		"buffer\tvalue:GAUGE:0:18446744073709551615\n"
		"cpu\tvalue:DERIVE:0:U\n"
		"percent\tvalue:GAUGE:0:100.1\n"
		"ps_disk_octets\tread:DERIVE:0:U, write:DERIVE:0:U\n");

	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);
	EXPECT_EQ(db.Size(), 4u);

	// Verify percent type with floating point max
	const DataSet *pct = db.GetDataSet("percent");
	ASSERT_NE(pct, nullptr);
	EXPECT_DOUBLE_EQ(pct->sources[0].max, 100.1);

	// Verify multi-source type
	const DataSet *disk = db.GetDataSet("ps_disk_octets");
	ASSERT_NE(disk, nullptr);
	ASSERT_EQ(disk->sources.size(), 2u);
	EXPECT_EQ(disk->sources[0].name, "read");
	EXPECT_EQ(disk->sources[1].name, "write");

	RemoveTempFile(path);
}

// ─── Spaces instead of tabs ─────────────────────────────────

TEST(TypesDbTest, SpaceSeparated)
{
	std::string path = WriteTempFile(
		"uptime                  value:GAUGE:0:4294967295\n");
	TypesDb db;
	ASSERT_EQ(db.Load(path), 0);

	const DataSet *ds = db.GetDataSet("uptime");
	ASSERT_NE(ds, nullptr);
	EXPECT_DOUBLE_EQ(ds->sources[0].max, 4294967295.0);

	RemoveTempFile(path);
}

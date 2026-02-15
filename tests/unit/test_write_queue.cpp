#include <gtest/gtest.h>
#include "core/WriteQueue.h"

/// Stub write plugin for queue tests.
class StubWriter : public IPlugin
{
public:
	std::string Name() const override { return "stub_writer"; }
	bool HasWrite() const override { return true; }
	int Write(const DataSet &ds, const ValueList &vl) override
	{
		(void)ds;
		lastValues.push_back(vl);
		return 0;
	}

	std::vector<ValueList> lastValues;
};

TEST(WriteQueueTest, EnqueueAndDrain)
{
	WriteQueue wq(128);
	StubWriter writer;
	std::vector<IPlugin *> writers = {&writer};

	DataSet ds;
	ds.type = "gauge";
	ValueList vl;
	vl.plugin = "cpu";
	vl.values.push_back(Value::Gauge(42.0));

	wq.Enqueue(ds, vl);
	EXPECT_EQ(wq.Size(), 1u);

	size_t processed = wq.DrainBatch(writers, 10);
	EXPECT_EQ(processed, 1u);
	EXPECT_EQ(wq.Size(), 0u);
	EXPECT_EQ(writer.lastValues.size(), 1u);
	EXPECT_EQ(writer.lastValues[0].plugin, "cpu");
}

TEST(WriteQueueTest, BoundedDropsOldest)
{
	WriteQueue wq(2);  // capacity = 2

	DataSet ds;
	ValueList vl1, vl2, vl3;
	vl1.plugin = "first";
	vl2.plugin = "second";
	vl3.plugin = "third";

	wq.Enqueue(ds, vl1);
	wq.Enqueue(ds, vl2);
	EXPECT_EQ(wq.Size(), 2u);
	EXPECT_EQ(wq.DroppedCount(), 0u);

	wq.Enqueue(ds, vl3);  // should drop "first"
	EXPECT_EQ(wq.Size(), 2u);
	EXPECT_EQ(wq.DroppedCount(), 1u);

	StubWriter writer;
	std::vector<IPlugin *> writers = {&writer};
	wq.DrainAll(writers);

	ASSERT_EQ(writer.lastValues.size(), 2u);
	EXPECT_EQ(writer.lastValues[0].plugin, "second");
	EXPECT_EQ(writer.lastValues[1].plugin, "third");
}

TEST(WriteQueueTest, EmptyQueueDrain)
{
	WriteQueue wq;
	StubWriter writer;
	std::vector<IPlugin *> writers = {&writer};

	size_t processed = wq.DrainBatch(writers, 10);
	EXPECT_EQ(processed, 0u);
	EXPECT_TRUE(wq.Empty());
}

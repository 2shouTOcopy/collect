#include <gtest/gtest.h>
#include "core/PluginManager.h"
#include "core/WriteQueue.h"

/// Integration test: mock read plugin → DispatchValues → WriteQueue → mock writer.

class IntegReadPlugin : public IPlugin
{
public:
	std::string Name() const override { return "integ_reader"; }
	bool HasRead() const override { return true; }
	int Read() override
	{
		// In real code, this would call PluginManager::DispatchValues
		readCount++;
		return 0;
	}
	int readCount = 0;
};

class IntegWritePlugin : public IPlugin
{
public:
	std::string Name() const override { return "integ_writer"; }
	bool HasWrite() const override { return true; }
	int Write(const DataSet &ds, const ValueList &vl) override
	{
		(void)ds;
		writtenPlugins.push_back(vl.plugin);
		return 0;
	}
	std::vector<std::string> writtenPlugins;
};

TEST(PipelineTest, ReadToWritePipeline)
{
	// Setup
	PluginManager pm;
	IntegReadPlugin reader;
	IntegWritePlugin writer;

	pm.Register(&reader);
	pm.Register(&writer);

	EXPECT_EQ(pm.GetReadPlugins().size(), 1u);
	EXPECT_EQ(pm.GetWritePlugins().size(), 1u);

	// Simulate read
	reader.Read();
	EXPECT_EQ(reader.readCount, 1);

	// Simulate dispatch to queue
	WriteQueue wq(128);
	DataSet ds;
	ds.type = "gauge";
	ValueList vl;
	vl.plugin = "cpu";
	vl.values.push_back(Value::Gauge(50.0));

	wq.Enqueue(ds, vl);

	// Drain to writer
	std::vector<IPlugin *> writers = {&writer};
	wq.DrainAll(writers);

	ASSERT_EQ(writer.writtenPlugins.size(), 1u);
	EXPECT_EQ(writer.writtenPlugins[0], "cpu");
}

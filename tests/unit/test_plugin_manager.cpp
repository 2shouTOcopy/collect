#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/PluginManager.h"

/// Mock plugin for testing PluginManager registration and dispatch.
class MockReadPlugin : public IPlugin
{
public:
	std::string Name() const override { return "mock_read"; }
	bool HasRead()  const override { return true; }
	bool HasWrite() const override { return false; }
	bool HasFlush() const override { return false; }
	int Read() override { return 0; }
};

class MockWritePlugin : public IPlugin
{
public:
	std::string Name() const override { return "mock_write"; }
	bool HasRead()  const override { return false; }
	bool HasWrite() const override { return true; }
	bool HasFlush() const override { return true; }
	int Write(const DataSet &ds, const ValueList &vl) override
	{
		(void)ds;
		(void)vl;
		return 0;
	}
};

TEST(PluginManagerTest, RegisterClassifiesReadPlugin)
{
	PluginManager pm;
	MockReadPlugin readPlugin;

	pm.Register(&readPlugin);

	EXPECT_EQ(pm.PluginCount(), 1u);
	EXPECT_EQ(pm.GetReadPlugins().size(), 1u);
	EXPECT_EQ(pm.GetWritePlugins().size(), 0u);
}

TEST(PluginManagerTest, RegisterClassifiesWritePlugin)
{
	PluginManager pm;
	MockWritePlugin writePlugin;

	pm.Register(&writePlugin);

	EXPECT_EQ(pm.PluginCount(), 1u);
	EXPECT_EQ(pm.GetReadPlugins().size(), 0u);
	EXPECT_EQ(pm.GetWritePlugins().size(), 1u);
}

TEST(PluginManagerTest, FindPluginByName)
{
	PluginManager pm;
	MockReadPlugin readPlugin;
	pm.Register(&readPlugin);

	auto *found = pm.FindPlugin("mock_read");
	EXPECT_NE(found, nullptr);
	EXPECT_EQ(found->Name(), "mock_read");

	auto *notFound = pm.FindPlugin("nonexistent");
	EXPECT_EQ(notFound, nullptr);
}

TEST(PluginManagerTest, RejectsNullPlugin)
{
	PluginManager pm;
	pm.Register(nullptr);
	EXPECT_EQ(pm.PluginCount(), 0u);
}
